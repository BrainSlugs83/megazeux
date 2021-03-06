/* MegaZeux
 *
 * Copyright (C) 2020 Alice Rowan <petrifiedrowan@gmail.com>
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

#include "../Unit.hpp"
#include "../../src/io/path.c"

#include <errno.h>

/**
 * Allow tests to specify when stat should succeed or fail...
 */
static enum
{
  STAT_FAIL,
  STAT_REG,
  STAT_DIR
} stat_result_type = STAT_FAIL;
int stat_result_errno = 0;
int mkdir_result = 0;

// Custom vstat so this builds without vfile.c and can control the output.
int vstat(const char *path, struct stat *buf)
{
  memset(buf, 0, sizeof(struct stat));
  switch(stat_result_type)
  {
    default:
    case STAT_FAIL:
      errno = stat_result_errno;
      return -1;

    case STAT_REG:
      buf->st_mode = S_IFREG;
      return 0;

    case STAT_DIR:
      buf->st_mode = S_IFDIR;
      return 0;
  }
}

int vmkdir(const char *path, int mode)
{
  return mkdir_result;
}

int vaccess(const char *path, int mode)
{
  return 0;
}

struct path_ext_result
{
  const char *path;
  const char *ext;
  const char *result;
};

UNITTEST(path_force_ext)
{
  static const path_ext_result data[] =
  {
    {
      "/here/is/a/path.chr",
      ".chr",
      "/here/is/a/path.chr"
    },
    {
      "/dont/modify/this/one/either/a.palidx",
      ".palidx",
      "/dont/modify/this/one/either/a.palidx"
    },
    {
      "/Ext/Should/Be/Case/Insensitive.EXT",
      ".ext",
      "/Ext/Should/Be/Case/Insensitive.EXT"
    },
    {
      "/Ext/Should/Be/Case/Insensitive.ext",
      ".EXT",
      "/Ext/Should/Be/Case/Insensitive.ext"
    },
    {
      "/add/the/ext/to/this/one.pal",
      ".chr",
      "/add/the/ext/to/this/one.pal.chr"
    },
    {
      "/lol.mxz",
      ".mzx",
      "/lol.mxz.mzx"
    },
    {
      "/hellowwolrd",
      ".sav",
      "/hellowwolrd.sav"
    },
    {
      u8"/home/\u00C8śŚ/megazeux/DE/saved.sav",
      ".sav",
      u8"/home/\u00C8śŚ/megazeux/DE/saved.sav",
    },
    {
      u8"/home/\u00C8śŚ/megazeux/DE/saved.svaśŚ",
      ".sav",
      u8"/home/\u00C8śŚ/megazeux/DE/saved.svaśŚ.sav",
    },
  };
  // These should result in failures in the second section.
  // All of them assume a buffer size of 32.
  static const path_ext_result bad_data[] =
  {
    {
      "/an/input/path",
      ".verylongextensionwtfwhydidyoudothis",
      "/an/input/path"
    },
    {
      "/some/really/long/path/i.guess",
      ".lol",
      "/some/really/long/path/i.guess"
    },
  };
  char buffer[MAX_PATH];
  boolean result;
  int i;

  SECTION(NormalCases)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      snprintf(buffer, MAX_PATH, "%s", data[i].path);
      result = path_force_ext(buffer, MAX_PATH, data[i].ext);

      ASSERTX(result, data[i].path);
      ASSERTCMP(buffer, data[i].result);
    }
  }

  SECTION(FailureCases)
  {
    for(i = 0; i < arraysize(bad_data); i++)
    {
      snprintf(buffer, MAX_PATH, "%s", bad_data[i].path);
      result = path_force_ext(buffer, 32, bad_data[i].ext);

      ASSERTX(!result, bad_data[i].path);
      ASSERTCMP(buffer, bad_data[i].result);
    }
  }
}

struct path_output_pair
{
  const char *path;
  const char *expected;
  ssize_t return_value;
};

