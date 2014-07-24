(** Basic filesystem IO operations *)

(**{1 Basic filesystem IO operations}

   This module provides a common set of basic file IO operations that
   is portable between Unix-like systems and Windows. The main point
   of these operations is to provide a very long pathname support on
   Windows, to allow for pathnames longer than MAX_PATH characters
   (260 chars including the drive name and terminating NUL byte). *)

(**{2 Pathnames} *)

(** Abstract type of absolute pathnames. *)
type pathname

(** Creates a new {!pathname} value. If the supplied string is a
    relative pathname, then it is internally turned into an absolute
    pathname based on the current working directory of the process. *)
val pathname_of_relative_path : string -> pathname

(** Appends a string on to a pathname, adding a platform specific
    separator. *)
val append : pathname -> string -> pathname

(** Returns a string form of a pathname, suitable for debugging
    output. *)
val string_of_pathname : pathname -> string

(**{2 Files} *)

(** [file_exists path] returns [true] if [path] refers to something in
    the filesystem. Otherwise, returns [false]. *)
val file_exists : pathname -> bool

(** [openfile path flags perm] opens the file referenced by [path]
    with the flags [flags] and permission [perm]. See the
    {!Unix.openfile} documentation for how to interpret [flags] and
    [perm]. *)
val openfile : pathname -> Unix.open_flag list -> Unix.file_perm -> Unix.file_descr

(**{2 Directories} *)

(** [is_directory path] returns [true] if [path] references a
    directory. Otherwise, returns [false]. *)
val is_directory : pathname -> bool

(** Abstract handle representing a position in a directory's list of
    entries. *)
type dir_handle

(** Opens a directory to read its entries, and returns a handle
    pointing to the first entry. *)
val opendir : pathname -> dir_handle

(** Returns the name of the entry currently pointed to by the
    [dir_handle], and advances the pointer to the next entry. Note
    that the special ["."] and [".."] entries representing the current
    directory and the parent directory can be returned in addition to
    the actual directory entries. *)
val readdir : dir_handle -> string

(** Closes a directory handle, and frees all the accompanying
    resources. *)
val closedir : dir_handle -> unit
