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



.equ MBOOT_PAGE_ALIGN, 1<<0    // Load kernel and modules on a page boundary
.equ MBOOT_MEM_INFO, 1<<1    // Provide your kernel with memory info
.equ MBOOT_ADDRESS_FIELD, 1<<16
.equ MBOOT_HEADER_MAGIC, 0x1BADB002 // Multiboot Magic value
// NOTE: We do not use MBOOT_AOUT_KLUDGE. It means that GRUB does not
// pass us a symbol table.
.equ MBOOT_HEADER_FLAGS, MBOOT_MEM_INFO | MBOOT_PAGE_ALIGN //| MBOOT_ADDRESS_FIELD
.equ MBOOT_CHECKSUM, -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

.global mboot,bss,end,_start,start

.section .__mbHeader
_start:
	jmp start
_mboot:
.align 4
    .long MBOOT_HEADER_MAGIC
    .long MBOOT_HEADER_FLAGS
    .long MBOOT_CHECKSUM

/*
    // AOUT kludge - must be physical addresses. Make a note of these:
    // The linker script fills in the data for these ones!
    .long _mboot
    .long _start
    .long _end_data
    .long _end_bss
    .long start*/
.section .text
.func start
start:	
	movl $0xc0400000, %esp  # initial stack pointer
	#lets call print to print some things
	#push 10			#count
	#push print_buffer
	#call early_put_chars
	call main		# jump to C code
littleloop:	jmp littleloop			# should not return, but if it does, spin.
.endfunc


.section .data

var1:
      .string "hola mundo!\n";



