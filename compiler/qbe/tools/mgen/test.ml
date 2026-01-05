open Match
open Fuzz
open Cgen

(* unit tests *)

let test_pattern_match =
  let pm = pattern_match
  and nm = fun x y -> not (pattern_match x y) in
  begin
    assert (nm (Atm Tmp) (Atm (Con 42L)));
    assert (pm (Atm AnyCon) (Atm (Con 42L)));
    assert (nm (Atm (Con 42L)) (Atm AnyCon));
    assert (nm (Atm (Con 42L)) (Atm Tmp));
  end

let test_peel =
  let o = Kw, Oadd in
  let p = Bnr (o, Bnr (o, Atm Tmp, Atm Tmp),
                  Atm (Con 42L)) in
  let l = peel p () in
  let () = assert (List.length l = 3) in
  let atomic_p (p, _) =
    match p with Atm _ -> true | _ -> false in
  let () = assert (List.for_all atomic_p l) in
  let l = List.map (fun (p, c) -> fold_cursor c p) l in
  let () = assert (List.for_all ((=) p) l) in
  ()

let test_fold_pairs =
  let l = [1; 2; 3; 4; 5] in
  let p = fold_pairs l l [] (fun a b -> a :: b) in
  let () = assert (List.length p = 25) in
  let p = sort_uniq compare p in
  let () = assert (List.length p = 25) in
  ()

(* test pattern & state *)

let print_sm oc =
  StateMap.iter (fun k s' ->
    match k with
    | K (o, sl, sr) ->
        let top =
          List.fold_left (fun top c ->
            match c with
            | Top r -> top ^ " " ^ r
            | _ -> top) "" s'.point
        in
        Printf.fprintf oc
          "  (%s %d %d) -> %d%s\n"
          (show_op o)
          sl.id sr.id s'.id top)

let rules =
  let oa = Kl, Oadd in
  let om = Kl, Omul in
  let va = Var ("a", Tmp)
  and vb = Var ("b", Tmp)
  and vc = Var ("c", Tmp)
  and vs = Var ("s", Tmp) in
  let vars = ["a"; "b"; "c"; "s"] in
  let rule name pattern =
    List.map
      (fun pattern -> {name; vars; pattern})
      (ac_equiv pattern)
  in
  match `X64Addr with
  (* ------------------------------- *)
  | `X64Addr ->
    (* o + b *)
    rule "ob" (Bnr (oa, Atm Tmp, Atm AnyCon))
    @ (* b + s * m *)
    rule "bsm" (Bnr (oa, vb, Bnr (om, Var ("m", Con 2L), vs)))
    @
    rule "bsm" (Bnr (oa, vb, Bnr (om, Var ("m", Con 4L), vs)))
    @
    rule "bsm" (Bnr (oa, vb, Bnr (om, Var ("m", Con 8L), vs)))
    @  (* b + s *)
    rule "bs1" (Bnr (oa, vb, vs))
    @ (* o + s * m *)
    (* rule "osm" (Bnr (oa, Atm AnyCon, Bnr (om, Atm (Con 4L), Atm Tmp))) *) []
    @ (* o + b + s *)
    rule "obs1" (Bnr (oa, Bnr (oa, Var ("o", AnyCon), vb), vs))
    @ (* o + b + s * m *)
    rule "obsm" (Bnr (oa, Bnr (oa, Var ("o", AnyCon), vb),
                          Bnr (om, Var ("m", Con 2L), vs)))
    @
    rule "obsm" (Bnr (oa, Bnr (oa, Var ("o", AnyCon), vb),
                          Bnr (om, Var ("m", Con 4L), vs)))
    @
    rule "obsm" (Bnr (oa, Bnr (oa, Var ("o", AnyCon), vb),
                          Bnr (om, Var ("m", Con 8L), vs)))
  (* ------------------------------- *)
  | `Add3 ->
    [ { name = "add"
      ; vars = []
      ; pattern = Bnr (oa, va, Bnr (oa, vb, vc)) } ] @
    [ { name = "add"
      ; vars = []
      ; pattern = Bnr (oa, Bnr (oa, va, vb), vc) } ]

(*

let sa, am, sm = generate_table rules
let () =
  Array.iteri (fun i s ->
      Format.printf "@[state %d: %s@]@."
        i (show_pattern s.seen))
    sa
let () = print_sm stdout sm; flush stdout

let matcher = lr_matcher sm sa rules "obsm" (* XXX *)
let () = Format.printf "@[<v>%a@]@." Action.pp matcher
let () = Format.printf "@[matcher size: %d@]@." (Action.size matcher)

let numbr = make_numberer sa am sm

let () =
  let opts = { pfx = ""
             ; static = true
             ; oc = stdout } in
  emit_c opts numbr;
  emit_matchers opts
    [ ( ["b"; "o"; "s"; "m"]
      , "obsm"
      , matcher ) ]

(*
let tp = fuzz_numberer rules numbr
let () = test_matchers tp numbr rules
*)

*)
