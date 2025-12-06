/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */
#ifndef MM64
#define MM64
#endif

#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

#if defined(MM64)

#define PAGING64_TABLE_ENTRIES 512
#define PAGING64_ENTRY_SIZE sizeof(ADDR_TYPE)

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(addr_t *pte,
             int pre,    // present
             addr_t fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0)
        return -1;  // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}


/*
 * get_pd_from_pagenum - Parse address to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
  // @Khoa
	/* TODO: implement the page direactories mapping */
  *pgd = PAGING64_ADDR_PGD(addr);
  *p4d = PAGING64_ADDR_P4D(addr);
  *pud = PAGING64_ADDR_PUD(addr);
  *pmd = PAGING64_ADDR_PMD(addr);
  *pt  = PAGING64_ADDR_PT(addr);

	return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_pagenum(addr_t pgn, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Shift the address to get page num and perform the mapping*/
	return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT,
                         pgd,p4d,pud,pmd,pt);
}


/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
  // @Khoa

  addr_t *pte = NULL;
  addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
	
#ifdef MM64	
  // @Khoa

  /* Get value from the system */
  /* TODO Perform multi-level page mapping */
  struct mm_struct *mm = caller->krnl->mm;
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);

  addr_t *pgd_base = mm->pgd;

  if (!pgd_base[pgd_idx]) return -1;
  addr_t *p4d_base = (addr_t *)pgd_base[pgd_idx];

  if (!p4d_base[p4d_idx]) return -1;
  addr_t *pud_base = (addr_t *)p4d_base[p4d_idx];

  if (!pud_base[pud_idx]) return -1;
  addr_t *pmd_base = (addr_t *)pud_base[pud_idx];

  if (!pmd_base[pmd_idx]) return -1;
  addr_t *pt_base = (addr_t *)pmd_base[pmd_idx];

  pte = &pt_base[pt_idx];
#else
  struct krnl_t *krnl = caller->krnl;
  pte = &krnl->mm->pgd[pgn];
#endif
	
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  // @Khoa

  addr_t *pte = NULL;
  addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
	
#ifdef MM64	
  // @Khoa

  struct mm_struct *mm = caller->krnl->mm;
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);

  addr_t *pgd_base = mm->pgd;

  if (!pgd_base[pgd_idx]) return -1;
  addr_t *p4d_base = (addr_t *)pgd_base[pgd_idx];

  if (!p4d_base[p4d_idx]) return -1;
  addr_t *pud_base = (addr_t *)p4d_base[p4d_idx];
  
  if (!pud_base[pud_idx]) return -1;
  addr_t *pmd_base = (addr_t *)pud_base[pud_idx];
  
  if (!pmd_base[pmd_idx]) return -1;
  addr_t *pt_base  = (addr_t *)pmd_base[pmd_idx];
  
  pte = &pt_base[pt_idx];
#else
  struct krnl_t *krnl = caller->krnl;
  pte = &krnl->mm->pgd[pgn];
#endif

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry 
 **/
uint32_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
// @Khoa
  uint32_t ret_pte = 0;
  addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
