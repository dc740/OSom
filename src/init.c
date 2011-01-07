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
#include <linux/kvm_para.h>
#include <linux/kernel.h>
#include <asm/lguest_hcall.h>
#include <linux/lguest.h>
#include <linux/mm.h>
#include <asm/bootparam.h>
#include "system.h"
#include "virtio_console.h"
#include "descriptor_table.h"
#include "init.h"
#include <asm/paravirt.h>
#include "paging.h"
#include "isr.h"

extern void print_pointer (unsigned long p);
static void clear_bss(void);


struct lguest_data lguest_data = {
	.kernel_address = PAGE_OFFSET, //0xc0000000,
	.syscall_vec = 0x80,
};

unsigned long mem_top;
char welcome[] = "\n\nWelcome!\nThis is OSom\nA minimal guest for Lguest\n\n";
char status_clean_buff[] = "\nClearing input buffer...";

	

static void clear_bss(void){
	extern char _end_data, _end_bss;

	/*Initialize bss*/
	unsigned char *p = (unsigned char *) &_end_data;
	while (p < (unsigned char *)&_end_bss)
		*p++ = 0;
}


int main (void)
{
	struct boot_params *boot = 0;
	mem_top = boot->e820_map[0].size;


	clear_bss();

	//verify if we are a guest and then do whatever we need to do
	guest = lguest_guest();

	if (guest){
	notify_lguest();
	//ASSUME 4mb of ram, so devices should be after that !!!!
	unsigned long devices = mem_top; // 4 * 1024 * 1024;
	/* Say hi! don't use this function ever again. This is just as POC*/
	early_put_chars("Memory Detected: ", 17);
	early_print_pointer (mem_top);
	early_put_chars("\n", 1);

	//this function creates identity and linear mappings for DEVICE_PAGES
	//we use a "random" page as the new page table. it is at 3mb in ram. 
	map_device_pages(devices);

	//testing our virtio console "driver"
	scan_console_device(devices);

	init_static_out_buffer();
	init_static_in_buffer();
	add_inbuf();

	// I don't know why. but lguest prints some weird chars
	// the first time you get data form the input stream
	// so we get some dummy data first
	put_chars(status_clean_buff, sizeof(status_clean_buff));

	char inbuf[100];
	get_buffer_chars(inbuf, 100);// there is no need to try to get everything. 2^(sizeof(int) * 8));

	}

	//init descriptor tables:
	init_descriptor_tables();

	// Welcome screen:
	put_chars(welcome, sizeof(welcome));

	irq_enable();
	register_interrupt_handler(14, page_fault);

	//Run tiny console
	init_tiny_console();


	return 0;
}
