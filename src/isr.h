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
#ifndef ISR_H_
#define ISR_H_
#include <linux/types.h>

void init_idt(void);

#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

typedef struct registers
{
   u32 ds;                  // Data segment selector
   u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
   u32 int_no, err_code;    // Interrupt number and error code (if applicable)
   u32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t;

struct idt_entry_struct
{
	union {
			struct {
				u32 a;
				u32 b;
			};
			struct{
			   u16 base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
			   u16 sel;                 // Kernel segment selector.
			   u8  always0;             // This must always be zero.
			   u8  flags;               // More flags. See documentation. unsigned always01110:5, dpl:2,p:1;
			   u16 base_hi;             // The upper 16 bits of the address to jump to.
			};
	};
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

struct idt_ptr_struct {
	unsigned short size;
	unsigned long address;
} __attribute__((packed)) ;
typedef struct idt_ptr_struct idt_ptr_t;

// Enables registration of callbacks for interrupts or IRQs.
// For IRQs, to ease confusion, use the #defines above as the
// first parameter.
typedef void (*isr_t)(registers_t);

void register_interrupt_handler(u8 n, isr_t handler);
void disable_lguest_irq(unsigned int irq);
void enable_lguest_irq(unsigned int irq);
unsigned long save_fl(void);
void irq_disable(void);
void irq_enable(void);

void isr0 ();
void isr1 ();
void isr2 ();
void isr3 ();
void isr4 ();
void isr5 ();
void isr6 ();
void isr7 ();
void isr8 ();
void isr9 ();
void isr10 ();
void isr11 ();
void isr12 ();
void isr13 ();
void isr14 ();
void isr15 ();
void isr16 ();
void isr17 ();
void isr18 ();
void isr19 ();
void isr20 ();
void isr21 ();
void isr22 ();
void isr23 ();
void isr24 ();
void isr25 ();
void isr26 ();
void isr27 ();
void isr28 ();
void isr29 ();
void isr30 ();
void isr31 ();
void irq0 ();
void irq1 ();
void irq2 ();
void irq3 ();
void irq4 ();
void irq5 ();
void irq6 ();
void irq7 ();
void irq8 ();
void irq9 ();
void irq10 ();
void irq11 ();
void irq12 ();
void irq13 ();
void irq14 ();
void irq15 ();


#endif /* ISR_H_ */
