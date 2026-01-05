type cls = Kw | Kl | Ks | Kd
type op_base =
  | Oadd
  | Osub
  | Omul
  | Oor
  | Oshl
  | Oshr
type op = cls * op_base

let op_bases =
  [Oadd; Osub; Omul; Oor; Oshl; Oshr]

let commutative = function
  | (_, (Oadd | Omul | Oor)) -> true
  | (_, _) -> false

let associative = function
  | (_, (Oadd | Omul | Oor)) -> true
  | (_, _) -> false

type atomic_pattern =
  | Tmp
  | AnyCon
  | Con of int64
(* Tmp < AnyCon < Con k *)

type pattern =
  | Bnr of op * pattern * pattern
  | Atm of atomic_pattern
  | Var of string * atomic_pattern

let is_atomic = function
  | (Atm _ | Var _) -> true
  | _ -> false

let show_op_base o =
  match o with
  | Oadd -> "add"
  | Osub -> "sub"
  | Omul -> "mul"
  | Oor -> "or"
  | Oshl -> "shl"
  | Oshr -> "shr"

let show_op (k, o) =
  show_op_base o ^
  (match k with
   | Kw -> "w"
   | Kl -> "l"
   | Ks -> "s"
   | Kd -> "d")

let rec show_pattern p =
  match p with
  | Atm Tmp -> "%"
  | Atm AnyCon -> "$"
  | Atm (Con n) -> Int64.to_string n
  | Var (v, p) ->
      show_pattern (Atm p) ^ "'" ^ v
  | Bnr (o, pl, pr) ->
      "(" ^ show_op o ^
      " " ^ show_pattern pl ^
      " " ^ show_pattern pr ^ ")"

let get_atomic p =
  match p with
  | (Atm a | Var (_, a)) -> Some a
  | _ -> None

let rec pattern_match p w =
  match p with
  | Var (_, p) ->
      pattern_match (Atm p) w
  | Atm Tmp ->
      begin match get_atomic w with
      | Some (Con _ | AnyCon) -> false
      | _ -> true
      end
  | Atm (Con _) -> w = p
  | Atm (AnyCon) ->
      not (pattern_match (Atm Tmp) w)
  | Bnr (o, pl, pr) ->
      begin match w with
      | Bnr (o', wl, wr) ->
          o' = o &&
          pattern_match pl wl &&
          pattern_match pr wr
      | _ -> false
      end

type +'a cursor = (* a position inside a pattern *)
  | Bnrl of op * 'a cursor * pattern
  | Bnrr of op * pattern * 'a cursor
  | Top of 'a

let rec fold_cursor c p =
  match c with
  | Bnrl (o, c', p') -> fold_cursor c' (Bnr (o, p, p'))
  | Bnrr (o, p', c') -> fold_cursor c' (Bnr (o, p', p))
  | Top _ -> p

let peel p x =
  let once out (p, c) =
    match p with
    | Var (_, p) -> (Atm p, c) :: out
    | Atm _ -> (p, c) :: out
    | Bnr (o, pl, pr) ->
        (pl, Bnrl (o, c, pr)) ::
        (pr, Bnrr (o, pl, c)) :: out
  in
  let rec go l =
    let l' = List.fold_left once [] l in
    if List.length l' = List.length l
    then l'
    else go l'
  in go [(p, Top x)]

let fold_pairs l1 l2 ini f =
  let rec go acc = function
    | [] -> acc
    | a :: l1' ->
        go (List.fold_left
          (fun acc b -> f (a, b) acc)
          acc l2) l1'
  in go ini l1

let iter_pairs l f =
  fold_pairs l l () (fun x () -> f x)

let inverse l =
  List.map (fun (a, b) -> (b, a)) l