UNITTEST(path_get_ext_offset)
{
  static const path_output_pair data[]
  {
    {
      "",
      nullptr,
      -1
    },
    {
      "return -1 for directories/",
      nullptr,
      -1
    },
    {
      "no really return -1 for directories\\",
      nullptr,
      -1
    },
    {
      "filewithnoext",
      nullptr,
      -1
    },
    {
      "somepath.not.an.ext/filewithnoext",
      nullptr,
      -1
    },
    {
      "somepath.still.not.an.ext\\filewithnoext",
      nullptr,
      -1
    },
    {
      "yeah this really counts.",
      ".",
      23
    },
    {
      "CAVERNS.MZX",
      ".MZX",
      7
    },
    {
      "zeux/forest/FOREST.MZX",
      ".MZX",
      18
    },
    {
      "assets/glsl/scalers/softscale.frag",
      ".frag",
      29
    },
    {
      "testgame\\smzx2.palidx",
      ".palidx",
      14
    },
    {
      "/lol.mxz.mzx",
      ".mzx",
      8
    },
    {
      ".mzx",
      ".mzx",
      0
    },
    {
      u8"unicode_fileśŚ.mzx",
      ".mzx",
      16
    },
    {
      u8"unicode_pathśŚ/caverns.mzx",
      ".mzx",
      24
    },
  };
  ssize_t result;
  int i;

  for(i = 0; i < arraysize(data); i++)
  {
    result = path_get_ext_offset(data[i].path);
    ASSERTEQX(result, data[i].return_value, data[i].path);
    if(result >= 0 && data[i].expected)
      ASSERTCMP(data[i].path + result, data[i].expected);
  }
}

struct path_clean_output
{
  const char *path;
  const char *posix_result;
  const char *win32_result;
};

#ifdef __WIN32__
#define PATH_CLEAN_RESULT win32_result
#else
#define PATH_CLEAN_RESULT posix_result
#endif

UNITTEST(path_clean_slashes)
{
  static const path_clean_output data[] =
  {
    {
      "",
      "",
      ""
    },
    {
      "/a/path",
      "/a/path",
      "\\a\\path"
    },
    {
      "/remove/trailing/slash/",
      "/remove/trailing/slash",
      "\\remove\\trailing\\slash"
    },
    {
      "/normalize\\slashes/that\\are/like\\this",
      "/normalize/slashes/that/are/like/this",
      "\\normalize\\slashes\\that\\are\\like\\this",
    },
    {
      "/////remove////duplicate//////slashes/////",
      "/remove/duplicate/slashes",
      "\\remove\\duplicate\\slashes",
    },
    {
      "C:\\work\\on\\dos\\style\\paths\\",
      "C:/work/on/dos/style/paths",
      "C:\\work\\on\\dos\\style\\paths",
    },
    {
      "C:\\\\\\remove\\\\\\duplicate\\\\slashes\\\\here\\\\too\\",
      "C:/remove/duplicate/slashes/here/too",
      "C:\\remove\\duplicate\\slashes\\here\\too",
    },
    {
      "C:\\",
      "C:/",
      "C:\\"
    },
    {
      "C:/",
      "C:/",
      "C:\\"
    },
    {
      "/",
      "/",
      "\\"
    },
    {
      "\\",
      "/",
      "\\"
    }
  };
  // Data for testing truncation (assume buffer size == 32). This only matters
  // for path_clean_slashes_copy, which should return false in this case.
  static const path_clean_output truncate_data[] =
  {
    {
      "/a/rly/long/path/looooooooooooooooooooool/",
      "/a/rly/long/path/looooooooooooo",
      "\\a\\rly\\long\\path\\looooooooooooo",
    },
    {
      "C:\\truncate\\my\\dos\\style\\path\\pls",
      "C:/truncate/my/dos/style/path/p",
      "C:\\truncate\\my\\dos\\style\\path\\p",
    },
  };

  char buffer[MAX_PATH];
  size_t result;
  int i;

  SECTION(path_clean_slashes)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      snprintf(buffer, MAX_PATH, "%s", data[i].path);
      buffer[MAX_PATH - 1] = '\0';

      path_clean_slashes(buffer, MAX_PATH);
      ASSERTCMP(buffer, data[i].PATH_CLEAN_RESULT);
    }
  }

  SECTION(path_clean_slashes_copy)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      result = path_clean_slashes_copy(buffer, MAX_PATH, data[i].path);
      ASSERTEQ(result, strlen(data[i].PATH_CLEAN_RESULT));
      ASSERTCMP(buffer, data[i].PATH_CLEAN_RESULT);
    }
  }

  SECTION(path_clean_slashes_copy_truncation)
  {
    for(i = 0; i < arraysize(truncate_data); i++)
    {
      result = path_clean_slashes_copy(buffer, 32, truncate_data[i].path);
      ASSERTEQ(result, strlen(truncate_data[i].PATH_CLEAN_RESULT));
      ASSERTCMP(buffer, truncate_data[i].PATH_CLEAN_RESULT);
    }
  }
}

