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

#include "system.h"
#include "paging.h"
#include <linux/types.h>
#include <linux/lguest.h>
#include <asm/lguest_hcall.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include "virtio_console.h"


void page_fault(registers_t regs)
{
    // A page fault has occurred.

	// Output an error message.
	put_chars("Page fault! ( ",14);

    // The faulting address is stored in the CR2 register.
    u32 faulting_address;
    faulting_address = lguest_read_cr2();

    // The error code gives us details of what happened.
    int present   = !(regs.err_code & 0x1); // Page not present
    int rw = regs.err_code & 0x2;           // Write operation?
    int us = regs.err_code & 0x4;           // Processor was in user-mode?
    int reserved = regs.err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
    int id = regs.err_code & 0x10;          // Caused by an instruction fetch?


    if (present) {put_chars("present ",8);}
    if (rw) {put_chars("read-only ",10);}
    if (us) {put_chars("user-mode ",10);}
    if (reserved) {put_chars("reserved ",9);}
    if (id) {put_chars("ifetch ",7);}
    put_chars(") at 0x",7);
    print_pointer(faulting_address);
    put_chars("\n",1);
    hcall(LHCALL_SHUTDOWN, __pa("PAGE FAULT"), LGUEST_SHUTDOWN_POWEROFF,0,0);
}

//lguest calls:
/*
* Intel has four control registers, imaginatively named cr0, cr2, cr3 and cr4.
* I assume there's a cr1, but it hasn't bothered us yet, so we'll not bother
* it. The Host needs to know when the Guest wants to change them, so we have
* a whole series of functions like read_cr0() and write_cr0().
*
* We start with cr0. cr0 allows you to turn on and off all kinds of basic
* features, but Linux only really cares about one: the horrifically-named Task
* Switched (TS) bit at bit 3 (ie. 8)
*
* What does the TS bit do? Well, it causes the CPU to trap (interrupt 7) if
* the floating point unit is used. Which allows us to restore FPU state
* lazily after a task switch, and Linux uses that gratefully, but wouldn't a
* name like "FPUTRAP bit" be a little less cryptic?
*
* We store cr0 locally because the Host never changes it. The Guest sometimes
* wants to read it and we'd prefer not to bother the Host unnecessarily.
*/
unsigned long current_cr0;
void lguest_write_cr0(unsigned long val)
{
hcall(LHCALL_TS, val & X86_CR0_TS,0,0,0);
current_cr0 = val;
}
unsigned long lguest_read_cr0(void)
{
return current_cr0;
}
/*
* Intel provided a special instruction to clear the TS bit for people too cool
* to use write_cr0() to do it. This "clts" instruction is faster, because all
* the vowels have been optimized out.
*/
void lguest_clts(void)
{
hcall(LHCALL_TS, 0,0,0,0);
current_cr0 &= ~X86_CR0_TS;
}
/*
* cr2 is the virtual address of the last page fault, which the Guest only ever
* reads. The Host kindly writes this into our "struct lguest_data", so we
* just read it out of there.
*/
unsigned long lguest_read_cr2(void)
{
return lguest_data.cr2;
}

unsigned long lguest_read_cr3(void)
{
return lguest_data.pgdir;
}

/* The Guest calls lguest_set_pmd to set a top-level entry when !PAE. (... we don't support PAE yet. patience!*/
void lguest_set_pgd(unsigned long *pgdp, unsigned long pgdval)
{
	*pgdp = pgdval;
	hcall(LHCALL_SET_PGD, __pa(pgdp) & PAGE_MASK,
				   (__pa(pgdp) & (PAGE_SIZE - 1)) / sizeof(unsigned long),0,0);

	lguest_flush_tlb_kernel();
}

/*
* This is what happens after the Guest has removed a large number of entries.
* This tells the Host that any of the page table entries for userspace might
* have changed, ie. virtual addresses below PAGE_OFFSET.
*/
void lguest_flush_tlb_user(void)
{
	hcall(LHCALL_FLUSH_TLB, 0,0,0,0);
}
/*
* This is called when the kernel page tables have changed. That's not very
* common (unless the Guest is using highmem, which makes the Guest extremely
* slow), so it's worth separating this from the user flushing above.
*/

void lguest_flush_tlb_kernel(void)
{
	hcall(LHCALL_FLUSH_TLB, 1,0,0,0);
}

void lguest_set_pte(unsigned long *ptep, unsigned long pteval)
{
//	native_set_pte(ptep, pteval);
	*ptep = pteval;
//	if (cr3_changed)
	lguest_flush_tlb_kernel();
}

//this function creates identity and linear mappings for DEVICE_PAGES
//we use a "random" page as the new page table. it is at 3mb in ram.
void map_device_pages(unsigned long devices)
{
	unsigned long *pgdir, pgd, *pgdp;
	unsigned long *ptable, *pte;

	//si esta la hago estatica se acaba el problema :)
	unsigned long new_page_table = 0xc0300000;

	pgdir = (unsigned long *) lguest_read_cr3();
	pgdir = __va(pgdir);

	//linear mapping
	pgdp = &pgdir[pgd_index((unsigned long)__va(devices))];
	lguest_set_pgd(pgdp, __pa(new_page_table) | _PAGE_PRESENT | _PAGE_RW | _PAGE_USER);

	// identity mapping
	pgdp = &pgdir[pgd_index(devices)];
	lguest_set_pgd(pgdp, __pa(new_page_table) | _PAGE_PRESENT | _PAGE_RW | _PAGE_USER);

	pgd = *pgdp;
	pgd &= PAGE_MASK;
	ptable = __va(pgd);

	int i, j;
	for (i = j = 0; j < 256;j++, i += PAGE_SIZE) {  //DEVICE_PAGES is 256
		pte = &ptable[pte_index(devices + i)];
		lguest_set_pte(pte, (devices + i)  | _PAGE_PRESENT|_PAGE_RW|_PAGE_USER);
	}

}
