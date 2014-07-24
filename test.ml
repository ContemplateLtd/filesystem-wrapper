let read_file pathname =
  let s = Filesystem.string_of_pathname pathname in
  Printf.printf "## Reading (%d) %s\n" (String.length s) s;
  let fd = Filesystem.openfile pathname [Unix.O_RDONLY] 0 in
  let ch = Unix.in_channel_of_descr fd in
  let rec loop () =
    match try Some (input_line ch) with End_of_file -> None with
      | Some l -> Printf.printf "%s : %s\n" s l; loop ()
      | None   -> ()
  in
  try loop (); close_in ch
  with e -> (close_in ch; raise e)

let rec traverse dirname =
  Printf.printf "# Directory: %s\n" (Filesystem.string_of_pathname dirname);
  let handle = Filesystem.opendir dirname in
  let rec loop () =
    match try Some (Filesystem.readdir handle) with End_of_file -> None with
      | None -> ()
      | Some fname when String.length fname > 0 && fname.[0] = '.' ->
          loop ()
      | Some fname ->
          (let fullname = Filesystem.append dirname fname in
           if Filesystem.is_directory fullname then
             traverse fullname
           else
             read_file fullname);
          loop ()
  in
  try loop (); Filesystem.closedir handle
  with e -> (Filesystem.closedir handle; raise e)

let _ =
  try
    Printf.printf "Input: %s\n" Sys.argv.(1);
    let dir = Filesystem.pathname_of_relative_path Sys.argv.(1) in
    traverse dir
  with Unix.Unix_error (err, func, param) ->
    Printf.eprintf "Unix.Unix_error (%s, %s, %s)\n" (Unix.error_message err) func param