struct path_split_data
{
  const char *path;
  const char *directory_posix;
  const char *directory_win32;
  const char *filename;
  ssize_t dir_return_value;
  ssize_t file_return_value;
  boolean dir_and_file_return_value;
};

#ifdef __WIN32__
#define SPLIT_DIRECTORY directory_win32
#else
#define SPLIT_DIRECTORY directory_posix
#endif

UNITTEST(path_split_functions)
{
  // All of these tests assume a directory stat fail on the input path.
  static const path_split_data data[] =
  {
    {
      "",
      nullptr,
      nullptr,
      nullptr,
      -1,
      -1,
      false
    },
    {
      "a",
      "",
      "",
      "a",
      0,
      1,
      true
    },
    {
      "filename.ext",
      "",
      "",
      "filename.ext",
      0,
      12,
      true
    },
    {
      "input/filename.ext",
      "input",
      "input",
      "filename.ext",
      5,
      12,
      true
    },
    {
      "input\\filename.ext",
      "input",
      "input",
      "filename.ext",
      5,
      12,
      true
    },
    {
      "input/",
      "input",
      "input",
      "",
      5,
      0,
      true
    },
    {
      "input\\",
      "input",
      "input",
      "",
      5,
      0,
      true
    },
    {
      "C:\\Users\\MegaZeux\\Desktop\\MegaZeux\\Zeux\\Caverns\\CAVERNS.MZX",
      "C:/Users/MegaZeux/Desktop/MegaZeux/Zeux/Caverns",
      "C:\\Users\\MegaZeux\\Desktop\\MegaZeux\\Zeux\\Caverns",
      "CAVERNS.MZX",
      47,
      11,
      true
    },
    {
      u8"/home/\u00C8śŚ/megazeux/DE/DE_START.MZX",
      u8"/home/\u00C8śŚ/megazeux/DE",
      u8"\\home\\\u00C8śŚ\\megazeux\\DE",
      "DE_START.MZX",
      24,
      12,
      true
    },
    {
      u8"/home/ćçáö/megazeux/DE/saved.\u00C8śŚ.sav",
      u8"/home/ćçáö/megazeux/DE",
      u8"\\home\\ćçáö\\megazeux\\DE",
      u8"saved.\u00C8śŚ.sav",
      26,
      16,
      true
    },
  };
  // Internally all of these functions may stat the provided directory to
  // determine how much of it is/isn't a path. These paths all assume that
  // a directory stat succeeds for the input path.
  static const path_split_data data_stat[] =
  {
    {
      "actually_a_path",
      "actually_a_path",
      "actually_a_path",
      "",
      15,
      0,
      true
    },
    {
      "directory/with/no/trailing/slash",
      "directory/with/no/trailing/slash",
      "directory\\with\\no\\trailing\\slash",
      "",
      32,
      0,
      true
    },
    {
      "C:\\dos\\style\\directory",
      "C:/dos/style/directory",
      "C:\\dos\\style\\directory",
      "",
      22,
      0,
      true
    },
  };
  char dir_buffer[MAX_PATH];
  char file_buffer[MAX_PATH];
  ssize_t result;
  int i;

  stat_result_type = STAT_FAIL;

  SECTION(path_has_directory)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      boolean result = path_has_directory(data[i].path);
      ASSERTEQX(result, data[i].dir_return_value > 0, data[i].path);
    }

    stat_result_type = STAT_DIR;

    for(i = 0; i < arraysize(data_stat); i++)
    {
      boolean result = path_has_directory(data_stat[i].path);
      ASSERTEQX(result, data_stat[i].dir_return_value > 0, data_stat[i].path);
    }
  }

  SECTION(path_to_directory)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      snprintf(dir_buffer, MAX_PATH, "%s", data[i].path);
      dir_buffer[MAX_PATH - 1] = '\0';

      result = path_to_directory(dir_buffer, MAX_PATH);
      ASSERTEQX(result, data[i].dir_return_value, data[i].path);
      if(result >= 0 && data[i].SPLIT_DIRECTORY)
        ASSERTCMP(dir_buffer, data[i].SPLIT_DIRECTORY);
    }

    stat_result_type = STAT_DIR;

    for(i = 0; i < arraysize(data_stat); i++)
    {
      snprintf(dir_buffer, MAX_PATH, "%s", data_stat[i].path);
      dir_buffer[MAX_PATH - 1] = '\0';

      result = path_to_directory(dir_buffer, MAX_PATH);
      ASSERTEQX(result, data_stat[i].dir_return_value, data_stat[i].path);
      if(result >= 0 && data_stat[i].SPLIT_DIRECTORY)
        ASSERTCMP(dir_buffer, data_stat[i].SPLIT_DIRECTORY);
    }
  }

  SECTION(path_get_directory)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      result = path_get_directory(dir_buffer, MAX_PATH, data[i].path);
      ASSERTEQX(result, data[i].dir_return_value, data[i].path);
      if(result >= 0 && data[i].SPLIT_DIRECTORY)
        ASSERTCMP(dir_buffer, data[i].SPLIT_DIRECTORY);
    }

    stat_result_type = STAT_DIR;

    for(i = 0; i < arraysize(data_stat); i++)
    {
      result = path_get_directory(dir_buffer, MAX_PATH, data_stat[i].path);
      ASSERTEQX(result, data_stat[i].dir_return_value, data_stat[i].path);
      if(result >= 0 && data_stat[i].SPLIT_DIRECTORY)
        ASSERTCMP(dir_buffer, data_stat[i].SPLIT_DIRECTORY);
    }
  }

  SECTION(path_to_filename)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      snprintf(file_buffer, MAX_PATH, "%s", data[i].path);
      file_buffer[MAX_PATH - 1] = '\0';

      result = path_to_filename(file_buffer, MAX_PATH);
      ASSERTEQX(result, data[i].file_return_value, data[i].path);
      if(result >= 0 && data[i].filename)
        ASSERTCMP(file_buffer, data[i].filename);
    }

    stat_result_type = STAT_DIR;

    for(i = 0; i < arraysize(data_stat); i++)
    {
      snprintf(file_buffer, MAX_PATH, "%s", data_stat[i].path);
      file_buffer[MAX_PATH - 1] = '\0';

      result = path_to_filename(file_buffer, MAX_PATH);
      ASSERTEQX(result, data_stat[i].file_return_value, data_stat[i].path);
      if(result >= 0 && data_stat[i].filename)
        ASSERTCMP(file_buffer, data_stat[i].filename);
    }
  }

  SECTION(path_get_filename)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      result = path_get_filename(file_buffer, MAX_PATH, data[i].path);
      ASSERTEQX(result, data[i].file_return_value, data[i].path);
      if(result >= 0 && data[i].filename)
        ASSERTCMP(file_buffer, data[i].filename);
    }

    stat_result_type = STAT_DIR;

    for(i = 0; i < arraysize(data_stat); i++)
    {
      result = path_get_filename(file_buffer, MAX_PATH, data_stat[i].path);
      ASSERTEQX(result, data_stat[i].file_return_value, data_stat[i].path);
      if(result >= 0 && data_stat[i].filename)
        ASSERTCMP(file_buffer, data_stat[i].filename);
    }
  }

  SECTION(path_get_directory_and_filename)
  {
    boolean result;

    for(i = 0; i < arraysize(data); i++)
    {
      result = path_get_directory_and_filename(
        dir_buffer, MAX_PATH, file_buffer, MAX_PATH, data[i].path);
      ASSERTEQX(result, data[i].dir_and_file_return_value, data[i].path);
      if(result)
      {
        if(data[i].SPLIT_DIRECTORY)
          ASSERTCMP(dir_buffer, data[i].SPLIT_DIRECTORY);

        if(data[i].filename)
          ASSERTCMP(file_buffer, data[i].filename);
      }
    }

    stat_result_type = STAT_DIR;

    for(i = 0; i < arraysize(data_stat); i++)
    {
      result = path_get_directory_and_filename(
        dir_buffer, MAX_PATH, file_buffer, MAX_PATH, data_stat[i].path);
      ASSERTEQX(result, data_stat[i].dir_and_file_return_value, data_stat[i].path);
      if(result)
      {
        if(data_stat[i].SPLIT_DIRECTORY)
          ASSERTCMP(dir_buffer, data_stat[i].SPLIT_DIRECTORY);

        if(data_stat[i].filename)
          ASSERTCMP(file_buffer, data_stat[i].filename);
      }
    }
  }
}

