type pstate =
  { data: string
  ; line: int
  ; coln: int
  ; indx: int }

type perror =
  { error: string
  ; ps: pstate }

exception ParseError of perror

type 'a parser =
  { fn: 'r. pstate -> ('a -> pstate -> 'r) -> 'r }

let update_pos ps beg fin =
  let l, c = (ref ps.line, ref ps.coln) in
  for i = beg to fin - 1 do
    if ps.data.[i] = '\n' then
      (incr l; c := 0)
    else
      incr c
  done;
  { ps with line = !l; coln = !c }

let pret (type a) (x: a): a parser =
  let fn ps k = k x ps in { fn }

let pfail error: 'a parser =
  let fn ps _ = raise (ParseError {error; ps})
  in { fn }

let por: 'a parser -> 'a parser -> 'a parser =
  fun p1 p2 ->
  let fn ps k =
    try p1.fn ps k with ParseError e1 ->
    try p2.fn ps k with ParseError e2 ->
      if e1.ps.indx > e2.ps.indx then
        raise (ParseError e1)
      else
        raise (ParseError e2)
  in { fn }

let pbind: 'a parser -> ('a -> 'b parser) -> 'b parser =
  fun p1 p2 ->
  let fn ps k =
    p1.fn ps (fun x ps -> (p2 x).fn ps k)
  in { fn }

(* handy for recursive rules *)
let papp p x = pbind (pret x) p

let psnd: 'a parser -> 'b parser -> 'b parser =
  fun p1 p2 -> pbind p1 (fun _x -> p2)

let pfst: 'a parser -> 'b parser -> 'a parser =
  fun p1 p2 -> pbind p1 (fun x -> psnd p2 (pret x))

module Infix = struct
  let ( let* ) = pbind
  let ( ||| ) = por
  let ( |<< ) = pfst
  let ( |>> ) = psnd
end

open Infix

let pre: ?what:string -> string -> string parser =
  fun ?what re ->
  let what =
    match what with
    | None -> Printf.sprintf "%S" re
    | Some what -> what
  and re = Str.regexp re in
  let fn ps k =
    if not (Str.string_match re ps.data ps.indx) then
      (let error =
        Printf.sprintf "expected to match %s" what in
       raise (ParseError {error; ps}));
    let ps =
      let indx = Str.match_end () in
      { (update_pos ps ps.indx indx) with indx }
    in
    k (Str.matched_string ps.data) ps
  in { fn }

let peoi: unit parser =
  let fn ps k =
    if ps.indx <> String.length ps.data then
      raise (ParseError
               { error = "expected end of input"; ps });
    k () ps
  in { fn }

let pws = pre "[ \r\n\t*]*"
let pws1 = pre "[ \r\n\t*]+"

let pthen p1 p2 =
  let* x1 = p1 in
  let* x2 = p2 in
  pret (x1, x2)

let rec plist_tail: 'a parser -> ('a list) parser =
  fun pitem ->
  (pws |>> pre ")" |>> pret []) |||
  (let* itm = pitem in
   let* itms = plist_tail pitem in
   pret (itm :: itms))

let plist pitem =
  pws |>> pre ~what:"a list" "("
  |>> plist_tail pitem

let plist1p p1 pitem =
  pws |>> pre ~what:"a list" "("
  |>> pthen p1 (plist_tail pitem)

let ppair p1 p2 =
  pws |>> pre ~what:"a pair" "("
  |>> pthen p1 p2 |<< pws |<< pre ")"

let run_parser p s =
  let ps =
    {data = s; line = 1; coln = 0; indx = 0} in
  try `Ok (p.fn ps (fun res _ps -> res))
  with ParseError e ->
    let rec bol i =
      if i = 0 then i else
      if i < String.length s && s.[i] = '\n'
      then i+1 (* XXX BUG *)
      else bol (i-1)
    in
    let rec eol i =
      if i = String.length s then i else
      if s.[i] = '\n' then i else
      eol (i+1)
    in
    let bol = bol e.ps.indx in
    let eol = eol e.ps.indx in
    (*
    Printf.eprintf "bol:%d eol:%d indx:%d len:%d\n"
      bol eol e.ps.indx (String.length s);                 (* XXX debug *)
    *)
    let lines =
      String.split_on_char '\n'
        (String.sub s bol (eol - bol))
    in
    let nl = List.length lines in
    let caret = ref (e.ps.indx - bol) in
    let msg = ref [] in
    let pfx = "    > " in
    lines |> List.iteri (fun ln l ->
        if ln <> nl - 1 || l <> "" then begin
          let ll = String.length l + 1 in
          msg := (pfx ^ l ^ "\n") :: !msg;
          if !caret <= ll then begin
            let pad = String.make !caret ' ' in
            msg := (pfx ^ pad ^ "^\n") :: !msg;
          end;
          caret := !caret - ll;
        end;
      );
    `Error
      ( e.ps, e.error
      , String.concat "" (List.rev !msg) )

