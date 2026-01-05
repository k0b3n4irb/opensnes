open Match

type options =
  { pfx: string
  ; static: bool
  ; oc: out_channel }

type side = L | R

type id_pred =
  | InBitSet of Int64.t
  | Ge of int
  | Eq of int

and id_test =
  | Pred of (side * id_pred)
  | And of id_test * id_test

type case_code =
  | Table of ((int * int) * int) list
  | IfThen of
      { test: id_test
      ; cif: case_code
      ; cthen: case_code option }
  | Return of int

type case =
  { swap: bool
  ; code: case_code }

let cgen_case tmp nstates map =
  let cgen_test ids =
    match ids with
    | [id] -> Eq id
    | _ ->
        let min_id =
          List.fold_left min max_int ids in
        if List.length ids = nstates - min_id
        then Ge min_id
        else begin
          assert (nstates <= 64);
          InBitSet
            (List.fold_left (fun bs id ->
                 Int64.logor bs
                   (Int64.shift_left 1L id))
                0L ids)
        end
  in
  let symmetric =
    let inverse ((l, r), x) = ((r, l), x) in
    setify map = setify (List.map inverse map) in
  let map =
    let ordered ((l, r), _) = r <= l in
    if symmetric then
      List.filter ordered map
    else map
  in
  let exception BailToTable in
  try
    let st =
      match setify (List.map snd map) with
      | [st] -> st
      | _ -> raise BailToTable
    in
    (* the operation considered can only
     * generate a single state *)
    let pairs = List.map fst map in
    let ls, rs = List.split pairs in
    let ls = setify ls and rs = setify rs in
    if List.length ls > 1 && List.length rs > 1 then
      raise BailToTable;
    { swap = symmetric
    ; code =
        let pl = Pred (L, cgen_test ls)
        and pr = Pred (R, cgen_test rs) in
        IfThen
          { test = And (pl, pr)
          ; cif = Return st
          ; cthen = Some (Return tmp) } }
  with BailToTable ->
    { swap = symmetric
    ; code = Table map }

let show_op (_cls, op) =
  "O" ^ show_op_base op

let indent oc i =
  Printf.fprintf oc "%s" (String.sub "\t\t\t\t\t" 0 i)

let emit_swap oc i =
  let pf m = Printf.fprintf oc m in
  let pfi n m = indent oc n; pf m in
  pfi i "if (l < r)\n";
  pfi (i+1) "t = l, l = r, r = t;\n"

let gen_tables oc tmp pfx nstates (op, c) =
  let i = 1 in
  let pf m = Printf.fprintf oc m in
  let pfi n m = indent oc n; pf m in
  let ntables = ref 0 in
  (* we must follow the order in which
   * we visit code in emit_case, or
   * else ntables goes out of sync *)
  let base = pfx ^ show_op op in
  let swap = c.swap in
  let rec gen c =
    match c with
    | Table map ->
        let name =
          if !ntables = 0 then base else
          base ^ string_of_int !ntables
        in
        assert (nstates <= 256);
        if swap then
          let n = nstates * (nstates + 1) / 2 in
          pfi i "static uchar %stbl[%d] = {\n" name n
        else
          pfi i "static uchar %stbl[%d][%d] = {\n"
            name nstates nstates;
        for l = 0 to nstates - 1 do
          pfi (i+1) "";
          for r = 0 to nstates - 1 do
            if not swap || r <= l then
              begin
                pf "%d"
                  (try List.assoc (l,r) map
                   with Not_found -> tmp);
                pf ",";
              end
          done;
          pf "\n";
        done;
        pfi i "};\n"
    | IfThen {cif; cthen} ->
        gen cif;
        Option.iter gen cthen
    | Return _ -> ()
  in
  gen c.code

