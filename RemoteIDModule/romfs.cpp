#include "romfs.h"
#include "romfs_files.h"
#include <string.h>

const char *ROMFS::find_string(const char *fname)
{
    for (const auto &f : files) {
        if (strcmp(fname, f.filename) == 0) {
            return (const char *)f.contents;
        }
    }
    return nullptr;
}