(* ---------------------------------------- *)
(* pattern parsing                          *)
(* ---------------------------------------- *)
(* Example syntax:

   (with-vars (a b c d)
     (patterns
       (ob   (add (tmp a) (con d)))
       (bsm  (add (tmp b) (mul (tmp m) (con 2 4 8)))) ))
 *)
open Match

let pint64 =
  let* s = pre "[-]?[0-9_]+" in
  pret (Int64.of_string s)

let pid =
  pre ~what:"an identifer"
    "[a-zA-Z][a-zA-Z0-9_]*"

let pop_base =
  let sob, obs = show_op_base, op_bases in
  let* s = pre ~what:"an operator"
      (String.concat "\\|" (List.map sob obs))
  in pret (List.find (fun o -> s = sob o) obs)

let pop = let* ob = pop_base in pret (Kl, ob)

let rec ppat vs =
  let pcons_tail =
    let* cs = plist_tail (pws1 |>> pint64) in
    match cs with
    | [] -> pret [AnyCon]
    | _ -> pret (List.map (fun c -> Con c) cs)
  in
  let pvar =
    let* id = pid in
    if not (List.mem id vs) then
      pfail ("unbound variable: " ^ id)
    else
      pret id
  in
  pws |>> (
    ( let* c = pint64 in pret [Atm (Con c)] )
    |||
    ( pre "(con)" |>> pret [Atm AnyCon] ) |||
    ( let* cs = pre "(con" |>> pcons_tail in
      pret (List.map (fun c -> Atm c) cs) ) |||
    ( let* v = pre "(con" |>> pws1 |>> pvar in
      let* cs = pcons_tail in
      pret (List.map (fun c -> Var (v, c)) cs) )
    |||
    ( pre "(tmp)" |>> pret [Atm Tmp] ) |||
    ( let* v = pre "(tmp" |>> pws1 |>> pvar in
      pws |>> pre ")" |>> pret [Var (v, Tmp)] )
    |||
    ( let* (op, rands) =
        plist1p (pws |>> pop) (papp ppat vs) in
      let nrands = List.length rands in
      if nrands < 2 then
        pfail ( "binary op requires at least"
              ^ " two arguments" )
      else
        let mk x y = Bnr (op, x, y) in
        pret
          (products rands []
             (fun rands pats ->
                (* construct a left-heavy tree *)
                let r0 = List.hd rands in
                let rs = List.tl rands in
                List.fold_left mk r0 rs :: pats)) )
  )

let pwith_vars ?(vs = []) p =
  ( let* vs =
      pws |>> pre "(with-vars" |>> pws |>>
      plist (pws |>> pid)
    in pws |>> p vs |<< pws |<< pre ")" )
  ||| p vs

let ppats =
  pwith_vars @@ fun vs ->
  pre "(patterns" |>> plist_tail
    (pwith_vars ~vs @@ fun vs ->
       let* n, ps = ppair pid (ppat vs) in
       pret (n, vs, ps))

(* ---------------------------------------- *)
(* tests                                    *)
(* ---------------------------------------- *)

let () =
  if false then
  let show_patterns ps =
    "[" ^ String.concat "; "
      (List.map show_pattern ps) ^ "]"
  in
  let pat s =
    Printf.printf "parse %s = " s;
    let vars =
      [ "foobar"; "a"; "b"; "d"
      ; "m"; "s"; "x" ]
    in
    match run_parser (ppat vars) s with
    | `Ok p ->
        Printf.printf "%s\n" (show_patterns p)
    | `Error (_, e, _) ->
        Printf.printf "ERROR: %s\n" e
  in
  pat "42";
  pat "(tmp)";
  pat "(tmp foobar)";
  pat "(con)";
  pat "(con 1 2 3)";
  pat "(con x 1 2 3)";
  pat "(add 1 2)";
  pat "(add 1 2 3 4)";
  pat "(sub 1 2)";
  pat "(sub 1 2 3)";
  pat "(tmp unbound_var)";
  pat "(add 0)";
  pat "(add 1 (add 2 3))";
  pat "(add (tmp a) (con d))";
  pat "(add (tmp b) (mul (tmp m) (con s 2 4 8)))";
  pat "(add (con 1 2) (con 3 4))";
  ()