let emit_case oc pfx no_swap (op, c) =
  let fpf = Printf.fprintf in
  let pf m = fpf oc m in
  let pfi n m = indent oc n; pf m in
  let rec side oc = function
    | L -> fpf oc "l"
    | R -> fpf oc "r"
  in
  let pred oc (s, pred) =
    match pred with
    | InBitSet bs -> fpf oc "BIT(%a) & %#Lx" side s bs
    | Eq id -> fpf oc "%a == %d" side s id
    | Ge id -> fpf oc "%d <= %a" id side s
  in
  let base = pfx ^ show_op op in
  let swap = c.swap in
  let ntables = ref 0 in
  let rec code i c =
    match c with
    | Return id -> pfi i "return %d;\n" id
    | Table map ->
        let name =
          if !ntables = 0 then base else
          base ^ string_of_int !ntables
        in
        incr ntables;
        if swap then
          pfi i "return %stbl[(l + l*l)/2 + r];\n" name
        else pfi i "return %stbl[l][r];\n" name
    | IfThen ({test = And (And (t1, t2), t3)} as r) ->
        code i @@ IfThen
          {r with test = And (t1, And (t2, t3))}
    | IfThen {test = And (Pred p, t); cif; cthen} ->
        pfi i "if (%a)\n" pred p;
        code i (IfThen {test = t; cif; cthen})
    | IfThen {test = Pred p; cif; cthen} ->
        pfi i "if (%a) {\n" pred p;
        code (i+1) cif;
        pfi i "}\n";
        Option.iter (code i) cthen
  in
  pfi 1 "case %s:\n" (show_op op);
  if not no_swap && c.swap then
    emit_swap oc 2;
  code 2 c.code

let emit_list
    ?(limit=60) ?(cut_before_sep=false)
    ~col ~indent:i ~sep ~f oc l =
  let sl = String.length sep in
  let rstripped_sep, rssl =
    if sep.[sl - 1] = ' ' then
      String.sub sep 0 (sl - 1), sl - 1
    else sep, sl
  in
  let lstripped_sep, lssl =
    if sep.[0] = ' ' then
      String.sub sep 1 (sl - 1), sl - 1
    else sep, sl
  in
  let rec line col acc = function
    | [] -> (List.rev acc, [])
    | s :: l ->
        let col = col + sl + String.length s in
        let no_space =
          if cut_before_sep || l = [] then
            col > limit
          else
            col + rssl > limit
        in
        if no_space then
          (List.rev acc, s :: l)
        else
          line col (s :: acc) l
  in
  let rec go col l =
    if l = [] then () else
    let ll, l = line col [] l in
    Printf.fprintf oc "%s" (String.concat sep ll);
    if l <> [] && cut_before_sep then begin
      Printf.fprintf oc "\n";
      indent oc i;
      Printf.fprintf oc "%s" lstripped_sep;
      go (8*i + lssl) l
    end else if l <> [] then begin
      Printf.fprintf oc "%s\n" rstripped_sep;
      indent oc i;
      go (8*i) l
    end else ()
  in
  go col (List.map f l)

let emit_numberer opts n =
  let pf m = Printf.fprintf opts.oc m in
  let tmp = (atom_state n Tmp).id in
  let con = (atom_state n AnyCon).id in
  let nst = Array.length n.states in
  let cases =
    StateMap.by_ops n.statemap |>
    List.map (fun (op, map) ->
        (op, cgen_case tmp nst map))
  in
  let all_swap =
    List.for_all (fun (_, c) -> c.swap) cases in
  (* opn() *)
  if opts.static then pf "static ";
  pf "int\n";
  pf "%sopn(int op, int l, int r)\n" opts.pfx;
  pf "{\n";
  cases |> List.iter
    (gen_tables opts.oc tmp opts.pfx nst);
  if List.exists (fun (_, c) -> c.swap) cases then
    pf "\tint t;\n\n";
  if all_swap then emit_swap opts.oc 1;
  pf "\tswitch (op) {\n";
  cases |> List.iter
    (emit_case opts.oc opts.pfx all_swap);
  pf "\tdefault:\n";
  pf "\t\treturn %d;\n" tmp;
  pf "\t}\n";
  pf "}\n\n";
  (* refn() *)
  if opts.static then pf "static ";
  pf "int\n";
  pf "%srefn(Ref r, Num *tn, Con *con)\n" opts.pfx;
  pf "{\n";
  let cons =
    List.filter_map (function
        | (Con c, s) -> Some (c, s.id)
        | _ -> None)
      n.atoms
  in
  if cons <> [] then
    pf "\tint64_t n;\n\n";
  pf "\tswitch (rtype(r)) {\n";
  pf "\tcase RTmp:\n";
  if tmp <> 0 then begin
    assert
      (List.exists (fun (_, s) ->
           s.id = 0
         ) n.atoms &&
       (* no temp should ever get state 0 *)
       List.for_all (fun (a, s) ->
           s.id <> 0 ||
           match a with
           | AnyCon | Con _ -> true
           | _ -> false
         ) n.atoms);
    pf "\t\tif (!tn[r.val].n)\n";
    pf "\t\t\ttn[r.val].n = %d;\n" tmp;
  end;
  pf "\t\treturn tn[r.val].n;\n";
  pf "\tcase RCon:\n";
  if cons <> [] then begin
    pf "\t\tif (con[r.val].type != CBits)\n";
    pf "\t\t\treturn %d;\n" con;
    pf "\t\tn = con[r.val].bits.i;\n";
    cons |> inverse |> group_by_fst
    |> List.iter (fun (id, cs) ->
        pf "\t\tif (";
        emit_list ~cut_before_sep:true
          ~col:20 ~indent:2 ~sep:" || "
          ~f:(fun c -> "n == " ^ Int64.to_string c)
          opts.oc cs;
        pf ")\n";
        pf "\t\t\treturn %d;\n" id
      );
  end;
  pf "\t\treturn %d;\n" con;
  pf "\tdefault:\n";
  pf "\t\treturn INT_MIN;\n";
  pf "\t}\n";
  pf "}\n\n";
  (* match[]: patterns per state *)
  if opts.static then pf "static ";
  pf "bits %smatch[%d] = {\n" opts.pfx nst;
  n.states |> Array.iteri (fun sn s ->
      let tops =
        List.filter_map (function
          | Top ("$" | "%") -> None
          | Top r -> Some ("BIT(P" ^ r ^ ")")
          | _ -> None) s.point |> setify
      in
      if tops <> [] then
        pf "\t[%d] = %s,\n"
          sn (String.concat " | " tops);
    );
  pf "};\n\n"

