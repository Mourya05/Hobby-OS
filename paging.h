#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/* In 32-bit x86, both Page Directory Entries and Page Table Entries are 32-bit integers */
typedef uint32_t page_directory_entry_t;
typedef uint32_t page_table_entry_t;

extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

void init_paging(void);

#endif
