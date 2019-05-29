#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>


#include "memdb.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("USAGE: %s dbfile\n", argv[0]);
        return 1;
    }

    printf("fhdr_s size %lu entry_s size %lu\n", sizeof(struct fhdr_s), sizeof
            (struct entry_s));
    const char *dbfile = argv[1];

    int fd = open(dbfile, O_RDONLY);
    if (fd == -1) {
        perror(dbfile);
        return 2;
    }

    struct stat s;
    if (fstat(fd, &s) == -1) {
        perror(dbfile);
        return 3;
    }

    struct fhdr_s *fhdr = mmap(NULL, MAX_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (fhdr == (void *) -1) {
        perror(dbfile);
        return 4;
    }
    printf("Magic nubmer: %d\n", fhdr->magic);

    if (s.st_size < sizeof(*fhdr)) {
        printf("%s is not big enough to contain a DB\n", dbfile);
        return 5;
    }

    /* figure out the address of the end of the file */
    void *end_of_file = (char *) fhdr + s.st_size;

    if (fhdr->magic != FILE_MAGIC) {
        printf("%s doesn't have magic number\n", dbfile);
        return 6;
    }

    printf("mapped DB at %p\n", fhdr);
    printf("MAGIC number found\n");
    printf("  data starts at %lld\n", fhdr->data_start);
    printf("  free space starts at %lld\n", fhdr->free_start);

    /*
     * we need to get to the entry header right after the file header. there
     * are a couple of ways to do this.
     */
    /* if we like thinking of memory addresses in terms of bytes, then we can cast
     * fhdr to a char * (byte array) and move the address by the correct number of
     * bytes (the size of the file header). */
    struct entry_s *entry = (struct entry_s *) ((char *) fhdr + sizeof(*fhdr));
    /* we can let the compiler do most of the work for us. the compiler knows the size
     * of a file header pointer, so if we add 1 to it, the pointer shifts by the size of
     * the file header structure. */
    //struct entry_s *entry = (struct entry_s *)(fhdr + 1);
    /* another way to let the compiler do most of the work for us is to treat the file
     * header pointer as a pointer to an array and get the address of the 2nd element
     * of that array. */
    //struct entry_s *entry = (struct entry_s *)&fhdr[1];
    for (int i = 0;
         (void*)entry < end_of_file;
         i++, entry = (struct entry_s *) (entry->str + entry->len)) {
        printf("%hx entry %d@%p offset = %lld len = %d next = %lld: %s\n",
                entry->magic,
                i,
                entry,
                (moffset_t) entry - (moffset_t) fhdr,
                entry->len,
		entry->next,
               (entry->magic == ENTRY_MAGIC_DATA) ? entry->str : "<free>");
    }
    return 0;
}
