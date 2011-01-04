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

#include <linux/types.h>
#include "virtio_console.h"
#include "isr.h"
#include "system.h"
#include <asm/io.h>
#include <asm/lguest_hcall.h>
#include <linux/lguest.h>

extern struct lguest_data lguest_data; //declared in init.c
static void idt_set_gate(u8,u32,u16,u8);
static void idt_flush(u32 idt_pointer);
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

isr_t interrupt_handlers[256];
/*
* We can tell the Host what interrupts we want blocked ready for using the
* lguest_data.interrupts bitmap, so disabling (aka "masking") them is as
* simple as setting a bit.  We don't actually "ack" interrupts as such, we
* just mask and unmask them.  I wonder if we should be cleverer?
*/
void disable_lguest_irq(unsigned int irq)
{
	set_bit(irq, lguest_data.blocked_interrupts);
}

void enable_lguest_irq(unsigned int irq)
{
	clear_bit(irq, lguest_data.blocked_interrupts);
}


unsigned long save_fl(void)
{
	return lguest_data.irq_enabled;
}

/* Interrupts go off... */
void irq_disable(void)
{
	if (guest){
	lguest_data.irq_enabled = 0;
	}
	else{
	asm volatile ("cli");
	}
}

void irq_enable(void)
{
	if (guest){
	lguest_data.irq_enabled = X86_EFLAGS_IF;
	}
	else{
		asm volatile ("sti");
	}
}
void register_interrupt_handler(u8 n, isr_t handler)
{
	interrupt_handlers[n] = handler;
}

void reload_irq(u8 irqn){

	idt_entry_t *idt = idt_ptr.address;

	int i = 32 + irqn;
	hcall(LHCALL_LOAD_IDT_ENTRY, i, idt[i].a, idt[i].b, 0);
}


// This gets called from our ASM interrupt handler stub.
void isr_handler(registers_t regs)
{
	   if (interrupt_handlers[regs.int_no] != 0)
	   {
	       isr_t handler = interrupt_handlers[regs.int_no];
	       handler(regs);
	   }
}

// This gets called from our ASM interrupt handler stub.
void irq_handler(registers_t regs)
{

	   if (guest == 0){
		   // Send an EOI (end of interrupt) signal to the PICs.
		   // If this interrupt involved the slave.
		   if (regs.int_no >= 40)
		   {
			   // Send reset signal to slave.
			   outb(0xA0, 0x20);
		   }
		   // Send reset signal to master. (As well as slave, if necessary).
		   outb(0x20, 0x20);
	   }

   if (interrupt_handlers[regs.int_no] != 0)
   {
       isr_t handler = interrupt_handlers[regs.int_no];
       handler(regs);
   }
}



static void lguest_load_idt(idt_ptr_t *desc)
{
unsigned int i;
idt_entry_t *idt = desc->address;

/*
for (i = 0; i < 32;i++){//(desc->size+1)/8; i++)
hcall(LHCALL_LOAD_IDT_ENTRY, i, idt[i].a, idt[i].b, 0);
}
*/
for (i = 0; i < 47;i++){//(desc->size+1)/8; i++)   //48 interrupts!!!
hcall(LHCALL_LOAD_IDT_ENTRY, i, idt[i].a, idt[i].b, 0);
}

}

static void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags)
{
   idt_entries[num].base_lo = base & 0xFFFF;
   idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

   idt_entries[num].sel     = sel;
   idt_entries[num].always0 = 0;
   // We must uncomment the OR below when we get to using user-mode.
   // It sets the interrupt gate's privilege level to 3.
   idt_entries[num].flags   = flags /* | 0x60 */;
}



