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

/*
 * The Global Descriptor Table.
 *
 * The Intel architecture defines another table, called the Global Descriptor
 * Table (GDT).  You tell the CPU where it is (and its size) using the "lgdt"
 * instruction, and then several other instructions refer to entries in the
 * table.  There are three entries which the Switcher needs, so the Host simply
 * controls the entire thing and the Guest asks it to make changes using the
 * LOAD_GDT hypercall.
 *
 */

#include "descriptor_table.h"
#include "init.h"
#include "isr.h"
#include <asm-generic/int-ll64.h>
#include <asm/desc_defs.h>
#include <asm/lguest_hcall.h>


// Internal function prototypes.
static void init_gdt(void);
static void lguest_load_gdt(const struct desc_ptr *desc);
extern void setup_gdt (void);
extern void print_pointer (unsigned long p);
extern int early_put_chars (const char *buf, int count);


#define set_cs( cs ) asm volatile ( "ljmp %0, $fake_label \n\t fake_label: \n\t" :: "i"(cs) )

// IDT related functions:
/*
#define SET_IDT_GATE(idt,ring,s,addr) \
	(idt).off1 = addr & 0xffff; \
	(idt).off2 = addr >> 16; \
	(idt).sel = s; \
	(idt).none = 0; \
	(idt).flags = 0x8E | (ring << 5); \

	struct idt {
        unsigned short off1;
        unsigned short sel;
        unsigned char none,flags;
        unsigned short off2;
} __attribute__ ((packed));
*/
#define SET_IDT_GATE(idt,flags,sel,base) \
		(idt).a = ((base) & 0xffff) | (((sel) & 0xffff) << 16); \
		(idt).b = (0xFFFFFF00 & ((((flags) & 0xff) << 8) | ((base & 0xFFFF0000) << 16))); \


/*
 * useless, just a POC
#define SET_IDT_GATE(flags,sel,base) { { { \
		.a = ((base) & 0xffff) | (((sel) & 0xffff) << 16), \
		.b = (0xFFFFFF00 & ((((flags) & 0xff) << 8) | ((base & 0xFFFF0000) << 16))), \
		} } }
*/





//GDT format:
//flags,base,limit
//in C09A, 0 is ignored, don't worry about it.
//0xC = granularity
//0x9A = access

/**
 *  [GDT_ENTRY_KERNEL_CS]		= GDT_ENTRY_INIT(0xc09a, 0, 0xfffff),
 *  [GDT_ENTRY_KERNEL_DS]		= GDT_ENTRY_INIT(0xc092, 0, 0xfffff),
 *
 */
static gdt_entry_t gdt_entries[5] = {GDT_ENTRY_INIT(0x0,0x0,0x0),GDT_ENTRY_INIT(0xC0BA,0x0,0xFFFFFFFF),GDT_ENTRY_INIT(0xC0B2,0x0,0xFFFFFFFF),GDT_ENTRY_INIT(0xC0FA,0x0,0xFFFFFFFF),GDT_ENTRY_INIT(0xC0F2,0x0,0xFFFFFFFF)};
static gdt_ptr_t   gdt_ptr;
//we r gonna init the IDT too... just because I can
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;


// Initialization routine - zeroes all the interrupt service routines,
// initializes the GDT and IDT.
void init_descriptor_tables()
{
	early_put_chars("Setting up GDT\n",15);
	// Initialise the global descriptor table.
   init_gdt();
   early_put_chars("Setting up IDT\n",15);
   init_idt();
}

static void init_gdt()
{
   gdt_ptr.size = (sizeof(gdt_entry_t) * 5 -1); //we have 5 entries
   gdt_ptr.address  = (u32)&gdt_entries;

   const struct desc_ptr * gdt_address = &gdt_ptr;
   lguest_load_gdt(gdt_address);

   int cs = 0x09;
   int ds = 0x11;

   set_cs(cs);

   asm volatile ("movw  %0,%%ax;"
			"movw %%ax,%%ds;"
			"movw %%ax,%%es;"
			"movw %%ax,%%fs;"
			"movw %%ax,%%gs;"
			"movw %%ax,%%ss;" :: "i"(ds):"%ax"
   );


}


static void lguest_load_gdt(const struct desc_ptr *desc)
{
unsigned int i;
struct desc_struct *gdt = (void *)desc->address;
for (i = 0; i < (unsigned int) (desc->size+1)/8; i++)
hcall(LHCALL_LOAD_GDT_ENTRY, i, gdt[i].a, gdt[i].b, 0);
}
