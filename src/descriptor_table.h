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

#ifndef DESCRIPTOR_TABLE_H_
#define DESCRIPTOR_TABLE_H_

// This structure contains the value of one GDT entry.
// We use the attribute 'packed' to tell GCC not to change
// any of the alignment in the structure.

// I was going to use a custom made GDT struct... but... why if someone else has done it before...
// So we used the linux kernel defined structs
typedef struct desc_struct gdt_entry_t;
typedef struct desc_ptr gdt_ptr_t;

// Initialization function is publicly accessible.
void init_descriptor_tables(void);

#endif /* DESCRIPTOR_TABLE_H_ */
