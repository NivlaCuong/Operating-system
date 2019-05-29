//
//  main.c
//  persistent
//
//  Created by Calvin Nguyen on 3/29/19.
//  Copyright © 2019 Calvin Nguyen. All rights reserved.
//
//  Free Space management Policy: In my code, I used "Next Fit"
//  because whenever I found the next free space, I just added the new element into that new free-space.
//  In other words, I relocate the next free-start data after the previous data, which includes the offset and the length of the data.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "memdb.h"

void add(char *data);
void list(void);
void delete(char *data);
void initMmap(int argc, char **argv);
void action(char *dbfileName);

int fd, size = INIT_SIZE;
char inp[256], cmd[256], data[256];
struct entry_s *entry;
struct fhdr_s *fhdr;
struct stat s;
void *end_of_file;
const char *dbfile;

int main(int argc, char **argv) {
    if (argc > 1) {
        initMmap(argc, argv);
        action(argv[1]);
    } else {
        printf("USAGE: %s dbfile\n", argv[0]);
        return 1;
    }
}

// mmap to the file
void action(char *dbfileName) {
    while (fgets(inp, 256, stdin) != NULL) {
        // Add '\0' Add the end to avoid NULL
        int index = strlen(inp) - 1;
        inp[index] = '\0';
        
        // Assign substring to cmd and data
        strncpy(cmd, inp, 1);
        strncpy(data, inp+(2), sizeof(data));
        
        // Options
        if (strcmp(cmd, "a") == 0) {        // add data to db
            add(data);
        } else if(strcmp(cmd, "l") == 0) {  // list all data
            list();
        } else {                            // remove data
            delete(data);
        }
    }
}

/**
 * Add the new data to the list in the correct order
 */
void add(char *data) {
    // From Discussion on Canvas.
    // In case we need more space in the database. Ftrunc to the MAX_SIZE
    moffset_t newFreeStart = fhdr->free_start + sizeof(*entry) + strlen(data) + 1;
    if (newFreeStart > size) {
        size = MAX_SIZE;
        if (ftruncate(fd, MAX_SIZE) == -1) {
            perror("Ftruncate Failed");
            exit(8);
        }
    }
    
    // initialize the newEntry
    struct entry_s *newEntry = (struct entry_s *) ((char *) fhdr + fhdr->free_start);
    strcpy(newEntry->str, data);
    newEntry->len = strlen(data) + 1;
    newEntry->magic = ENTRY_MAGIC_DATA;
    newEntry->next = 0;
    
    // First edge case: if entry is NULL => set data start to the entry and return;
    if (fhdr->data_start == 0) {
        fhdr->data_start = fhdr->free_start;
        entry = newEntry;
    } else {
        // Second case: If data < the first element. Add straight to the front of list
        entry = (struct entry_s *) ((char *) fhdr + fhdr->data_start);
        struct entry_s *currentEntry = entry;
        struct entry_s *nextEntry = (struct entry_s *) ((char *) fhdr + currentEntry->next);
        if (strcmp(data, currentEntry->str) < 0) {
            newEntry->next = fhdr->data_start;
            fhdr->data_start = fhdr->free_start;
            entry = newEntry;
        } else {
            // Third case: If data > the first element. Compare the next element and add infront of element that data < element
            for (; (void*) currentEntry < end_of_file;
                 currentEntry    = (struct entry_s *) ((char *) fhdr + currentEntry->next),
                 nextEntry       = (struct entry_s *) ((char *) fhdr + nextEntry->next)) {
                
                if (strcmp(data, currentEntry->str) == 0) {
                    printf("The string is already existed!\n");
                    break;
                } else if (currentEntry->next == 0) {
                    currentEntry->next = fhdr->free_start;
                    break;
                } else if(strcmp(data, nextEntry->str) < 0) {
                    newEntry->next = currentEntry->next;
                    currentEntry->next = fhdr->free_start;
                    break;
                }
            }
        }
    }
    
    fhdr->free_start = newFreeStart;
    struct entry_s *freeEntry = (struct entry_s *) ((char *) fhdr + fhdr->free_start);
    freeEntry->magic = ENTRY_MAGIC_FREE;
    freeEntry->len = size - fhdr->free_start - sizeof(*entry);
    freeEntry->next = 0;
}

/**
 * List all the data in db in sorted order
 */
void list() {
    entry = (struct entry_s *) ((char *) fhdr + fhdr->data_start);
    for (; (void*)entry < end_of_file; entry = (struct entry_s *) ((char *) fhdr + entry->next)) {
        if (entry->magic == ENTRY_MAGIC_DATA) {
            printf("%s\n", entry->str);
        }
    }
}

/**
 * Delete the data and move the pointer + free memory
 */
void delete(char *data) {
    printf("Delete %s to db\n", data);
}

/**
 * initial mmap function. Used source code from Ben
 */
void initMmap(int argc, char **argv) {
    mode_t mode = S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH;
    int private = 0;
    int found = 0;
    
    if (argc >= 2) {
        if (argc == 3 && (strcmp(argv[1], "-t") == 0)) {
            private = 1;
            dbfile = argv[2];
        } else {
            dbfile = argv[1];
        }
        if (access(dbfile, F_OK ) != -1 ) {
            found = 1;
        }
    }
    fd = open(dbfile, O_CREAT | O_RDWR, S_IRWXU);
    if (fd == -1) {
        perror(dbfile);
        exit(2);
    }
    
    if (ftruncate(fd, INIT_SIZE) == -1) {
        perror("Ftruncate Failed");
        exit(3);
    }
    
    if (fstat(fd, &s) == -1) {
        perror(dbfile);
        exit(4);
    }
    
    // Create mmap to the file descriptor with permission to read and write
    if (private == 1) {
        fhdr = mmap(NULL, MAX_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    } else {
        fhdr = mmap(NULL, MAX_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    }

    if (fhdr == (void *) -1) {
        perror(dbfile);
        exit(5);
    }
    
    if (found == 0) {
        fhdr->magic = FILE_MAGIC;
        fhdr->data_start = 0;
        fhdr->free_start = sizeof(*fhdr);
    } else {
        fhdr->magic = FILE_MAGIC;
    }
    
    if (s.st_size < sizeof(*fhdr)) {
        printf("%s is not big enough to contain a DB\n", dbfile);
        exit(6);
    }
    
    /* figure out the address of the end of the file */
    end_of_file = (char *) fhdr + s.st_size;
    
    if (fhdr->magic != FILE_MAGIC) {
        printf("%s doesn't have magic number\n", dbfile);
        exit(7);
    }
    
    entry = (struct entry_s *) ((char *) fhdr + fhdr->free_start);
    entry->magic = ENTRY_MAGIC_FREE;
    entry->len = INIT_SIZE - fhdr->free_start - sizeof(*entry);
    entry->next = 0;
}