let var_id vars f =
  List.mapi (fun i x -> (x, i)) vars |>
  List.assoc f

let compile_action vars act =
  let pcs = Hashtbl.create 100 in
  let rec gen pc (act: Action.t) =
    try
      [10 + Hashtbl.find pcs act.id]
    with Not_found ->
      let code =
        match act.node with
        | Action.Stop ->
            [0]
        | Action.Push (sym, k) ->
            let c = if sym then 1 else 2 in
            [c] @ gen (pc + 1) k
        | Action.Set (v, {node = Action.Pop k; _})
        | Action.Set (v, ({node = Action.Stop; _} as k)) ->
            let v = var_id vars v in
            [3; v] @ gen (pc + 2) k
        | Action.Set _ ->
            (* for now, only atomic patterns can be
             * tied to a variable, so Set must be
             * followed by either Pop or Stop *)
            assert false
        | Action.Pop k ->
            [4] @ gen (pc + 1) k
        | Action.Switch cases ->
            let cases =
              inverse cases |> group_by_fst |>
              List.sort (fun (_, cs1) (_, cs2) ->
                  let n1 = List.length cs1
                  and n2 = List.length cs2 in
                  compare n2 n1)
            in
            (* the last case is the one with
             * the max number of entries *)
            let cases = List.rev (List.tl cases)
            and last = fst (List.hd cases) in
            let ncases =
              List.fold_left (fun n (_, cs) ->
                  List.length cs + n)
                0 cases
            in
            let body_off = 2 + 2 * ncases + 1 in
            let pc, tbl, body =
              List.fold_left
                (fun (pc, tbl, body) (a, cs) ->
                   let ofs = body_off + List.length body in
                   let case = gen pc a in
                   let pc = pc + List.length case in
                   let body = body @ case in
                   let tbl =
                     List.fold_left (fun tbl c ->
                         tbl @ [c; ofs]
                       ) tbl cs
                   in
                   (pc, tbl, body))
                (pc + body_off, [], [])
                cases
            in
            let ofs = body_off + List.length body in
            let tbl = tbl @ [ofs] in
            assert (2 + List.length tbl = body_off);
            [5; ncases] @ tbl @ body @ gen pc last
      in
      if act.node <> Action.Stop then
        Hashtbl.replace pcs act.id pc;
      code
  in
  gen 0 act

let emit_matchers opts ms =
  let pf m = Printf.fprintf opts.oc m in
  if opts.static then pf "static ";
  pf "uchar *%smatcher[] = {\n" opts.pfx;
  List.iter (fun (vars, pname, m) ->
      pf "\t[P%s] = (uchar[]){\n" pname;
      pf "\t\t";
      let bytes = compile_action vars m in
      emit_list
        ~col:16 ~indent:2 ~sep:","
        ~f:string_of_int opts.oc bytes;
      pf "\n";
      pf "\t},\n")
    ms;
  pf "};\n\n"

let emit_c opts n =
  emit_numberer opts n
