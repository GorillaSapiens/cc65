////////////////////////////////////////////////////////////////////////////////
//
//                               searchpath.h
//
//                         Handling of search paths
//
//
//
// (C) 2000-2013, Ullrich von Bassewitz
//                Roemerstrasse 52
//                D-70794 Filderstadt
// EMail:         uz@cc65.org
//
//
// This software is provided 'as-is', without any expressed or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source
//    distribution.
//
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#if defined(_MSC_VER)
// Microsoft compiler
#include <io.h>
#else
// Anyone else
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#include "cmdline.h"
#endif

// common
#include "coll.h"
#include "searchpath.h"
#include "strbuf.h"
#include "xmalloc.h"

////////////////////////////////////////////////////////////////////////////////
//                                   Code
////////////////////////////////////////////////////////////////////////////////

static char *CleanupPath(const char *Path)
// Prepare and return a clean copy of Path
{
   unsigned Len;
   char *NewPath;

   // Get the length of the path
   Len = strlen(Path);

   // Check for a trailing path separator and remove it
   if (Len > 0 && (Path[Len - 1] == '\\' || Path[Len - 1] == '/')) {
      --Len;
   }

   // Allocate memory for the new string
   NewPath = (char *)xmalloc(Len + 1);

   // Copy the path and terminate it, then return the copy
   memcpy(NewPath, Path, Len);
   NewPath[Len] = '\0';
   return NewPath;
}

static void Add(SearchPaths *P, const char *New)
// Cleanup a new search path and add it to the list
{
   // Add a clean copy of the path to the collection
   CollAppend(P, CleanupPath(New));
}

SearchPaths *NewSearchPath(void)
// Create a new, empty search path list
{
   return NewCollection();
}

void AddSearchPath(SearchPaths *P, const char *NewPath)
// Add a new search path to the end of an existing list
{
   // Allow a NULL path
   if (NewPath) {
      Add(P, NewPath);
   }
}

void AddSearchPathFromEnv(SearchPaths *P, const char *EnvVar)
// Add a search path from an environment variable to the end of an existing
// list.
{
   AddSearchPath(P, getenv(EnvVar));
}

void AddSubSearchPathFromEnv(SearchPaths *P, const char *EnvVar,
                             const char *SubDir)
// Add a search path from an environment variable, adding a subdirectory to
// the environment variable value.
{
   StrBuf Dir = AUTO_STRBUF_INITIALIZER;

   const char *EnvVal = getenv(EnvVar);
   if (EnvVal == 0) {
      // Not found
      return;
   }

   // Copy the environment variable to the buffer
   SB_CopyStr(&Dir, EnvVal);

   // Add a path separator if necessary
   if (SB_NotEmpty(&Dir)) {
      if (SB_LookAtLast(&Dir) != '\\' && SB_LookAtLast(&Dir) != '/') {
         SB_AppendChar(&Dir, '/');
      }
   }

   // Add the subdirectory and terminate the string
   SB_AppendStr(&Dir, SubDir);
   SB_Terminate(&Dir);

   // Add the search path
   AddSearchPath(P, SB_GetConstBuf(&Dir));

   // Free the temp buffer
   SB_Done(&Dir);
}

#ifdef _WIN32
#define PATHSEP "\\"
#undef PATH_MAX
#define PATH_MAX _MAX_PATH
#else
#define PATHSEP "/"

//
// on POSIX-compatible operating system, a binary can be started in
// 3 distinct ways:
//
// 1) using absolute path; in which case argv[0] starts with '/'
// 2) using relative path; in which case argv[0] contains '/', but
// does not start with it. e.g.: ./ca65 ; bin/cc65
// 3) using PATH environment variable, which is a colon-separated
// list of directories which will be searched for a command
// name used unprefixed. see execlp() and execvp() in man 3p exec:
//
// >  The argument file is used to construct a pathname that identifies the new
// >  process  image  file.  If the file argument contains a <slash> character,
// >  the  file  argument  shall  be  used  as  the  pathname  for  this  file.
// >  Otherwise,  the  path prefix for this file is obtained by a search of the
// >  directories passed  as  the  environment  variable  PATH  (see  the  Base
// >  Definitions  volume  of  POSIX.1*2008, Chapter 8, Environment Variables).
// >  If this environment variable is not present, the results  of  the  search
// >  are implementation-defined.
//