struct path_target_output
{
  const char *path;
  const char *target;
  const char *posix_result;
  const char *win32_result;
  ssize_t return_value;
};

UNITTEST(path_append_and_path_join)
{
  static const path_target_output data[] =
  {
    {
      "",
      "",
      nullptr,
      nullptr,
      -1
    },
    {
      "/a/base",
      "",
      nullptr,
      nullptr,
      -1
    },
    {
      "",
      "a/target",
      nullptr,
      nullptr,
      -1
    },
    {
      "/base/path",
      "a/target.ext",
      "/base/path/a/target.ext",
      "\\base\\path\\a\\target.ext",
      23
    },
    {
      "/do/not/duplicate/",
      "this/slash",
      "/do/not/duplicate/this/slash",
      "\\do\\not\\duplicate\\this\\slash",
      28
    },
    {
      "/loool/",
      "loool/",
      "/loool/loool",
      "\\loool\\loool",
      12
    },
    {
      "C:\\dos\\path",
      "to\\join",
      "C:/dos/path/to/join",
      "C:\\dos\\path\\to\\join",
      19
    }
  };
  // Assume a buffer size of 32 for these.
  static const path_target_output small_data[] =
  {
    {
      "/should/barely/fit",
      "this/string",
      "/should/barely/fit/this/string",
      "\\should\\barely\\fit\\this\\string",
      30
    },
    {
      "C:/an/exact/fit",
      "and/should/pass",
      "C:/an/exact/fit/and/should/pass",
      "C:\\an\\exact\\fit\\and\\should\\pass",
      31
    },
    {
      "/should/not/be/able/to/fit",
      "these/strings",
      nullptr,
      nullptr,
      -1
    },
    {
      "C:\\wow\\this\\path\\is\\kinda\\very\\long\\",
      "whatever",
      nullptr,
      nullptr,
      -1
    },
    {
      "C:\\",
      "wtf\\is\\wrong\\with\\you\\no\\seriously",
      nullptr,
      nullptr,
      -1
    }
  };

  char buffer[MAX_PATH];
  ssize_t result;
  int i;

  SECTION(path_append_NormalCases)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      snprintf(buffer, MAX_PATH, "%s", data[i].path);
      buffer[MAX_PATH - 1] = '\0';

      result = path_append(buffer, MAX_PATH, data[i].target);
      ASSERTEQX(result, data[i].return_value, data[i].path);
      if(result && data[i].PATH_CLEAN_RESULT)
        ASSERTCMP(buffer, data[i].PATH_CLEAN_RESULT);
    }
  }

  SECTION(path_append_SmallBufferCases)
  {
    for(i = 0; i < arraysize(small_data); i++)
    {
      snprintf(buffer, MAX_PATH, "%s", small_data[i].path);
      buffer[MAX_PATH - 1] = '\0';

      result = path_append(buffer, 32, small_data[i].target);
      ASSERTEQX(result, small_data[i].return_value, small_data[i].path);
      if(result && small_data[i].PATH_CLEAN_RESULT)
      {
        ASSERTCMP(buffer, small_data[i].PATH_CLEAN_RESULT);
      }
      else
        ASSERTCMP(buffer, small_data[i].path);
    }
  }

  SECTION(path_join_NormalCases)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      result = path_join(buffer, MAX_PATH, data[i].path, data[i].target);
      ASSERTEQX(result, data[i].return_value, data[i].path);
      if(result && data[i].PATH_CLEAN_RESULT)
        ASSERTCMP(buffer, data[i].PATH_CLEAN_RESULT);
    }
  }

  SECTION(path_join_SmallBufferCases)
  {
    for(i = 0; i < arraysize(small_data); i++)
    {
      static const char *def = "DO NOT MODIFY";
      snprintf(buffer, MAX_PATH, "%s", def);

      result = path_join(buffer, 32, small_data[i].path, small_data[i].target);
      ASSERTEQX(result, small_data[i].return_value, small_data[i].path);
      if(result && small_data[i].PATH_CLEAN_RESULT)
      {
        ASSERTCMP(buffer, small_data[i].PATH_CLEAN_RESULT);
      }
      else
        ASSERTCMP(buffer, def);
    }
  }
}

