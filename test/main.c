/* test_mm64_main.c - simple unit test for mm64 paging */
#define MM64
#define MM_PAGING

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/mm.h"
#include "../include/mm64.h"
#include "../include/common.h"

/* Minimal mock memphy (for MEMPHY_get_freefp if used) */
int MEMPHY_get_freefp(struct memphy_struct *mp, addr_t *fpn) {
    static addr_t mock = 100;
    if (mp && mock >= (addr_t)mp->maxsz + 100) return -1;
    *fpn = mock++;
    return 0;
}
int MEMPHY_put_freefp(struct memphy_struct *mp, addr_t fpn) { (void)mp; (void)fpn; return 0; }
int MEMPHY_read(struct memphy_struct * mp, addr_t addr, BYTE *value) { (void)mp; (void)addr; (void)value; return 0; }
int MEMPHY_write(struct memphy_struct * mp, addr_t addr, BYTE data) { (void)mp; (void)addr; (void)data; return 0; }

void dump_tables(struct mm_struct *mm, addr_t start_addr) {
    if (!mm) { printf("mm NULL\n"); return; }
    printf("PGD base at %p\n", (void*)mm->pgd);
    addr_t pgd_idx = PAGING64_ADDR_PGD(start_addr);
    addr_t p4d_idx = PAGING64_ADDR_P4D(start_addr);
    addr_t pud_idx = PAGING64_ADDR_PUD(start_addr);
    addr_t pmd_idx = PAGING64_ADDR_PMD(start_addr);
    addr_t pt_idx  = PAGING64_ADDR_PT(start_addr);

    addr_t *pgd_base = (addr_t *) mm->pgd;
    printf("Computed indices: pgd=%llu p4d=%llu pud=%llu pmd=%llu pt=%llu\n",
           (unsigned long long)pgd_idx, (unsigned long long)p4d_idx,
           (unsigned long long)pud_idx, (unsigned long long)pmd_idx,
           (unsigned long long)pt_idx);

    if (pgd_base[pgd_idx]) {
        printf(" P4D table at %p\n", (void*)pgd_base[pgd_idx]);
        addr_t *p4d = (addr_t*)pgd_base[pgd_idx];
        if (p4d[p4d_idx]) {
            printf("  PUD table at %p\n", (void*)p4d[p4d_idx]);
            addr_t *pud = (addr_t*)p4d[p4d_idx];
            if (pud[pud_idx]) {
                printf("   PMD table at %p\n", (void*)pud[pud_idx]);
                addr_t *pmd = (addr_t*)pud[pud_idx];
                if (pmd[pmd_idx]) {
                    printf("    PT table at %p\n", (void*)pmd[pmd_idx]);
                    addr_t *pt = (addr_t*)pmd[pmd_idx];
                    printf("     PTE entry: 0x%llx\n", (unsigned long long)pt[pt_idx]);
                } else printf("   PMD entry is zero\n");
            } else printf("  PUD entry is zero\n");
        } else printf(" P4D entry is zero\n");
    } else {
        printf(" PGD entry is zero\n");
    }
}

int main(void)
{
    struct mm_struct *mm = calloc(1, sizeof(struct mm_struct));
    struct memphy_struct *ram = calloc(1, sizeof(struct memphy_struct));
    struct krnl_t *krnl = calloc(1, sizeof(struct krnl_t));
    struct pcb_t *proc = calloc(1, sizeof(struct pcb_t));

    if (!mm || !ram || !krnl || !proc) {
        fprintf(stderr, "OOM\n"); return 1;
    }

    ram->maxsz = 1000;
    krnl->mm = mm;
    krnl->mram = ram;
    proc->krnl = krnl;

    /* init mm (your function) */
    init_mm(mm, proc);

    /* Show initial state */
    printf("Initial state:\n");
    dump_tables(mm, 0x0);

    /* Map page number 0 (virtual address 0) to fpn 123 */
    addr_t pgn = 0;
    addr_t fpn = 123;
    if (pte_set_fpn(proc, pgn, fpn) != 0) {
        fprintf(stderr, "pte_set_fpn failed\n");
        return 2;
    }

    printf("\nAfter pte_set_fpn( pgn=0 -> fpn=123 ):\n");
    dump_tables(mm, 0x0);

    /* Print PTE explicitly */
    addr_t pte_val = 0;
    /* get PD pointers to read PTE */
    addr_t pgd_idx = PAGING64_ADDR_PGD(0);
    addr_t *pgd_base = (addr_t *)mm->pgd;
    addr_t *p4d = (addr_t*)pgd_base[pgd_idx];
    addr_t p4d_idx = PAGING64_ADDR_P4D(0);
    addr_t *pud = (addr_t*)p4d[p4d_idx];
    addr_t pud_idx = PAGING64_ADDR_PUD(0);
    addr_t *pmd = (addr_t*)pud[pud_idx];
    addr_t pmd_idx = PAGING64_ADDR_PMD(0);
    addr_t *pt  = (addr_t*)pmd[pmd_idx];
    addr_t pt_idx = PAGING64_ADDR_PT(0);

    if (pt) {
        pte_val = pt[pt_idx];
        printf("\nFinal PTE value (raw): 0x%llx\n", (unsigned long long)pte_val);
    } else {
        printf("PT missing\n");
    }

    return 0;
}
