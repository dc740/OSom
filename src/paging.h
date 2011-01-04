/*
 *    This file is part of OSom. A simple guest for lguest32.
 *    2010, By: Emilio Moretti; Matias Zabaljauregui
 *
 *    OSom is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PAGING_H
#define PAGING_H

#include <linux/types.h>

#include "isr.h"

/**
   Handler for page faults.
**/
void page_fault(registers_t regs);

/**
 * Set page directory entry:
 * locate the entry
 * 	pgdp = &pgdir[pgd_index(devices)];
 * and then call set_pgd with the new value.
 * 	lguest_set_pgd(pgdp, __pa(new_page_table) | _PAGE_PRESENT | _PAGE_RW | _PAGE_USER);
 */
void lguest_set_pgd(unsigned long *pgdp, unsigned long pgdval);

/**
 * Set page table entry
 */

void lguest_set_pte(unsigned long *ptep, unsigned long pteval);

void lguest_write_cr0(unsigned long val);
void lguest_write_cr3(unsigned long val);
unsigned long lguest_read_cr0(void);
unsigned long lguest_read_cr2(void);
unsigned long lguest_read_cr3(void);

void lguest_flush_tlb_user(void);
void lguest_flush_tlb_kernel(void);
/**
 * Intel provided a special instruction to clear the TS bit for people too cool
 * to use write_cr0() to do it. This "clts" instruction is faster, because all
 * the vowels have been optimized out.
 */
void lguest_clts(void);

/**
 * this function creates identity and linear mappings for DEVICE_PAGES
 * we use a "random" page as the new page table. it is at 3mb in ram.
 */
void map_device_pages(unsigned long devices);


#endif