UNITTEST(path_remove_prefix)
{
  static const path_target_output data[] =
  {
    {
      "",
      "",
      nullptr,
      nullptr,
      -1
    },
    {
      "valid path",
      "",
      nullptr,
      nullptr,
      -1
    },
    {
      "",
      "valid prefix",
      nullptr,
      nullptr,
      -1
    },
    {
      "/some/path/here/with/an/invalid/prefix",
      "/some/p",
      nullptr,
      nullptr,
      -1
    },
    {
      "/some/regular/path",
      "/some/regular/path/except/the/prefix/is/really/long",
      nullptr,
      nullptr,
      -1
    },
    {
      "C:\\dont\\mix\\root\\styles",
      "\\dont\\mix\\root",
      nullptr,
      nullptr,
      -1
    },
    {
      "/dont/mix/slash/styles",
      "\\dont/mix\\slash",
      nullptr,
      nullptr,
      -1
    },
    {
      "/some/path/here",
      "/some/path",
      "here",
      "here",
      4
    },
    {
      "/some/prefix/some/path/here",
      "/some/prefix/",
      "some/path/here",
      "some\\path\\here",
      14
    },
    {
      "C:\\a\\dos\\style\\prefixed\\path",
      "C:\\a\\dos\\style\\prefixed",
      "path",
      "path",
      4
    },
    {
      "work\\on\\relative\\paths\\too\\thanks",
      "work\\on\\relative\\",
      "paths/too/thanks",
      "paths\\too\\thanks",
      16
    },
    {
      "consume/all/slashes////////////////////////////////////thanks",
      "consume/all/slashes",
      "thanks",
      "thanks",
      6
    },
  };
  char buffer[MAX_PATH];
  ssize_t result;
  int i;

  SECTION(NoPrefixLength)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      snprintf(buffer, MAX_PATH, "%s", data[i].path);
      buffer[MAX_PATH - 1] = '\0';

      result = path_remove_prefix(buffer, MAX_PATH, data[i].target, 0);
      ASSERTEQX(result, data[i].return_value, data[i].path);
      if(result >= 0)
        ASSERTCMP(buffer, data[i].PATH_CLEAN_RESULT);
      else
        ASSERTCMP(buffer, data[i].path);
    }
  }

  SECTION(PrefixLength)
  {
    for(i = 0; i < arraysize(data); i++)
    {
      snprintf(buffer, MAX_PATH, "%s", data[i].path);
      buffer[MAX_PATH - 1] = '\0';

      result = path_remove_prefix(buffer, MAX_PATH, data[i].target,
       strlen(data[i].target));
      ASSERTEQX(result, data[i].return_value, data[i].path);
      if(result >= 0)
        ASSERTCMP(buffer, data[i].PATH_CLEAN_RESULT);
      else
        ASSERTCMP(buffer, data[i].path);
    }
  }
}

