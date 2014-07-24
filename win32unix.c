/***********************************************************************/
/*                                                                     */
/*                           Objective Caml                            */
/*                                                                     */
/*   Pascal Cuoq and Xavier Leroy, projet Cristal, INRIA Rocquencourt  */
/*                                                                     */
/*  Copyright 1996 Institut National de Recherche en Informatique et   */
/*  en Automatique.  All rights reserved.  This file is distributed    */
/*  under the terms of the GNU Library General Public License, with    */
/*  the special exception on linking described in file ../../LICENSE.  */
/*                                                                     */
/***********************************************************************/

/* Modified versions of win_open, win_findfirst, and win_findnext,
   plus implementations of is_directory and get_full_path_name, that
   can handle unicode (and so very long) pathnames. These functions
   convert back and forth between UTF8 (for OCaml) and UTF16 (for
   Windows).

   For information on 'very long pathname' support, see:
     http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx

   See also the following OCaml bug tracker link to build unicode
   filename support on Windows into Ocaml properly:
     http://caml.inria.fr/mantis/view.php?id=3771
*/

#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <errno.h>
#include <fcntl.h>
#include <caml/alloc.h>
#include <caml/fail.h>
#include <caml/unixsupport.h>

/******************************************************************************/
/* Conversion between utf8 and utf16 using the windows API. Note that
   these two functions assume that the input is '\0'-terminated, which
   is true for strings coming from OCaml, and for the Windows API
   functions that we use.

   MultiByteToWideChar:
     http://msdn.microsoft.com/en-us/library/windows/desktop/dd319072.aspx

   WideCharToMultiByte:
     http://msdn.microsoft.com/en-us/library/windows/desktop/dd374130%28v=vs.85%29.aspx
*/

static char*
utf16_to_utf8 (const WCHAR *s)
{
  char *buf;
  int buf_size;

  buf_size = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
  if (0 == buf_size) {
    return NULL;
  }

  buf = (char*) malloc (sizeof(char) * buf_size);
  if (NULL == buf) {
    return NULL;
  }

  if (WideCharToMultiByte(CP_UTF8, 0, s, -1, buf, buf_size, NULL, NULL) != buf_size) {
    free(buf);
    return NULL;
  }

  return buf;
}

static WCHAR*
utf8_to_utf16 (const char *s)
{
  WCHAR* buf;
  int buf_size;

  buf_size = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  if (0 == buf_size) {
    return NULL;
  }

  buf = (WCHAR*) malloc (sizeof(WCHAR) * buf_size);
  if (NULL == buf) {
    return NULL;
  }

  if (MultiByteToWideChar(CP_UTF8, 0, s, -1, buf, buf_size) != buf_size) {
    free(buf);
    return NULL;
  }

  return buf;
}

/******************************************************************************/
/* Unicode version of openfile

   CreateFile:
     http://msdn.microsoft.com/en-us/library/windows/desktop/aa363858%28v=vs.85%29.aspx
*/