type 'a state =
  { id: int
  ; seen: pattern
  ; point: ('a cursor) list }

let rec binops side {point; _} =
  List.filter_map (fun c ->
      match c, side with
      | Bnrl (o, c, r), `L -> Some ((o, c), r)
      | Bnrr (o, l, c), `R -> Some ((o, c), l)
      | _ -> None)
    point

let group_by_fst l =
  List.fast_sort (fun (a, _) (b, _) ->
    compare a b) l |>
  List.fold_left (fun (oo, l, res) (o', c) ->
      match oo with
      | None -> (Some o', [c], [])
      | Some o when o = o' -> (oo, c :: l, res)
      | Some o -> (Some o', [c], (o, l) :: res))
    (None, [], []) |>
  (function
    | (None, _, _) -> []
    | (Some o, l, res) -> (o, l) :: res)

let sort_uniq cmp l =
  List.fast_sort cmp l |>
  List.fold_left (fun (eo, l) e' ->
      match eo with
      | None -> (Some e', l)
      | Some e when cmp e e' = 0 -> (eo, l)
      | Some e -> (Some e', e :: l))
    (None, []) |>
  (function
    | (None, _) -> []
    | (Some e, l) -> List.rev (e :: l))

let setify l =
  sort_uniq compare l

let normalize (point: ('a cursor) list) =
  setify point

let next_binary tmp s1 s2 =
  let pm w (_, p) = pattern_match p w in
  let o1 = binops `L s1 |>
           List.filter (pm s2.seen) |>
           List.map fst in
  let o2 = binops `R s2 |>
           List.filter (pm s1.seen) |>
           List.map fst in
  List.map (fun (o, l) ->
      o,
      { id = -1
      ; seen = Bnr (o, s1.seen, s2.seen)
      ; point = normalize (l @ tmp) })
    (group_by_fst (o1 @ o2))

type p = string

module StateSet : sig
  type t
  val create: unit -> t
  val add: t -> p state ->
           [> `Added | `Found ] * p state
  val iter: t -> (p state -> unit) -> unit
  val elems: t -> (p state) list
end = struct
  open Hashtbl.Make(struct
    type t = p state
    let equal s1 s2 = s1.point = s2.point
    let hash s = Hashtbl.hash s.point
  end)
  type nonrec t =
    { h: int t
    ; mutable next_id: int }
  let create () =
    { h = create 500; next_id = 0 }
  let add set s =
    assert (s.point = normalize s.point);
    try
      let id = find set.h s in
      `Found, {s with id}
    with Not_found -> begin
      let id = set.next_id in
      set.next_id <- id + 1;
      add set.h s id;
      `Added, {s with id}
    end
  let iter set f =
    let f s id = f {s with id} in
    iter f set.h
  let elems set =
    let res = ref [] in
    iter set (fun s -> res := s :: !res);
    !res
end

type table_key =
  | K of op * p state * p state

module StateMap = struct
  include Map.Make(struct
      type t = table_key
      let compare ka kb =
        match ka, kb with
        | K (o, sl, sr), K (o', sl', sr') ->
            compare (o, sl.id, sr.id)
                    (o', sl'.id, sr'.id)
    end)
  let invert n sm =
    let rmap = Array.make n [] in
    iter (fun k {id; _} ->
        match k with
        | K (o, sl, sr) ->
            rmap.(id) <-
              (o, (sl.id, sr.id)) :: rmap.(id)
      ) sm;
    Array.map group_by_fst rmap
  let by_ops sm =
    fold (fun tk s ops ->
        match tk with
        | K (op, l, r) ->
            (op, ((l.id, r.id), s.id)) :: ops)
      sm [] |> group_by_fst
end

type rule =
  { name: string
  ; vars: string list
  ; pattern: pattern }

let generate_table rl =
  let states = StateSet.create () in
  let rl =
    (* these atomic patterns must occur in
     * rules so that we are able to number
     * all possible refs *)
    [ { name = "$"; vars = []
      ; pattern = Atm AnyCon }
    ; { name = "%"; vars = []
      ; pattern = Atm Tmp } ] @ rl
  in
  (* initialize states *)
  let ground =
    List.concat_map
      (fun r -> peel r.pattern r.name) rl |>
    group_by_fst
  in
  let tmp = List.assoc (Atm Tmp) ground in
  let con = List.assoc (Atm AnyCon) ground in
  let atoms = ref [] in
  let () =
    List.iter (fun (seen, l) ->
      let point =
        if pattern_match (Atm Tmp) seen
        then normalize (tmp @ l)
        else normalize (con @ l)
      in
      let s = {id = -1; seen; point} in
      let _, s = StateSet.add states s in
      match get_atomic seen with
      | Some atm -> atoms := (atm, s) :: !atoms
      | None -> ()
    ) ground
  in
  (* setup loop state *)
  let map = ref StateMap.empty in
  let map_add k s' =
    map := StateMap.add k s' !map
  in
  let flag = ref `Added in
  let flagmerge = function
    | `Added -> flag := `Added
    | _ -> ()
  in
  (* iterate until fixpoint *)
  while !flag = `Added do
    flag := `Stop;
    let statel = StateSet.elems states in
    iter_pairs statel (fun (sl, sr) ->
      next_binary tmp sl sr |>
      List.iter (fun (o, s') ->
        let flag', s' =
          StateSet.add states s' in
        flagmerge flag';
        map_add (K (o, sl, sr)) s';
    ));
  done;
  let states =
    StateSet.elems states |>
    List.sort (fun s s' -> compare s.id s'.id) |>
    Array.of_list
  in
  (states, !atoms, !map)

let intersperse x l =
  let rec go left right out =
    let out =
      (List.rev left @ [x] @ right) ::
      out in
    match right with
    | x :: right' ->
        go (x :: left) right' out
    | [] -> out
  in go [] l []

let rec permute = function
  | [] -> [[]]
  | x :: l ->
      List.concat (List.map
        (intersperse x) (permute l))

(* build all binary trees with ordered
 * leaves l *)
let rec bins build l =
  let rec go l r out =
    match r with
    | [] -> out
    | x :: r' ->
        go (l @ [x]) r'
          (fold_pairs
            (bins build l)
            (bins build r)
            out (fun (l, r) out ->
                   build l r :: out))
  in
  match l with
  | [] -> []
  | [x] -> [x]
  | x :: l -> go [x] l []

let products l ini f =
  let rec go acc la = function
    | [] -> f (List.rev la) acc
    | xs :: l ->
        List.fold_left (fun acc x ->
            go acc (x :: la) l)
          acc xs
  in go ini [] l

(* combinatorial nuke... *)
let rec ac_equiv =
  let rec alevel o = function
    | Bnr (o', l, r) when o' = o ->
        alevel o l @ alevel o r
    | x -> [x]
  in function
  | Bnr (o, _, _) as p
    when associative o ->
      products
        (List.map ac_equiv (alevel o p)) []
        (fun choice out ->
          List.concat_map
            (bins (fun l r -> Bnr (o, l, r)))
            (if commutative o
              then permute choice
              else [choice]) @ out)
  | Bnr (o, l, r)
    when commutative o ->
      fold_pairs
        (ac_equiv l) (ac_equiv r) []
        (fun (l, r) out ->
          Bnr (o, l, r) ::
          Bnr (o, r, l) :: out)
  | Bnr (o, l, r) ->
      fold_pairs
        (ac_equiv l) (ac_equiv r) []
        (fun (l, r) out ->
          Bnr (o, l, r) :: out)
  | x -> [x]

module Action: sig
  type node =
    | Switch of (int * t) list
    | Push of bool * t
    | Pop of t
    | Set of string * t
    | Stop
  and t = private
    { id: int; node: node }
  val equal: t -> t -> bool
  val size: t -> int
  val stop: t
  val mk_push: sym:bool -> t -> t
  val mk_pop: t -> t
  val mk_set: string -> t -> t
  val mk_switch: int list -> (int -> t) -> t
  val pp: Format.formatter -> t -> unit
end = struct
  type node =
    | Switch of (int * t) list
    | Push of bool * t
    | Pop of t
    | Set of string * t
    | Stop
  and t =
    { id: int; node: node }

  let equal a a' = a.id = a'.id
  let size a =
    let seen = Hashtbl.create 10 in
    let rec node_size = function
      | Switch l ->
          List.fold_left
            (fun n (_, a) -> n + size a) 0 l
      | (Push (_, a) | Pop a | Set (_, a)) ->
          size a
      | Stop -> 0
    and size {id; node} =
      if Hashtbl.mem seen id
      then 0
      else begin
        Hashtbl.add seen id ();
        1 + node_size node
      end
    in
    size a

  let mk =
    let hcons = Hashtbl.create 100 in
    let fresh = ref 0 in
    fun node ->
      let id =
        try Hashtbl.find hcons node
        with Not_found ->
          let id = !fresh in
          Hashtbl.add hcons node id;
          fresh := id + 1;
          id
      in
      {id; node}
  let stop = mk Stop
  let mk_push ~sym a = mk (Push (sym, a))
  let mk_pop a =
    match a.node with
    | Stop -> a
    | _ -> mk (Pop a)
  let mk_set v a = mk (Set (v, a))
  let mk_switch ids f =
    match List.map f ids with
    | [] -> failwith "empty switch";
    | c :: cs as cases ->
        if List.for_all (equal c) cs then c
        else
          let cases = List.combine ids cases in
          mk (Switch cases)

  open Format
  let rec pp_node fmt = function
    | Switch l ->
        fprintf fmt "@[<v>@[<v2>switch{";
        let pp_case (c, a) =
          let pp_sep fmt () = fprintf fmt "," in
          fprintf fmt "@,@[<2>→%a:@ @[%a@]@]"
            (pp_print_list ~pp_sep pp_print_int)
            c pp a
        in
        inverse l |> group_by_fst |> inverse |>
          List.iter pp_case;
        fprintf fmt "@]@,}@]"
    | Push (true, a) -> fprintf fmt "pushsym@ %a" pp a
    | Push (false, a) -> fprintf fmt "push@ %a" pp a
    | Pop a -> fprintf fmt "pop@ %a" pp a
    | Set (v, a) -> fprintf fmt "set(%s)@ %a" v pp a
    | Stop -> fprintf fmt "•"
  and pp fmt a = pp_node fmt a.node
end

(* a state is commutative if (a op b) enters
 * it iff (b op a) enters it as well *)
let symmetric rmap id =
  List.for_all (fun (_, l) ->
      let l1, l2 =
        List.filter (fun (a, b) -> a <> b) l |>
        List.partition (fun (a, b) -> a < b)
      in
      setify l1 = setify (inverse l2))
    rmap.(id)

(* left-to-right matching of a set of patterns;
 * may raise if there is no lr matcher for the
 * input rule *)
let lr_matcher statemap states rules name =
  let rmap =
    let nstates = Array.length states in
    StateMap.invert nstates statemap
  in
  let exception Stuck in
  (* the list of ids represents a class of terms
   * whose root ends up being labelled with one
   * such id; the gen function generates a matcher
   * that will, given any such term, assign values
   * for the Var nodes of one pattern in pats *)
  let rec gen
  : 'a. int list -> (pattern * 'a) list
        -> (int -> (pattern * 'a) list -> Action.t)
        -> Action.t
  = fun ids pats k ->
    Action.mk_switch (setify ids) @@ fun id_top ->
    let sym = symmetric rmap id_top in
    let id_ops =
      if sym then
        let ordered (a, b) = a <= b in
        List.map (fun (o, l) ->
            (o, List.filter ordered l))
          rmap.(id_top)
      else rmap.(id_top)
    in
    (* consider only the patterns that are
     * compatible with the current id *)
    let atm_pats, bin_pats =
      List.filter (function
          | Bnr (o, _, _), _ ->
              List.exists
                (fun (o', _) -> o' = o)
                id_ops
          | _ -> true) pats |>
      List.partition
        (fun (pat, _) -> is_atomic pat)
    in
    try
      if bin_pats = [] then raise Stuck;
      let pats_l =
        List.map (function
            | (Bnr (o, l, r), x) ->
                (l, (o, x, r))
            | _ -> assert false)
          bin_pats
      and pats_r =
        List.map (fun (l, (o, x, r)) ->
            (r, (o, l, x)))
      and patstop =
        List.map (fun (r, (o, l, x)) ->
            (Bnr (o, l, r), x))
      in
      let id_pairs = List.concat_map snd id_ops in
      let ids_l = List.map fst id_pairs
      and ids_r id_left =
        List.filter_map (fun (l, r) ->
            if l = id_left then Some r else None)
          id_pairs
      in
      (* match the left arm *)
      Action.mk_push ~sym
        (gen ids_l pats_l
         @@ fun lid pats ->
         (* then the right arm, considering
          * only the remaining possible
          * patterns and knowing that the
          * left arm was numbered 'lid' *)
          Action.mk_pop
            (gen (ids_r lid) (pats_r pats)
             @@ fun _rid pats ->
             (* continue with the parent *)
             k id_top (patstop pats)))
    with Stuck ->
      let atm_pats =
        let seen = states.(id_top).seen in
        List.filter (fun (pat, _) ->
            pattern_match pat seen) atm_pats
      in
      if atm_pats = [] then raise Stuck else
      let vars =
        List.filter_map (function
            | (Var (v, _), _) -> Some v
            | _ -> None) atm_pats |> setify
      in
      match vars with
      | [] -> k id_top atm_pats
      | [v] -> Action.mk_set v (k id_top atm_pats)
      | _ -> failwith "ambiguous var match"
  in
  (* generate a matcher for the rule *)
  let ids_top =
    Array.to_list states |>
    List.filter_map (fun {id; point = p; _} ->
        if List.exists ((=) (Top name)) p then
          Some id
        else None)
  in
  let rec filter_dups pats =
    match pats with
    | p :: pats ->
        if List.exists (pattern_match p) pats
        then filter_dups pats
        else p :: filter_dups pats
    | [] -> []
  in
  let pats_top =
    List.filter_map (fun r ->
        if r.name = name then
          Some r.pattern
        else None) rules |>
    filter_dups |>
    List.map (fun p -> (p, ()))
  in
  gen ids_top pats_top (fun _ pats ->
      assert (pats <> []);
      Action.stop)

type numberer =
  { atoms: (atomic_pattern * p state) list
  ; statemap: p state StateMap.t
  ; states: p state array
  ; mutable ops: op list
    (* memoizes the list of possible operations
     * according to the statemap *) }

let make_numberer sa am sm =
  { atoms = am
  ; states = sa
  ; statemap = sm
  ; ops = [] }

let atom_state n atm =
  List.assoc atm n.atoms
