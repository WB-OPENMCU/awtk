/*-------------------------------------------------------------------------
 *
 * dirent.c
 *	  opendir/readdir/closedir for win32/msvc
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/port/dirent.c
 *
 *-------------------------------------------------------------------------
 */

#define DT_DIR 1
#define DT_REG 2

#include "tkc/types_def.h"

#ifdef __GNUC__

#include "errno.h"

#endif

struct dirent {
  long d_ino;
  uint8_t d_type;
  unsigned short d_reclen;
  unsigned short d_namlen;
  char d_name[MAX_PATH];
};

typedef struct DIR DIR;

DIR* opendir(const char*);
struct dirent* readdir(DIR*);
int closedir(DIR*);

struct DIR {
  char* dirname;
  struct dirent ret; /* Used to return to caller */
  HANDLE handle;
};

DIR* opendir(const char* dirname) {
  DWORD attr;
  DIR* d;

  /* Make sure it is a directory */
  attr = GetFileAttributes(dirname);
  if (attr == INVALID_FILE_ATTRIBUTES) {
    errno = ENOENT;
    return NULL;
  }
  if ((attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
    errno = ENOTDIR;
    return NULL;
  }

  d = malloc(sizeof(DIR));
  if (!d) {
    errno = ENOMEM;
    return NULL;
  }
  d->dirname = malloc(strlen(dirname) + 4);
  if (!d->dirname) {
    errno = ENOMEM;
    free(d);
    return NULL;
  }
  strcpy(d->dirname, dirname);
  if (d->dirname[strlen(d->dirname) - 1] != '/' && d->dirname[strlen(d->dirname) - 1] != '\\')
    strcat(d->dirname, "\\"); /* Append backslash if not already
                               * there */
  strcat(d->dirname, "*");    /* Search for entries named anything */
  d->handle = INVALID_HANDLE_VALUE;
  d->ret.d_ino = 0;    /* no inodes on win32 */
  d->ret.d_reclen = 0; /* not used on win32 */

  return d;
}

void rewinddir(DIR* d) {
}

struct dirent* readdir(DIR* d) {
  WIN32_FIND_DATA fd;

  if (d->handle == INVALID_HANDLE_VALUE) {
    d->handle = FindFirstFile(d->dirname, &fd);
    if (d->handle == INVALID_HANDLE_VALUE) {
      errno = ENOENT;
      return NULL;
    }
  } else {
    if (!FindNextFile(d->handle, &fd)) {
      if (GetLastError() == ERROR_NO_MORE_FILES) {
        /* No more files, force errno=0 (unlike mingw) */
        errno = 0;
        return NULL;
      }
      return NULL;
    }
  }
  strcpy(d->ret.d_name, fd.cFileName); /* Both strings are MAX_PATH
                                        * long */
  d->ret.d_namlen = strlen(d->ret.d_name);

  d->ret.d_type = 0;
  if (fd.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) {
    d->ret.d_type |= DT_REG;
  }

  if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    d->ret.d_type |= DT_DIR;
  }

  return &d->ret;
}

int closedir(DIR* d) {
  int ret = 0;

  if (d->handle != INVALID_HANDLE_VALUE) ret = !FindClose(d->handle);
  free(d->dirname);
  free(d);

  return ret;
}