/* test/main.c - Final Fixed Version */
#define MM64
#define MM_PAGING

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include Header thật */
#include "../include/os-mm.h"
#include "../include/mm.h"
#include "../include/common.h" 

/* Mock các hàm vật lý thiếu */
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

/* Forward declarations */
int vmap_pgd_memset(struct pcb_t *caller, addr_t addr, int pgnum);
addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst);
addr_t vmap_page_range(struct pcb_t *caller, addr_t addr, int pgnum, struct framephy_struct *frames, struct vm_rg_struct *ret_rg);
int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end);
int init_mm(struct mm_struct *mm, struct pcb_t *caller);

void test_page_walk_and_map() {
    printf("=== Test Multi-level Paging (64-bit) ===\n\n");
    
    // Setup struct
    struct mm_struct *my_mm = malloc(sizeof(struct mm_struct));
    struct memphy_struct *my_ram = malloc(sizeof(struct memphy_struct));
    struct pcb_t *my_proc = malloc(sizeof(struct pcb_t));
    
    if (!my_mm || !my_ram || !my_proc) {
        printf("ERROR: Memory allocation failed!\n");
        return;
    }
    
    memset(my_mm, 0, sizeof(struct mm_struct));
    memset(my_ram, 0, sizeof(struct memphy_struct));
    memset(my_proc, 0, sizeof(struct pcb_t));
    
    // ✅ FIX: Gán qua krnl->mm và krnl->mram
    my_proc->krnl = malloc(sizeof(struct krnl_t));
    if (!my_proc->krnl) {
        printf("ERROR: krnl allocation failed!\n");
        return;
    }
    memset(my_proc->krnl, 0, sizeof(struct krnl_t));
    
    my_proc->krnl->mm = my_mm;
    my_proc->krnl->mram = my_ram;
    my_ram->maxsz = 1000; 
    
    // Test Data
    addr_t TEST_ADDR = 0x100000000000ULL;
    int NUM_TEST = 5; 

    printf("Test Configuration:\n");
    printf("  - Virtual Address: 0x%llx\n", (unsigned long long)TEST_ADDR);
    printf("  - Number of Pages: %d\n", NUM_TEST);
    printf("  - RAM Size: %d frames\n\n", my_ram->maxsz);

    // Chạy các hàm cần test
    printf("Step 1: Initializing MM...\n");
    init_mm(my_mm, my_proc);
    
    printf("Step 2: Creating page table structure (vmap_pgd_memset)...\n");
    vmap_pgd_memset(my_proc, TEST_ADDR, NUM_TEST);
    
    printf("Step 3: Allocating physical frames...\n");
    struct framephy_struct *frames = NULL;
    int alloc_result = alloc_pages_range(my_proc, NUM_TEST, &frames);
    if (alloc_result < 0) {
        printf("ERROR: Frame allocation failed!\n");
        return;
    }
    printf("  → Allocated %d frames\n", NUM_TEST);
    
    printf("Step 4: Mapping virtual to physical (vmap_page_range)...\n");
    struct vm_rg_struct region;
    int map_result = vmap_page_range(my_proc, TEST_ADDR, NUM_TEST, frames, &region);
    if (map_result < 0) {
        printf("ERROR: Page mapping failed!\n");
        return;
    }
    
    printf("\nStep 5: Verifying page table entries...\n");
    print_pgtbl(my_proc, TEST_ADDR, TEST_ADDR + NUM_TEST * 4096 - 1);
    
    printf("\n=== Test Completed ===\n");
}

int main() {
    test_page_walk_and_map();
    return 0;
}