open Cgen
open Match

let mgen ~verbose ~fuzz path lofs input oc =
  let info ?(level = 1) fmt =
    if level <= verbose then
      Printf.eprintf fmt
    else
      Printf.ifprintf stdout fmt
  in

  let rules =
    match Sexp.(run_parser ppats) input with
    | `Error (ps, err, loc) ->
        Printf.eprintf "%s:%d:%d %s\n"
          path (lofs + ps.Sexp.line) ps.Sexp.coln err;
        Printf.eprintf "%s" loc;
        exit 1
    | `Ok rules -> rules
  in

  info "adding ac variants...%!";
  let nparsed =
    List.fold_left
      (fun npats (_, _, ps) ->
         npats + List.length ps)
      0 rules
  in
  let varsmap = Hashtbl.create 10 in
  let rules =
    List.concat_map (fun (name, vars, patterns) ->
        (try assert (Hashtbl.find varsmap name = vars)
         with Not_found -> ());
        Hashtbl.replace varsmap name vars;
        List.map
          (fun pattern -> {name; vars; pattern})
          (List.concat_map ac_equiv patterns)
      ) rules
  in
  info " %d -> %d patterns\n"
    nparsed (List.length rules);

  let rnames =
    setify (List.map (fun r -> r.name) rules) in

  info "generating match tables...%!";
  let sa, am, sm = generate_table rules in
  let numbr = make_numberer sa am sm in
  info " %d states, %d rules\n"
    (Array.length sa) (StateMap.cardinal sm);
  if verbose >= 2 then begin
    info "-------------\nstates:\n";
    Array.iteri (fun i s ->
        info "  state %d: %s\n"
          i (show_pattern s.seen)) sa;
    info "-------------\nstatemap:\n";
    Test.print_sm stderr sm;
    info "-------------\n";
  end;

  info "generating matchers...\n";
  let matchers =
    List.map (fun rname ->
        info "+ %s...%!" rname;
        let m = lr_matcher sm sa rules rname in
        let vars = Hashtbl.find varsmap rname in
        info " %d nodes\n" (Action.size m);
        info ~level:2 "  -------------\n";
        info ~level:2 "  automaton:\n";
        info ~level:2 "%s\n"
          (Format.asprintf "    @[%a@]" Action.pp m);
        info ~level:2 "  ----------\n";
        (vars, rname, m)
      ) rnames
  in

  if fuzz then begin
    info ~level:0 "fuzzing statemap...\n";
    let tp = Fuzz.fuzz_numberer rules numbr in
    info ~level:0 "testing %d patterns...\n"
      (List.length rules);
    Fuzz.test_matchers tp numbr rules
  end;

  info "emitting C...\n";
  flush stderr;

  let cgopts =
    { pfx = ""; static = true; oc = oc } in
  emit_c cgopts numbr;
  emit_matchers cgopts matchers;

  ()

let read_all ic =
  let bufsz = 4096 in
  let buf = Bytes.create bufsz in
  let data = Buffer.create bufsz in
  let read = ref 0 in
  while
    read := input ic buf 0 bufsz;
    !read <> 0
  do
    Buffer.add_subbytes data buf 0 !read
  done;
  Buffer.contents data

let split_c src =
  let begin_re, eoc_re, end_re =
    let re = Str.regexp in
    ( re "mgen generated code"
    , re "\\*/"
    , re "end of generated code" )
  in
  let str_match regexp str =
    try
      let _: int =
        Str.search_forward regexp str 0
      in true
    with Not_found -> false
  in

  let rec go st lofs pfx rules lines =
    let line, lines =
      match lines with
      | [] ->
          failwith (
            match st with
            | `Prefix -> "could not find mgen section"
            | `Rules -> "mgen rules not terminated"
            | `Skip -> "mgen section not terminated"
          )
      | l :: ls -> (l, ls)
    in
    match st with
    | `Prefix ->
        let pfx = line :: pfx in
        if str_match begin_re line
        then
          let lofs = List.length pfx in
          go `Rules lofs pfx rules lines
        else go `Prefix 0 pfx rules lines
    | `Rules ->
        let pfx = line :: pfx in
        if str_match eoc_re line
        then go `Skip lofs pfx rules lines
        else go `Rules lofs pfx (line :: rules) lines
    | `Skip ->
        if str_match end_re line then
          let join = String.concat "\n" in
          let pfx = join (List.rev pfx) ^ "\n\n"
          and rules = join (List.rev rules)
          and sfx = join (line :: lines)
          in (lofs, pfx, rules, sfx)
        else go `Skip lofs pfx rules lines
  in

  let lines = String.split_on_char '\n' src in
  go `Prefix 0 [] [] lines

let () =
  let usage_msg =
    "mgen [--fuzz] [--verbose <N>] <file>" in

  let fuzz_arg = ref false in
  let verbose_arg = ref 0 in
  let input_paths = ref [] in

  let anon_fun filename =
    input_paths := filename :: !input_paths in

  let speclist =
    [ ( "--fuzz", Arg.Set fuzz_arg
      , "  Fuzz tables and matchers" )
    ; ( "--verbose", Arg.Set_int verbose_arg
      , "<N>  Set verbosity level" )
    ; ( "--", Arg.Rest_all (List.iter anon_fun)
      , "  Stop argument parsing" ) ]
  in
  Arg.parse speclist anon_fun usage_msg;

  let input_paths = !input_paths in
  let verbose = !verbose_arg in
  let fuzz = !fuzz_arg in
  let input_path, input =
    match input_paths with
    | ["-"] -> ("-", read_all stdin)
    | [path] -> (path, read_all (open_in path))
    | _ ->
        Printf.eprintf
          "%s: single input file expected\n"
          Sys.argv.(0);
        Arg.usage speclist usage_msg; exit 1
  in
  let mgen = mgen ~verbose ~fuzz in

  if Str.last_chars input_path 2 <> ".c"
  then mgen input_path 0 input stdout
  else
    let tmp_path = input_path ^ ".tmp" in
    Fun.protect
      ~finally:(fun () ->
          try Sys.remove tmp_path with _ -> ())
      (fun () ->
         let lofs, pfx, rules, sfx = split_c input in
         let oc = open_out tmp_path in
         output_string oc pfx;
         mgen input_path lofs rules oc;
         output_string oc sfx;
         close_out oc;
         Sys.rename tmp_path input_path;
         ());

  ()
