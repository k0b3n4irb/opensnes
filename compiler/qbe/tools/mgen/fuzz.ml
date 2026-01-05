(* fuzz the tables and matchers generated *)
open Match

module Buffer: sig
  type 'a t
  val create: ?capacity:int -> unit -> 'a t
  val reset: 'a t -> unit
  val size: 'a t -> int
  val get: 'a t -> int -> 'a
  val set: 'a t -> int -> 'a -> unit
  val push: 'a t -> 'a -> unit
end = struct
  type 'a t =
    { mutable size: int
    ; mutable data: 'a array }
  let mk_array n = Array.make n (Obj.magic 0)
  let create ?(capacity = 10) () =
    if capacity < 0 then invalid_arg "Buffer.make";
    {size = 0; data = mk_array capacity}
  let reset b = b.size <- 0
  let size b = b.size
  let get b n =
    if n >= size b then invalid_arg "Buffer.get";
    b.data.(n)
  let set b n x =
    if n >= size b then invalid_arg "Buffer.set";
    b.data.(n) <- x
  let push b x =
    let cap = Array.length b.data in
    if size b = cap then begin
      let data = mk_array (2 * cap + 1) in
      Array.blit b.data 0 data 0 cap;
      b.data <- data
    end;
    let sz = size b in
    b.size <- sz + 1;
    set b sz x
end

let binop_state n op s1 s2 =
  let key = K (op, s1, s2) in
  try StateMap.find key n.statemap
  with Not_found -> atom_state n Tmp

type id = int
type term_data =
  | Binop of op * id * id
  | Leaf of atomic_pattern
type term =
  { id: id
  ; data: term_data
  ; state: p state }

let pp_term fmt (ta, id) =
  let fpf x = Format.fprintf fmt x in
  let rec pp _fmt id =
    match ta.(id).data with
    | Leaf (Con c) -> fpf "%Ld" c
    | Leaf AnyCon -> fpf "$%d" id
    | Leaf Tmp -> fpf "%%%d" id
    | Binop (op, id1, id2) ->
        fpf "@[(%s@%d:%d @[<hov>%a@ %a@])@]"
          (show_op op) id ta.(id).state.id
          pp id1 pp id2
  in pp fmt id

(* A term pool is a deduplicated set of term
 * that maintains nodes numbering using the
 * statemap passed at creation time *)
module TermPool = struct
  type t =
    { terms: term Buffer.t
    ; hcons: (term_data, id) Hashtbl.t
    ; numbr: numberer }

  let create numbr =
    { terms = Buffer.create ()
    ; hcons = Hashtbl.create 100
    ; numbr }
  let reset tp =
    Buffer.reset tp.terms;
    Hashtbl.clear tp.hcons

  let size tp = Buffer.size tp.terms
  let term tp id = Buffer.get tp.terms id

  let mk_leaf tp atm =
    let data = Leaf atm in
    match Hashtbl.find tp.hcons data with
    | id -> term tp id
    | exception Not_found ->
        let id = Buffer.size tp.terms in
        let state = atom_state tp.numbr atm in
        Buffer.push tp.terms {id; data; state};
        Hashtbl.add tp.hcons data id;
        term tp id
  let mk_binop tp op t1 t2 =
    let data = Binop (op, t1.id, t2.id) in
    match Hashtbl.find tp.hcons data with
    | id -> term tp id
    | exception Not_found ->
        let id = Buffer.size tp.terms in
        let state =
          binop_state tp.numbr op t1.state t2.state
        in
        Buffer.push tp.terms {id; data; state};
        Hashtbl.add tp.hcons data id;
        term tp id

  let rec add_pattern tp = function
    | Bnr (op, p1, p2) ->
        let t1 = add_pattern tp p1 in
        let t2 = add_pattern tp p2 in
        mk_binop tp op t1 t2
    | Atm atm -> mk_leaf tp atm
    | Var (_, atm) -> add_pattern tp (Atm atm)

  let explode_term tp id =
    let rec aux tms n id =
      let t = term tp id in
      match t.data with
      | Leaf _ -> (n, {t with id = n} :: tms)
      | Binop (op, id1, id2) ->
          let n1, tms = aux tms n id1 in
          let n = n1 + 1 in
          let n2, tms = aux tms n id2 in
          let n = n2 + 1 in
          (n, { t with data = Binop (op, n1, n2)
                     ; id = n } :: tms)
    in
    let n, tms = aux [] 0 id in
    Array.of_list (List.rev tms), n
end

module R = Random

(* uniform pick in a list *)
let list_pick l =
  let rec aux n l x =
    match l with
    | [] -> x
    | y :: l ->
        if R.int (n + 1) = 0 then
          aux (n + 1) l y
        else
          aux (n + 1) l x
  in
  match l with
  | [] -> invalid_arg "list_pick"
  | x :: l -> aux 1 l x

let term_pick ~numbr =
  let ops =
    if numbr.ops = [] then
      numbr.ops <-
        (StateMap.fold (fun k _ ops ->
             match k with
             | K (op, _, _) -> op :: ops)
            numbr.statemap [] |> setify);
    numbr.ops
  in
  let rec gen depth =
    (* exponential probability for leaves to
     * avoid skewing towards shallow terms *)
    let atm_prob = 0.75 ** float_of_int depth in
    if R.float 1.0 <= atm_prob || ops = [] then
      let atom, st = list_pick numbr.atoms in
      (st, Atm atom)
    else
      let op = list_pick ops in
      let s1, t1 = gen (depth - 1) in
      let s2, t2 = gen (depth - 1) in
      ( binop_state numbr op s1 s2
      , Bnr (op, t1, t2) )
  in fun ~depth -> gen depth

exception FuzzError

let rec pattern_depth = function
  | Bnr (_, p1, p2) ->
      1 + max (pattern_depth p1) (pattern_depth p2)
  | Atm _ -> 0
  | Var (_, atm) -> pattern_depth (Atm atm)

let ( %% ) a b =
  1e2 *. float_of_int a /. float_of_int b

let progress ?(width = 50) msg pct =
  Format.eprintf "\x1b[2K\r%!";
  let progress_bar fmt =
    let n =
      let fwidth = float_of_int width in
      1 + int_of_float (pct *. fwidth /. 1e2)
    in
    Format.fprintf fmt "  %s%s %.0f%%@?"
      (String.concat "" (List.init n (fun _ -> "▒")))
      (String.make (max 0 (width - n)) '-')
      pct
  in
  Format.kfprintf progress_bar
    Format.err_formatter msg

let fuzz_numberer rules numbr =
  (* pick twice the max pattern depth so we
   * have a chance to find non-trivial numbers
   * for the atomic patterns in the rules *)
  let depth =
    List.fold_left (fun depth r ->
        max depth (pattern_depth r.pattern))
      0 rules * 2
  in
  (* fuzz until the term pool we are constructing
   * is no longer growing fast enough; or we just
   * went through sufficiently many iterations *)
  let max_iter = 1_000_000 in
  let low_insert_rate = 1e-2 in
  let tp = TermPool.create numbr in
  let rec loop new_stats i =
    let (_, _, insert_rate) = new_stats in
    if insert_rate <= low_insert_rate then () else
    if i >= max_iter then () else
    (* periodically update stats *)
    let new_stats =
      let (num, cnt, rate) = new_stats in
      if num land 1023 = 0 then
        let rate =
          0.5 *. (rate +. float_of_int cnt /. 1023.)
        in
        progress "  insert_rate=%.1f%%"
          (i %% max_iter) (rate *. 1e2);
        (num + 1, 0, rate)
      else new_stats
    in
    (* create a term and check that its number is
     * accurate wrt the rules *)
    let st, term = term_pick ~numbr ~depth in
    let state_matched =
      List.filter_map (fun cu ->
          match cu with
          | Top ("$" | "%") -> None
          | Top name -> Some name
          | _ -> None)
        st.point |> setify
    in
    let rule_matched =
      List.filter_map (fun r ->
          if pattern_match r.pattern term then
            Some r.name
          else None)
        rules |> setify
    in
    if state_matched <> rule_matched then begin
      let open Format in
      let pp_str_list =
        let pp_sep fmt () = fprintf fmt ",@ " in
        pp_print_list ~pp_sep pp_print_string
      in
      eprintf "@.@[<v2>fuzz error for %s"
        (show_pattern term);
      eprintf "@ @[state matched: %a@]"
        pp_str_list state_matched;
      eprintf "@ @[rule matched: %a@]"
        pp_str_list rule_matched;
      eprintf "@]@.";
      raise FuzzError;
    end;
    if state_matched = [] then
      loop new_stats (i + 1)
    else
      (* add to the term pool *)
      let old_size = TermPool.size tp in
      let _ = TermPool.add_pattern tp term in
      let new_stats =
        let (num, cnt, rate) = new_stats in
        if TermPool.size tp <> old_size then
          (num + 1, cnt + 1, rate)
        else
          (num + 1, cnt, rate)
      in
      loop new_stats (i + 1)
  in
  loop (1, 0, 1.0) 0;
  Format.eprintf
    "@.@[  generated %.3fMiB of test terms@]@."
    (float_of_int (Obj.reachable_words (Obj.repr tp))
     /.  128. /. 1024.);
  tp

let rec run_matcher stk m (ta, id as t) =
  let state id = ta.(id).state.id in
  match m.Action.node with
  | Action.Switch cases ->
      let m =
        try List.assoc (state id) cases
        with Not_found -> failwith "no switch case"
      in
      run_matcher stk m t
  | Action.Push (sym, m) ->
      let l, r =
        match ta.(id).data with
        | Leaf _ -> failwith "push on leaf"
        | Binop (_, l, r) -> (l, r)
      in
      if sym && state l > state r
      then run_matcher (l :: stk) m (ta, r)
      else run_matcher (r :: stk) m (ta, l)
  | Action.Pop m -> begin
      match stk with
      | id :: stk -> run_matcher stk m (ta, id)
      | [] -> failwith "pop on empty stack"
    end
  | Action.Set (v, m) ->
      (v, id) :: run_matcher stk m t
  | Action.Stop -> []

let rec term_match p (ta, id) =
  let (|>>) x f =
    match x with None -> None | Some x -> f x
  in
  let atom_match a =
    match ta.(id).data with
    | Leaf a' -> pattern_match (Atm a) (Atm a')
    | Binop _ -> pattern_match (Atm a) (Atm Tmp)
  in
  match p with
  | Var (v, a) when atom_match a ->
      Some [(v, id)]
  | Atm a when atom_match a -> Some []
  | (Atm _ | Var _) -> None
  | Bnr (op, pl, pr) -> begin
      match ta.(id).data with
      | Binop (op', idl, idr) when op' = op ->
          term_match pl (ta, idl) |>> fun l1 ->
          term_match pr (ta, idr) |>> fun l2 ->
          Some (l1 @ l2)
      | _ -> None
    end

let test_matchers tp numbr rules =
  let {statemap = sm; states = sa; _} = numbr in
  let total = ref 0 in
  let matchers =
    let htbl = Hashtbl.create (Array.length sa) in
    List.map (fun r -> (r.name, r.pattern)) rules |>
    group_by_fst |>
    List.iter (fun (r, ps) ->
      total := !total + List.length ps;
      let pm = (ps, lr_matcher sm sa rules r) in
      sa |> Array.iter (fun s ->
        if List.mem (Top r) s.point then
          Hashtbl.add htbl s.id pm));
    htbl
  in
  let seen = Hashtbl.create !total in
  for id = 0 to TermPool.size tp - 1 do
    if id land 1023 = 0 ||
       id = TermPool.size tp - 1 then begin
      progress
        "  coverage=%.1f%%"
        (id %% TermPool.size tp)
        (Hashtbl.length seen %% !total)
    end;
    let t = TermPool.explode_term tp id in
    Hashtbl.find_all matchers
      (TermPool.term tp id).state.id |>
    List.iter (fun (ps, m) ->
      let norm = List.fast_sort compare in
      let ok =
        match norm (run_matcher [] m t) with
        | asn -> `Match (List.exists (fun p ->
              match term_match p t with
              | None -> false
              | Some asn' ->
                  if asn = norm asn' then begin
                    Hashtbl.replace seen p ();
                    true
                  end else false) ps)
        | exception e -> `RunFailure e
      in
      if ok <> `Match true then begin
        let open Format in
        let pp_asn fmt asn =
          fprintf fmt "@[<h>";
          pp_print_list
            ~pp_sep:(fun fmt () -> fprintf fmt ";@ ")
            (fun fmt (v, d) ->
              fprintf fmt "@[%s←%d@]" v d)
            fmt asn;
          fprintf fmt "@]"
        in
        eprintf "@.@[<v2>matcher error for";
        eprintf "@ @[%a@]" pp_term t;
        begin match ok with
        | `RunFailure e ->
            eprintf "@ @[exception: %s@]"
              (Printexc.to_string e)
        | `Match (* false *) _ ->
            let asn = run_matcher [] m t in
            eprintf "@ @[assignment: %a@]"
              pp_asn asn;
            eprintf "@ @[<v2>could not match";
            List.iter (fun p ->
                eprintf "@ + @[%s@]"
                  (show_pattern p)) ps;
            eprintf "@]"
        end;
        eprintf "@]@.";
        raise FuzzError
      end)
  done;
  Format.eprintf "@."


