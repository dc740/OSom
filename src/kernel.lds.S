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
OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")
ENTRY(start)			/* Kernel starts at "start" symbol. */
PHYS_ADDR = 0X00100000;
VIRT_ADDR = 0XC0100000;

SECTIONS {

/*
Using the multtiboot specifications, we set the mbHeader at the begining of the .text section.
So we don't need an extra section for that
http://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Header-address-fields

   .__mbHeader : AT ( ADDR( .__mbHeader ) - VIRT_ADDR ) {
    	_start = .;
    	_mboot = .;
    *(.__mbHeader)
    . = ALIGN(4);
   } = 0x90
*/
  .text VIRT_ADDR : AT(PHYS_ADDR) {
    *(.__mbHeader)
  	*(.text)
  	*(.rodata) *(.rodata.*)
  	. = ALIGN(4096);
  } = 0x90

  .data : {
  		_data = .;
  		*(.data)
  		. = ALIGN(4096);
  }

  /* BSS (zero-initialized data) is after everything else. */

  .bss : {
 	_end_data = .;
  *(.bss)
  . = ALIGN(4096);
  	_end_bss = .;
  }


  _end = .;
}
