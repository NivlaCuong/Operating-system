/* standard structures and constants for the memdb file */
#ifndef DBDUMP_MEMDB_H
#define DBDUMP_MEMDB_H

#include <unistd.h>

static const off_t INIT_SIZE = 64 * 1024;
static const off_t MAX_SIZE = 1024 * 1024 * 1024;

static const int FILE_MAGIC = 0xC5149;
static const short ENTRY_MAGIC_DATA = 0xda7a;
static const short ENTRY_MAGIC_FREE = 0xdead;

typedef unsigned long long moffset_t;

#pragma pack(push,1)
struct entry_s {
    short magic;
    moffset_t next;
    int len;
    char str[];
};

struct fhdr_s {
    int magic;
    moffset_t free_start;
    moffset_t data_start;
};
#pragma pack(pop)

#endif //DBDUMP_MEMDB_H