UNITTEST(path_navigate)
{
  static const path_target_output data[] =
  {
    {
      "",
      "",
      nullptr,
      nullptr,
      -1
    },
    {
      "lol",
      "",
      nullptr,
      nullptr,
      -1
    },
    {
      "",
      "lol",
      nullptr,
      nullptr,
      -1
    },
    {
      "/start/path",
      "relative/target",
      "/start/path/relative/target",
      "\\start\\path\\relative\\target",
      27
    },
    {
      "/",
      "hello",
      "/hello",
      "\\hello",
      6
    },
    {
      "C:\\",
      "a",
      "C:/a",
      "C:\\a",
      4
    },
    {
      "/some/path",
      "..",
      "/some",
      "\\some",
      5
    },
    {
      "/",
      "..",
      "/",
      "\\",
      1
    },
    {
      "/another/path",
      "./../path/../../../another/./",
      "/another",
      "\\another",
      8
    },
    {
      "/start/path",
      "/an/absolute/path",
      "/an/absolute/path",
      "\\an\\absolute\\path",
      17
    },
    {
      "jdflkjsdlfjksdklfjsdlksjdfklsd",
      "\\also\\an\\absolute\\path",
      "/also/an/absolute/path",
      "\\also\\an\\absolute\\path",
      22
    },
    {
      "C:\\start\\path",
      "D:\\folder",
      "D:/folder",
      "D:\\folder",
      9
    },
    {
      "/some/directory",
      "C:",
      "C:/",
      "C:\\",
      3
    },
    {
      "/cwd",
      "mix\\up/some\\of/these\\slashes/lol",
      "/cwd/mix/up/some/of/these/slashes/lol",
      "\\cwd\\mix\\up\\some\\of\\these\\slashes\\lol",
      37
    },
    {
      "/skdlfjlskdjfklsd/",
      "i/am\\sure/..\\someone/relies\\..\\on/this",
      "/skdlfjlskdjfklsd/i/am/someone/on/this",
      "\\skdlfjlskdjfklsd\\i\\am\\someone\\on\\this",
      38
    },
  };
  char buffer[MAX_PATH];
  ssize_t result;
  int i;

  SECTION(Success)
  {
    stat_result_type = STAT_DIR;

    for(i = 0; i < arraysize(data); i++)
    {
      snprintf(buffer, MAX_PATH, "%s", data[i].path);
      buffer[MAX_PATH - 1] = '\0';

      result = path_navigate(buffer, MAX_PATH, data[i].target);
      ASSERTEQX(result, data[i].return_value, data[i].path);
      if(result && data[i].PATH_CLEAN_RESULT)
        ASSERTCMP(buffer, data[i].PATH_CLEAN_RESULT);
    }
  }

  SECTION(Fail)
  {
    stat_result_type = STAT_FAIL;

    for(i = 0; i < arraysize(data); i++)
    {
      snprintf(buffer, MAX_PATH, "%s", data[i].path);
      buffer[MAX_PATH - 1] = '\0';

      result = path_navigate(buffer, MAX_PATH, data[i].target);
      ASSERTEQX(result, -1, data[i].path);
      ASSERTCMP(buffer, data[i].path);
    }
  }
}

