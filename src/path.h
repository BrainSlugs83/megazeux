/* MegaZeux
 *
 * Copyright (C) 2007 Alistair John Strachan <alistair@devzero.co.uk>
 * Copyright (C) 2018 Alice Rowan <petrifiedrowan@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __PATH_H
#define __PATH_H

#include "compat.h"

__M_BEGIN_DECLS

#include <stdio.h>
#include <sys/stat.h>

UTILS_LIBSPEC FILE *fopen_unsafe(const char *path, const char *mode);
UTILS_LIBSPEC char *mzx_getcwd(char *buf, size_t size);
UTILS_LIBSPEC int mzx_chdir(const char *path);
UTILS_LIBSPEC int mzx_mkdir(const char *path, int mode);
UTILS_LIBSPEC int mzx_unlink(const char *path);
UTILS_LIBSPEC int mzx_rmdir(const char *path);
UTILS_LIBSPEC int mzx_stat(const char *path, struct stat *buf);

// Replace standard versions of above functions with our wrappers.
#define getcwd(buf, size) mzx_getcwd(buf, size)
#define chdir(path) mzx_chdir(path)
#define mkdir(path, mode) mzx_mkdir(path, mode)
#define unlink(path) mzx_unlink(path)
#define rmdir(path) mzx_rmdir(path)
#define stat(path, buf) mzx_stat(path, buf)

// fopen() gets special treatment since we distinguish "safe" and
// "unsafe" versions of it.

#ifdef __GNUC__

// On GNU compilers we can mark fopen() as "deprecated" to remind callers
// to properly audit calls to it.
//
// Any case where a file to open is specified by a game, the path must first
// be translated, so that any case-insensitive matches will be used and
// safety checks will be applied. Callers should use either
// fsafetranslate()+fopen_unsafe(), or call fsafeopen() directly.
//
// Any case where a file comes from the editor, or from MZX-proper, it
// probably should not be translated. In this case fopen_unsafe() can be
// used directly.

static inline FILE *check_fopen(const char *path, const char *mode)
 __attribute__((deprecated));
static inline FILE *check_fopen(const char *path, const char *mode)
 { return fopen_unsafe(path, mode); }

#define fopen(path, mode) check_fopen(path, mode)

#else // !__GNUC__

#define fopen(path, mode) fopen_unsafe(path, mode)

#endif // __GNUC__

#define PATH_BUF_LEN MAX_PATH

enum mzx_dir_type
{
  DIR_TYPE_UNKNOWN,
  DIR_TYPE_FILE,
  DIR_TYPE_DIR
};

struct mzx_dir
{
#if defined(CONFIG_PSP) || defined(CONFIG_3DS)
  char path[PATH_BUF_LEN];
#endif
  void *opaque;
  long entries;
  long pos;
#ifdef __WIN32__
  boolean is_wdirent;
#endif
};

boolean dir_open(struct mzx_dir *dir, const char *path);
void dir_close(struct mzx_dir *dir);
void dir_seek(struct mzx_dir *dir, long offset);
long dir_tell(struct mzx_dir *dir);
boolean dir_get_next_entry(struct mzx_dir *dir, char *entry, int *type);

__M_END_DECLS

#endif // __PATH_H