static int SearchPathBin(const char *bin, char *buf, size_t buflen)
// search colon-separated list of paths in PATH environment variable
// for the full path of argv[0].
// returns 1 if successfull, in which case buf will contain the full path.
// bin = binary name (from argv[0]), buf = work buffer, buflen = sizeof buf
{
   char *p = getenv("PATH");
   char *o;
   size_t l;

   if (!p) {
      return 0;
   }
   for (;;) {
      o = buf;
      l = buflen;
      while (l && *p && *p != ':') {
         *(o++) = *(p++);
         l--;
      }
      snprintf(o, l, "/%s", bin);
      if (access(buf, X_OK) == 0) {
         return 1;
      }
      if (*p == ':') {
         p++;
      }
      else if (!p) {
         break;
      }
   }
   return 0;
}

static char *GetProgPath(char *pathbuf, char *a0)
// search for the full path of the binary using the argv[0] parameter
// passed to int main(), according to the description above.
//
// the binary name will be passed to realpath(3p) to have all symlinks
// resolved and gratuitous path components like "../" removed.
//
// argument "pathbuf" is a work buffer of size PATH_MAX,
// "a0" the original argv[0].
// returns pathbuf with the full path of the binary.
{
   char tmp[PATH_MAX];

   if (!strchr(a0, '/')) {
      // path doesn't contain directory separator, so it was looked up
      // via PATH environment variable
      SearchPathBin(a0, tmp, PATH_MAX);
      a0 = tmp;
   }

   // realpath returns the work buffer passed to it, so checking the
   // return value is superfluous. gcc11 warns anyway.
   if (realpath(a0, pathbuf)) {
   }

   return pathbuf;
}

#endif

void AddSubSearchPathFromBin(SearchPaths *P, const char *SubDir)
// Add a search path from the running binary, adding a subdirectory to
// the parent directory of the directory containing the binary.
//
// currently this will work on POSIX systems and on Windows. Should
// we run into build errors on systems that are neither, we must add
// another exception below.
{
   char *Ptr;
   char Dir[PATH_MAX];

#if defined(_WIN32)

   if (GetModuleFileName(NULL, Dir, _MAX_PATH) == 0) {
      return;
   }

#else // POSIX

   GetProgPath(Dir, ArgVec[0]);

#endif

   // Remove binary name
   Ptr = strrchr(Dir, PATHSEP[0]);
   if (Ptr == 0) {
      return;
   }
   *Ptr = '\0';

   // Check for 'bin' directory
   Ptr = strrchr(Dir, PATHSEP[0]);
   if (Ptr == 0) {
      return;
   }
   if (strcmp(Ptr++, PATHSEP "bin") != 0) {
      return;
   }

   // Append SubDir
   strcpy(Ptr, SubDir);

   // Add the search path
   AddSearchPath(P, Dir);
}

int PushSearchPath(SearchPaths *P, const char *NewPath)
// Add a new search path to the head of an existing search path list, provided
// that it's not already there. If the path is already at the first position,
// return zero, otherwise return a non zero value.
{
   // Generate a clean copy of NewPath
   char *Path = CleanupPath(NewPath);

   // If we have paths, check if Path is already at position zero
   if (CollCount(P) > 0 && strcmp(CollConstAt(P, 0), Path) == 0) {
      // Match. Delete the copy and return to the caller
      xfree(Path);
      return 0;
   }

   // Insert a clean copy of the path at position 0, return success
   CollInsert(P, Path, 0);
   return 1;
}

void PopSearchPath(SearchPaths *P)
// Remove a search path from the head of an existing search path list
{
   // Remove the path at position 0
   xfree(CollAt(P, 0));
   CollDelete(P, 0);
}

char *GetSearchPath(SearchPaths *P, unsigned Index)
// Return the search path at the given index, if the index is valid, return an
// empty string otherwise.
{
   if (Index < CollCount(P))
      return CollAtUnchecked(P, Index);
   return "";
}

char *SearchFile(const SearchPaths *P, const char *File)
// Search for a file in a list of directories. Return a pointer to a malloced
// area that contains the complete path, if found, return 0 otherwise.
{
   char *Name = 0;
   StrBuf PathName = AUTO_STRBUF_INITIALIZER;

   // Start the search
   unsigned I;
   for (I = 0; I < CollCount(P); ++I) {

      // Copy the next path element into the buffer
      SB_CopyStr(&PathName, CollConstAt(P, I));

      // Add a path separator and the filename
      if (SB_NotEmpty(&PathName)) {
         SB_AppendChar(&PathName, '/');
      }
      SB_AppendStr(&PathName, File);
      SB_Terminate(&PathName);

      // Check if this file exists
      if (access(SB_GetBuf(&PathName), 0) == 0) {
         // The file exists, we're done
         Name = xstrdup(SB_GetBuf(&PathName));
         break;
      }
   }

   // Cleanup and return the result of the search
   SB_Done(&PathName);
   return Name;
}
