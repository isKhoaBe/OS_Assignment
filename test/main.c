/* test/main.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include Header thật (Các macro đã được bật trong os-cfg.h) */
#include "../include/os-mm.h"
#include "../include/mm.h"
#include "../include/common.h" 

/* Mock các hàm vật lý */
int MEMPHY_get_freefp(struct memphy_struct *mp, addr_t *fpn) {
    static addr_t mock = 100;
    if (mock >= mp->maxsz + 100) return -1; 
    *fpn = mock++;
    return 0;
}
int MEMPHY_read(struct memphy_struct * mp, addr_t addr, BYTE *value) { return 0; }
int MEMPHY_write(struct memphy_struct * mp, addr_t addr, BYTE data) { return 0; }
int MEMPHY_dump(struct memphy_struct * mp) { return 0; }
int MEMPHY_put_freefp(struct memphy_struct *mp, addr_t fpn) { return 0; }

/* Include logic cần test */
#include "../src/mm64.c"

void test_page_walk_and_map() {
    printf("=== Test Multi-level Paging (64-bit) ===\n\n");
    
    // 1. Setup struct
    struct mm_struct *my_mm = malloc(sizeof(struct mm_struct));
    struct memphy_struct *my_ram = malloc(sizeof(struct memphy_struct));
    struct pcb_t *my_proc = malloc(sizeof(struct pcb_t));
    
    // Gán trực tiếp vào pcb_t (Vì os-cfg.h đã bật MM_PAGING nên struct này đã có mm)
    my_proc->mm = my_mm;
    my_proc->mram = my_ram;
    my_ram->maxsz = 1000; 
    
    // Test Data
    addr_t TEST_ADDR = 0x100000000000ULL;
    int NUM_TEST = 5; 

    // 2. Chạy test
    printf("Step 1: Initializing MM...\n");
    init_mm(my_mm, my_proc);
    
    printf("Step 2: Creating page table (Dummy Alloc)...\n");
    vmap_pgd_memset(my_proc, TEST_ADDR, NUM_TEST);
    
    printf("Step 3: Allocating frames...\n");
    struct framephy_struct *frames = NULL;
    alloc_pages_range(my_proc, NUM_TEST, &frames);
    
    printf("Step 4: Mapping...\n");
    struct vm_rg_struct region;
    vmap_page_range(my_proc, TEST_ADDR, NUM_TEST, frames, &region);
    
    printf("\nStep 5: Verifying...\n");
    print_pgtbl(my_proc, TEST_ADDR, TEST_ADDR + NUM_TEST * 4096 - 1);
    
    // Cleanup
    free(my_mm); free(my_ram); free(my_proc);
}

int main() {
    test_page_walk_and_map();
    return 0;
}