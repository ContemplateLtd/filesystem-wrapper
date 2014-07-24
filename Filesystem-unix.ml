(* Invariant: pathnames always starts with '/'. It is intended to
   represent an absolute pathname. *)
type pathname = string

let pathname_of_relative_path relative_path =
  if String.length relative_path = 0 then
    Unix.getcwd ()
  else if relative_path.[0] = '/' then
    relative_path
  else
    let cwd = Unix.getcwd () in
    cwd ^ "/" ^ relative_path

let append pathname relative_path =
  Filename.concat pathname relative_path

let string_of_pathname pathname =
  pathname

let file_exists pathname =
  Sys.file_exists pathname

let openfile pathname open_flags file_perms =
  Unix.openfile pathname open_flags file_perms

let is_directory pathname =
  Sys.is_directory pathname

type dir_handle = Unix.dir_handle

let opendir pathname =
  Unix.opendir pathname

let readdir handle =
  Unix.readdir handle

let closedir handle =
  Unix.closedir handle
