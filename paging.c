#include "paging.h"

/* We allocate the Page Directory and one Page Table as global arrays.
 * The __attribute__((aligned(4096))) directive ensures that the compiler
 * aligns these structures to a 4 KB boundary in memory, which is strictly
 * required by the x86 paging hardware.
 */
page_directory_entry_t page_directory[1024] __attribute__((aligned(4096)));
page_table_entry_t first_page_table[1024] __attribute__((aligned(4096)));

void init_paging(void) {
    // 1. Set all entries in the page directory to 'not present' with Read/Write enabled
    for (int i = 0; i < 1024; i++) {
        // Bit 1: Read/Write (1 = writable)
        // Bit 0: Present (0 = not present)
        page_directory[i] = 0x00000002;
    }

    // 2. Identity Map the first 4 MB (1024 pages)
    // Physical address 0x0 maps to Virtual address 0x0
    for (int i = 0; i < 1024; i++) {
        // physical address = i * 4096
        // Flags: 7 (Present = 1, Read/Write = 1, User = 1)
        first_page_table[i] = (i * 4096) | 7;
    }

    // 3. Attach our page table to the first entry of the page directory
    // This covers virtual addresses 0x00000000 to 0x003FFFFF (first 4MB)
    // Flags: 7 (Present = 1, Read/Write = 1, User = 1)
    page_directory[0] = ((uint32_t)first_page_table) | 7;

    // 4. Load the address of the page directory into the CR3 register
    load_page_directory(page_directory);

    // 5. Read CR0, set the Paging bit (bit 31), and write it back to enable paging
    enable_paging();
}
