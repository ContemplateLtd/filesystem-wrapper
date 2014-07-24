(* Invariant: pathnames are strings that start with a drive letter and
   ':\'. There are no backslashes in the string. *)
type pathname = string

external get_full_path_name : string -> string
  = "win_caml_GetFullPathName"

let replace_forwardslashes_with_backwardslashes str =
  let str = String.copy str in
  for i = 0 to String.length str - 1 do
    if str.[i] = '/' then str.[i] <- '\\'
  done;
  str

let pathname_of_relative_path relative_path =
  "\\\\?\\" ^ get_full_path_name (replace_forwardslashes_with_backwardslashes relative_path)

let append pathname relative_path =
  pathname ^ "\\" ^ (replace_forwardslashes_with_backwardslashes relative_path)

let string_of_pathname pathname =
  pathname

(**********************************************************************)
(* File opening *)

external file_exists : pathname -> bool = "file_exists_unicode"

external openfile : pathname -> Unix.open_flag list -> Unix.file_perm -> Unix.file_descr
  = "open_unicode"

(**********************************************************************)
(* Directories *)
external findfirst_unicode : string -> string * int = "win_findfirst_unicode"
external findnext_unicode : int -> string = "win_findnext_unicode"
external win_findclose : int -> unit = "win_findclose"
external is_directory : pathname -> bool = "isdir_unicode"

type dir_entry =
    Dir_empty
  | Dir_read of string
  | Dir_toread

type dir_handle =
  { dirname: string; mutable handle: int; mutable entry_read: dir_entry }

let opendir dirname =
  try
    let (first_entry, handle) = findfirst_unicode (dirname ^ "\\*.*") in
    { dirname = dirname; handle = handle; entry_read = Dir_read first_entry }
  with End_of_file ->
    { dirname = dirname; handle = 0; entry_read = Dir_empty }

let readdir d =
  match d.entry_read with
    Dir_empty -> raise End_of_file
  | Dir_read name -> d.entry_read <- Dir_toread; name
  | Dir_toread -> findnext_unicode d.handle

let closedir d =
  match d.entry_read with
    Dir_empty -> ()
  | _ -> win_findclose d.handle