#ifdef MM64  
  struct mm_struct *mm = caller->krnl->mm;

  /* TODO Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);  //... krnl->mm->pgd
  addr_t *pgd_base = mm->pgd;
  if (pgd_base == NULL || pgd_base[pgd_idx] == 0) return 0;
  
  addr_t *p4d_base = (addr_t *)pgd_base[pgd_idx];
  if (p4d_base[p4d_idx] == 0) return 0;
  
  addr_t *pud_base = (addr_t *)p4d_base[p4d_idx];
  if (pud_base[pud_idx] == 0) return 0;
  
  addr_t *pmd_base = (addr_t *)pud_base[pud_idx];
  if (pmd_base[pmd_idx] == 0) return 0;
  
  addr_t *pt_base  = (addr_t *)pmd_base[pmd_idx];

  ret_pte = (uint32_t)pt_base[pt_idx];
#else
  if (caller->krnl->mm->pgd) ret_pte = caller->krnl->mm->pgd[pgn];
#endif
  return ret_pte;
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
	struct krnl_t *krnl = caller->krnl;
	krnl->mm->pgd[pgn]=pte_val;
	
	return 0;
}


/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum)                      // num of mapping page
{
  // @Khoa
  
  struct mm_struct *mm = caller->krnl->mm; // FIXED
  addr_t *pgd_base, *p4d_base, *pud_base, *pmd_base, *pt_base;
  addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
  int pgit;
  addr_t vaddr = addr;

  for (pgit = 0; pgit < pgnum; pgit++) {
     get_pd_from_address(vaddr, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);

     // --- LEVEL 1: PGD ---
     pgd_base = mm->pgd;
     if (pgd_base[pgd_idx] == 0) {
         void *new_table = malloc(PAGING64_TABLE_ENTRIES * PAGING64_ENTRY_SIZE);
         memset(new_table, 0, PAGING64_TABLE_ENTRIES * PAGING64_ENTRY_SIZE);
         pgd_base[pgd_idx] = (addr_t)new_table;
     }

     // --- LEVEL 2: P4D ---
     p4d_base = (addr_t *)pgd_base[pgd_idx];
     if (p4d_base[p4d_idx] == 0) {
         void *new_table = malloc(PAGING64_TABLE_ENTRIES * PAGING64_ENTRY_SIZE);
         memset(new_table, 0, PAGING64_TABLE_ENTRIES * PAGING64_ENTRY_SIZE);
         p4d_base[p4d_idx] = (addr_t)new_table;
     }

     // --- LEVEL 3: PUD ---
     pud_base = (addr_t *)p4d_base[p4d_idx];
     if (pud_base[pud_idx] == 0) {
         void *new_table = malloc(PAGING64_TABLE_ENTRIES * PAGING64_ENTRY_SIZE);
         memset(new_table, 0, PAGING64_TABLE_ENTRIES * PAGING64_ENTRY_SIZE);
         pud_base[pud_idx] = (addr_t)new_table;
     }

     // --- LEVEL 4: PMD ---
     pmd_base = (addr_t *)pud_base[pud_idx];
     if (pmd_base[pmd_idx] == 0) {
         void *new_table = malloc(PAGING64_TABLE_ENTRIES * PAGING64_ENTRY_SIZE);
         memset(new_table, 0, PAGING64_TABLE_ENTRIES * PAGING64_ENTRY_SIZE);
         pmd_base[pmd_idx] = (addr_t)new_table;
     }

     vaddr += PAGING64_PAGESZ; 
  }

  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
  // @Khoa

  struct framephy_struct *fpit = frames;
  int pgit = 0;
  addr_t pgn = addr >> PAGING64_ADDR_PT_SHIFT;

  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr + pgnum * PAGING64_PAGESZ;

  for(pgit = 0; pgit < pgnum; pgit++) {
     if (fpit == NULL) break;
     
     pte_set_fpn(caller, pgn + pgit, fpit->fpn);
     
     enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn + pgit); // FIXED

     fpit = fpit->fp_next;
  }
  return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  // @Khoa

  int pgit;
  addr_t fpn;
  struct framephy_struct *newfp_str = NULL;

  /* TODO: allocate the page */
  for(pgit = 0; pgit < req_pgnum; pgit++)
  {
    if(MEMPHY_get_freefp(caller->krnl->mram, &fpn) == 0) // FIXED
    {
        newfp_str = malloc(sizeof(struct framephy_struct));
        newfp_str->fpn = fpn;
        newfp_str->owner = caller->krnl->mm; // FIXED
        
        newfp_str->fp_next = *frm_lst;
        *frm_lst = newfp_str;
    } 
    else {
        return -3000;
    }
  }

  /* End TODO */

  return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  addr_t ret_alloc = 0;

  // @Khoa
  ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  /* Out of memory */
  if (ret_alloc == -3000)
  {
    return -1;
  }

   vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                   struct memphy_struct *mpdst, addr_t dstfpn)
{
  int cellidx;
  addr_t addrsrc, addrdst;
  for (cellidx = 0; cellidx < PAGING64_PAGESZ; cellidx++)
  {
    addrsrc = srcfpn * PAGING64_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING64_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
  // @Khoa

   mm->pgd = malloc(PAGING64_TABLE_ENTRIES * PAGING64_ENTRY_SIZE);
   memset(mm->pgd, 0, PAGING64_TABLE_ENTRIES * PAGING64_ENTRY_SIZE);
    mm->p4d = NULL;
    mm->pud = NULL;
    mm->pmd = NULL;
    mm->pt  = NULL;

  /* By default the owner comes with at least one vma */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  // @Khoa

  vma0->vm_next = NULL;
  vma0->vm_mm = mm;
  mm->mmap = vma0;

  return 0;
}

struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}

int enlist_pgn_node(struct pgn_t **plist, addr_t pgn)
{
  struct pgn_t *pnode = malloc(sizeof(struct pgn_t));

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
  struct framephy_struct *fp = ifp;

  printf("print_list_fp: ");
  if (fp == NULL) { printf("NULL list\n"); return -1;}
  printf("\n");
  while (fp != NULL)
  {
    printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[" FORMAT_ADDR "->"  FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
  struct vm_area_struct *vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (vma != NULL)
  {
    printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (ip != NULL)
  {
    printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("\n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
// @Khoa
addr_t pgn_start = start >> PAGING64_ADDR_PT_SHIFT;
addr_t  pgn_end = end >> PAGING64_ADDR_PT_SHIFT;

printf (" PAGE TABLE DUMP (Virtual range: %lx - %lx )\n", start, end);

for (addr_t pgn = pgn_start; pgn <= pgn_end; pgn++) {
    uint32_t pte = pte_get_entry(caller, pgn);
    if (pte & PAGING_PTE_PRESENT_MASK) {
        addr_t fpn = GETVAL(pte, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
        printf(" PGN: " FORMAT_ADDR " -> PTE: " FORMAT_ADDR " (FPN: " FORMAT_ADDR ")\n", pgn, pte, fpn);
    } else {
        printf(" PGN: " FORMAT_ADDR " -> PTE: " FORMAT_ADDR " (Not Present)\n", pgn, pte);
    }
  }
  return 0;
}

#endif  //def MM64