void init_idt(void)
{
   idt_ptr.size = sizeof(idt_entry_t) * 256 -1;
   idt_ptr.address  = (u32)&idt_entries;

   memset(&idt_entries, 0, sizeof(idt_entry_t)*256);

   // Remap the irq table.
   //so irqs0...irq15 will trigger ints 32...47
   if (guest == 0){
	   outb(0x20, 0x11);
	   outb(0xA0, 0x11);
	   outb(0x21, 0x20);
	   outb(0xA1, 0x28);
	   outb(0x21, 0x04);
	   outb(0xA1, 0x02);
	   outb(0x21, 0x01);
	   outb(0xA1, 0x01);
	   outb(0x21, 0x0);
	   outb(0xA1, 0x0);
   }

   idt_set_gate( 0, (u32)isr0 , 0x09, 0xAE); //0x8e = CPL=0... 0xAE = CPL = 1
   idt_set_gate( 1, (u32)isr1 , 0x09, 0xAE);
   idt_set_gate( 2, (u32)isr2 , 0x09, 0xAE);
   idt_set_gate( 3, (u32)isr3 , 0x09, 0xAE);
   idt_set_gate( 4, (u32)isr4 , 0x09, 0xAE);
   idt_set_gate( 5, (u32)isr5 , 0x09, 0xAE);
   idt_set_gate( 6, (u32)isr6 , 0x09, 0xAE);
   idt_set_gate( 7, (u32)isr7 , 0x09, 0xAE);
   idt_set_gate( 8, (u32)isr8 , 0x09, 0xAE);
   idt_set_gate( 9, (u32)isr9 , 0x09, 0xAE);
   idt_set_gate( 10, (u32)isr10 , 0x09, 0xAE);
   idt_set_gate( 11, (u32)isr11 , 0x09, 0xAE);
   idt_set_gate( 12, (u32)isr12 , 0x09, 0xAE);
   idt_set_gate( 13, (u32)isr13 , 0x09, 0xAE);
   idt_set_gate( 14, (u32)isr14 , 0x09, 0xAE);
   idt_set_gate( 15, (u32)isr15 , 0x09, 0xAE);
   idt_set_gate( 16, (u32)isr16 , 0x09, 0xAE);
   idt_set_gate( 17, (u32)isr17 , 0x09, 0xAE);
   idt_set_gate( 18, (u32)isr18 , 0x09, 0xAE);
   idt_set_gate( 19, (u32)isr19 , 0x09, 0xAE);
   idt_set_gate( 20, (u32)isr20 , 0x09, 0xAE);
   idt_set_gate( 21, (u32)isr21 , 0x09, 0xAE);
   idt_set_gate( 22, (u32)isr22 , 0x09, 0xAE);
   idt_set_gate( 23, (u32)isr23 , 0x09, 0xAE);
   idt_set_gate( 24, (u32)isr24 , 0x09, 0xAE);
   idt_set_gate( 25, (u32)isr25 , 0x09, 0xAE);
   idt_set_gate( 26, (u32)isr26 , 0x09, 0xAE);
   idt_set_gate( 27, (u32)isr27 , 0x09, 0xAE);
   idt_set_gate( 28, (u32)isr28 , 0x09, 0xAE);
   idt_set_gate( 29, (u32)isr29 , 0x09, 0xAE);
   idt_set_gate( 30, (u32)isr30 , 0x09, 0xAE);
   idt_set_gate( 31, (u32)isr31 , 0x09, 0xAE);

   //IRQ handlers!
   idt_set_gate(32, (u32)irq0, 0x09, 0xAE);
   idt_set_gate(33, (u32)irq1, 0x09, 0xAE);
   idt_set_gate(34, (u32)irq2, 0x09, 0xAE);
   idt_set_gate(35, (u32)irq3, 0x09, 0xAE);
   idt_set_gate(36, (u32)irq4, 0x09, 0xAE);
   idt_set_gate(37, (u32)irq5, 0x09, 0xAE);
   idt_set_gate(38, (u32)irq6, 0x09, 0xAE);
   idt_set_gate(39, (u32)irq7, 0x09, 0xAE);
   idt_set_gate(40, (u32)irq8, 0x09, 0xAE);
   idt_set_gate(41, (u32)irq9, 0x09, 0xAE);
   idt_set_gate(42, (u32)irq10, 0x09, 0xAE);
   idt_set_gate(43, (u32)irq11, 0x09, 0xAE);
   idt_set_gate(44, (u32)irq12, 0x09, 0xAE);
   idt_set_gate(45, (u32)irq13, 0x09, 0xAE);
   idt_set_gate(46, (u32)irq14, 0x09, 0xAE);
   idt_set_gate(47, (u32)irq15, 0x09, 0xAE);

   if (guest == 0){
	   idt_flush((u32)&idt_ptr);
   }
   else{
	   lguest_load_idt((u32)&idt_ptr);
   }

}

static void idt_flush(u32 idt_pointer){
	asm volatile("movl %0,%%eax;"
			"lidt (%%eax);":"=r"(idt_pointer)::"%eax");
}