struct path_mkdir_data
{
  const char *path;
};

template<size_t N>
static void test_mkdir(const path_mkdir_data (&data)[N],
 enum path_create_error expected)
{
  for(size_t i = 0; i < N; i++)
  {
    const path_mkdir_data &cur = data[i];
    enum path_create_error ret = path_create_parent_recursively(cur.path);
    ASSERTEQX(ret, expected, cur.path);
  }
}

UNITTEST(path_create_parent_recursively)
{
  static const path_mkdir_data has_parent[] =
  {
    { "path/with/parent.txt" },
    { "not\\really\\a\\lot\\to\\test\\here\\right\\now" },
    { "check/out\\my/cool\\slashes" },
  };

  static const path_mkdir_data no_parent[] =
  {
    { "config.txt" },
    { "lol.cnf" },
    { "megazeux.exe" },
  };

  mkdir_result = 0;
  stat_result_type = STAT_FAIL;
  stat_result_errno = ENOENT;

  SECTION(Success)
  {
    test_mkdir(has_parent, PATH_CREATE_SUCCESS);
  }

  SECTION(NoParent)
  {
    test_mkdir(no_parent, PATH_CREATE_SUCCESS);
  }

  SECTION(ParentExists)
  {
    stat_result_type = STAT_DIR;
    test_mkdir(has_parent, PATH_CREATE_SUCCESS);
  }

  SECTION(FileExists)
  {
    stat_result_type = STAT_REG;
    test_mkdir(no_parent, PATH_CREATE_SUCCESS);
    test_mkdir(has_parent, PATH_CREATE_ERR_FILE_EXISTS);
  }

  SECTION(mkdirFail)
  {
    mkdir_result = -1;
    test_mkdir(no_parent, PATH_CREATE_SUCCESS);
    test_mkdir(has_parent, PATH_CREATE_ERR_MKDIR_FAILED);
  }

  SECTION(StatError)
  {
    stat_result_errno = EACCES;
    test_mkdir(no_parent, PATH_CREATE_SUCCESS);
    test_mkdir(has_parent, PATH_CREATE_ERR_STAT_ERROR);
  }
}