static int open_access_flags[12] = {
  GENERIC_READ, GENERIC_WRITE, GENERIC_READ|GENERIC_WRITE,
  0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int open_create_flags[12] = {
  0, 0, 0, 0, 0, O_CREAT, O_TRUNC, O_EXCL, 0, 0, 0, 0
};

CAMLprim value open_unicode(value path, value flags, value perm)
{
  int fileaccess, createflags, fileattrib, filecreate;
  SECURITY_ATTRIBUTES attr;
  HANDLE h;
  char * path_utf8 = String_val(path);
  WCHAR *path_utf16;

  path_utf16 = utf8_to_utf16(path_utf8);
  if (NULL == path_utf16) {
    win32_maperr(GetLastError());
    uerror("open", path);
  }

  fileaccess = convert_flag_list(flags, open_access_flags);

  createflags = convert_flag_list(flags, open_create_flags);
  if ((createflags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
    filecreate = CREATE_NEW;
  else if ((createflags & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC))
    filecreate = CREATE_ALWAYS;
  else if (createflags & O_TRUNC)
    filecreate = TRUNCATE_EXISTING;
  else if (createflags & O_CREAT)
    filecreate = OPEN_ALWAYS;
  else
    filecreate = OPEN_EXISTING;

  if ((createflags & O_CREAT) && (Int_val(perm) & 0200) == 0)
    fileattrib = FILE_ATTRIBUTE_READONLY;
  else
    fileattrib = FILE_ATTRIBUTE_NORMAL;

  attr.nLength = sizeof(attr);
  attr.lpSecurityDescriptor = NULL;
  attr.bInheritHandle = TRUE;

  h = CreateFileW(path_utf16, fileaccess,
                  FILE_SHARE_READ | FILE_SHARE_WRITE, &attr,
                  filecreate, fileattrib, NULL);
  free(path_utf16);
  if (h == INVALID_HANDLE_VALUE) {
    win32_maperr(GetLastError());
    uerror("open", path);
  }
  return win_alloc_handle(h);
}

/******************************************************************************/
/* Unicode-capable version of is_directory

   GetFileAttributes:
     http://msdn.microsoft.com/en-us/library/windows/desktop/aa364944%28v=vs.85%29.aspx
*/

CAMLprim value isdir_unicode(value path)
{
  char *path_utf8 = String_val(path);
  WCHAR *path_utf16;
  DWORD attributes;

  path_utf16 = utf8_to_utf16(path_utf8);
  if (NULL == path_utf16) {
    win32_maperr(GetLastError());
    uerror("is_directory", path);
  }

  attributes = GetFileAttributesW(path_utf16);
  free(path_utf16);

  if (INVALID_FILE_ATTRIBUTES == attributes) {
    win32_maperr(GetLastError());
    uerror("is_directory", path);
  }

  return Val_bool((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

/******************************************************************************/
/* Unicode-capable version of file_exists.

   This works by calling GetFileAttributes on the file. If valid
   attributes are returned, then we can deduce that the file
   exists. If an error is signalled, then if the error is either about
   a non-existant file or path, then we can deduce that the file does
   not exist. Otherwise, we wrap the error and throw it.

   GetFileAttributes:
     http://msdn.microsoft.com/en-us/library/windows/desktop/aa364944%28v=vs.85%29.aspx
*/

CAMLprim value file_exists_unicode(value path)
{
  char *path_utf8 = String_val(path);
  WCHAR *path_utf16;
  DWORD attributes;
  DWORD error;

  path_utf16 = utf8_to_utf16(path_utf8);
  if (NULL == path_utf16) {
    win32_maperr(GetLastError());
    uerror("file_exists", path);
  }

  attributes = GetFileAttributesW(path_utf16);
  free(path_utf16);

  /* If we couldn't get the file's attributes because it doesn't
     exist, then return false. Otherwise, throw the appropriate
     error. */
  if (INVALID_FILE_ATTRIBUTES == attributes) {
    error = GetLastError();

    if (ERROR_FILE_NOT_FOUND == error || ERROR_PATH_NOT_FOUND == error) {
      return Val_bool(0);
    } else {
      win32_maperr(error);
      uerror("file_exists", path);
    }
  }

  /* If we got some attributes, then presumably the file exists, so
     return true */
  return Val_bool(1);
}

/******************************************************************************/
/* Unicode versions of opening and reading directories

   FindFirstFile:
     http://msdn.microsoft.com/en-us/library/windows/desktop/aa364418%28v=vs.85%29.aspx

   FindNextFile:
     http://msdn.microsoft.com/en-us/library/windows/desktop/aa364428%28v=vs.85%29.aspx
*/

CAMLprim value
win_findfirst_unicode(value name)
{
  HANDLE h;
  value v;
  value valname = Val_unit;
  value valh = Val_unit;
  WIN32_FIND_DATAW fileinfo;
  char * tempo, *temp=String_val(name);
  WCHAR * wtemp;

  wtemp = utf8_to_utf16(temp);
  if (NULL == wtemp) {
    win32_maperr(GetLastError());
    uerror("opendir", name);
  }

  Begin_roots2 (valname,valh);
  h = FindFirstFileW(wtemp,&fileinfo);
  free(wtemp);
  if (h == INVALID_HANDLE_VALUE) {
    DWORD err = GetLastError();
    if (err == ERROR_NO_MORE_FILES)
      raise_end_of_file();
    else {
      win32_maperr(err);
      uerror("opendir", name);
    }
  }

  tempo = utf16_to_utf8(fileinfo.cFileName);
  if (NULL == tempo) {
    win32_maperr (GetLastError());
    uerror("opendir", name);
  }
  valname = copy_string(tempo);
  free(tempo);
  valh = win_alloc_handle(h);
  v = alloc_small(2, 0);
  Field(v,0) = valname;
  Field(v,1) = valh;
  End_roots();

  return v;
}

CAMLprim value
win_findnext_unicode(value valh)
{
  CAMLparam0 ();
  CAMLlocal1 (v);
  WIN32_FIND_DATAW fileinfo;
  char * temp;
  BOOL retcode;

  retcode = FindNextFileW(Handle_val(valh), &fileinfo);
  if (!retcode) {
    DWORD err = GetLastError();
    if (err == ERROR_NO_MORE_FILES)
      raise_end_of_file();
    else {
      win32_maperr(err);
      uerror("readdir", Nothing);
    }
  }

  temp = utf16_to_utf8(fileinfo.cFileName);
  if (NULL == temp) {
    win32_maperr(GetLastError());
    uerror("readdir", Nothing);
  }
  v=copy_string(temp);
  free(temp);
  CAMLreturn (v);
}

/******************************************************************************/
/* GetFullPathName wrapper

   http://msdn.microsoft.com/en-us/library/windows/desktop/aa364963%28v=vs.85%29.aspx
*/

CAMLprim value
win_caml_GetFullPathName (value name)
{
  CAMLparam1(name);
  CAMLlocal1(v);
  WCHAR *input_utf16, *output_utf16;
  char *output;
  DWORD estimated_length, generated_length;

  input_utf16 = utf8_to_utf16(String_val (name));
  if (NULL == input_utf16) {
    win32_maperr(GetLastError());
    uerror("get_full_path_name(utf8_to_utf16)", name);
  }

  estimated_length = GetFullPathNameW(input_utf16, 0, NULL, NULL);
  if (0 == estimated_length) {
    free(input_utf16);
    win32_maperr(GetLastError());
    uerror("get_full_path_name(GetFullPathnameW(input_utf16, 0, NULL, NULL))", name);
  }

  output_utf16 = (WCHAR*) malloc (estimated_length * sizeof(WCHAR));
  if (NULL == output_utf16) {
    free(input_utf16);
    win32_maperr(GetLastError());
    uerror("get_full_path_name(malloc)", name);
  }

  generated_length = GetFullPathNameW(input_utf16, estimated_length, output_utf16, NULL);
  if (generated_length > (unsigned int)(estimated_length - 1)) {
    free(input_utf16);
    free(output_utf16);
    win32_maperr(GetLastError());
    uerror("get_full_path_name(GetFullPathNameW(input_utf16, length, output_utf16, NULL))", name);
  }
  free(input_utf16);

  output = utf16_to_utf8(output_utf16);
  free(output_utf16);
  if (NULL == output) {
    win32_maperr(GetLastError());
    uerror("get_full_path_name(utf16_to_utf8)", name);
  }

  v = copy_string(output);
  free(output);

  CAMLreturn(v);
}
