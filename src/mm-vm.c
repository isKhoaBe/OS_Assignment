/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn , addr_t swpfpn)
{
    __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
    return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz)
{//@hưng
  struct vm_rg_struct * newrg;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;

  /* END TODO */

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend)
{
  //struct vm_area_struct *vma = caller->krnl->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  if (vmastart >= vmaend)
  {
    return -1;
  }

  struct vm_area_struct *vma = caller->krnl->mm->mmap;
  if (vma == NULL)
  {
    return -1;
  }

  /* TODO validate the planned memory area is not overlapped */

  struct vm_area_struct *cur_area = get_vma_by_num(caller->krnl->mm, vmaid);
  if (cur_area == NULL)
  {
    return -1;
  }

  while (vma != NULL)
  {
    if (vma != cur_area && OVERLAP(vmastart, vmaend, vma->vm_start, vma->vm_end))//@hưng
    {
      return -1;
    }
    vma = vma->vm_next;
  }
  /* End TODO*/

  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{//@hưng
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));

  /* TOTO with new address scheme, the size need tobe aligned 
   *      the raw inc_sz maybe not fit pagesize
   */ 
  //addr_t inc_amt;

//  int incnumpage =  inc_amt / PAGING_PAGESZ;
  int inc_amt;
  int incnumpage;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  int old_end;

  inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  incnumpage = inc_amt / PAGING_PAGESZ;

  old_end = cur_vma->sbrk;
  /* TODO Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, old_end, old_end + inc_amt) < 0)
    return -1; /* Overlap and failed allocation */

  /* TODO: Obtain the new vm area based on vmaid */
  //cur_vma->vm_end... 
  // inc_limit_ret...
  /* The obtained vm area (only)
   * now will be alloc real ram region */

  if (vm_map_ram(caller, old_end, old_end + inc_amt, 
                    old_end, incnumpage , newrg) < 0)
      return -1; /* Mapping failed (Out of RAM) */

  cur_vma->sbrk += inc_amt;
  cur_vma->vm_end += inc_amt; // Also expand the VMA end definition

  return 0;
}

// #endif

/* -----------------------
   Page management / swapping (@quocHuy)
   ----------------------- */
   
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte;
  addr_t tgtfpn = -1;
  addr_t vicpgn = -1;
  addr_t vicfpn;
  addr_t swpfpn;

  if (!caller || !caller->krnl) return -1;

  /* read current PTE */
  pte = pte_get_entry(caller, pgn);

  /* if present in RAM -> just return fpn */
  if (PAGING_PAGE_PRESENT(pte))
  {
    *fpn = PAGING_FPN(pte);
    return 0;
  }

  /* page not present: need to bring it to RAM */
  /* 1) Try to find a victim page to free RAM frame */
  if (find_victim_page(caller->krnl->mm, &vicpgn) == -1)
  {
    /* If no victim list available, try to get a free frame directly */
    if (MEMPHY_get_freefp(caller->krnl->mram, &tgtfpn) == -1)
      return -1; /* no free RAM frame -> failure */
  }
  else
  {
    /* We have a victim page: get its PTE and FPN */
    uint32_t vict_pte = pte_get_entry(caller, vicpgn);

    if (!PAGING_PAGE_PRESENT(vict_pte))
    {
      /* unexpected: victim not present; fallback to free frame */
      if (MEMPHY_get_freefp(caller->krnl->mram, &tgtfpn) == -1)
        return -1;
    }
    else
    {
      vicfpn = PAGING_FPN(vict_pte);

      /* allocate a free frame on swap device */
      if (MEMPHY_get_freefp(caller->krnl->active_mswp, &swpfpn) == -1)
        return -1;

      /* copy victim frame RAM -> SWAP */
      __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);

      /* update victim PTE as swapped (set swap offset) */
      if (pte_set_swap(caller, vicpgn, 0 /*swp type*/, swpfpn) != 0)
      {
        /* rollback swap allocation */
        MEMPHY_put_freefp(caller->krnl->active_mswp, swpfpn);
        return -1;
      }

      /* free victim RAM frame back to RAM free list */
      MEMPHY_put_freefp(caller->krnl->mram, vicfpn);

      /* allocate a RAM frame for request */
      if (MEMPHY_get_freefp(caller->krnl->mram, &tgtfpn) == -1)
        return -1;
    }
  }

  /* 2) If requested page was swapped earlier -> bring from swap */
  pte = pte_get_entry(caller, pgn);
  if ( (pte & PAGING_PTE_SWAPPED_MASK) )
  {
    addr_t swp_offset = PAGING_SWP(pte);
    /* copy from swap -> ram frame */
    __swap_cp_page(caller->krnl->active_mswp, swp_offset, caller->krnl->mram, tgtfpn);

    /* free swap frame (we reclaimed swap cell) */
    MEMPHY_put_freefp(caller->krnl->active_mswp, swp_offset);
  }
  else
  {
    /* page is new / zero page: assume frame zeroed at MEMPHY init */
  }

  /* 3) update PTE to mark present and set FPN */
  if (pte_set_fpn(caller, pgn, tgtfpn) != 0)
  {
    /* cleanup on failure */
    MEMPHY_put_freefp(caller->krnl->mram, tgtfpn);
    return -1;
  }

  /* 4) track page in replacement structure */
  enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn);

  *fpn = tgtfpn;
  return 0;
}

/*pg_getval - read a byte from virtual address 'addr' */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;
  int phyaddr;

  if (!caller || !data) return -1;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1;

  phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  if (MEMPHY_read(caller->krnl->mram, phyaddr, data) != 0)
    return -1;

  return 0;
}

/*pg_setval - write a byte to virtual address 'addr' */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;
  int phyaddr;

  if (!caller) return -1;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1;

  phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  if (MEMPHY_write(caller->krnl->mram, phyaddr, value) != 0)
    return -1;

  /* Optionally mark dirty bit in PTE if needed (not strictly required here) */
  return 0;
}

// #endif
