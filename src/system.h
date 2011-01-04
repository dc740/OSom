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

#ifndef __SYSTEM_H
#define __SYSTEM_H


//It's strange. this function IS defined. but it seems that I have to redefine it cause the linker can't find it

void *memcpy(void * destination, const void * source, int num);
//somehow gcc changes the funtion name to __built_in_memset and the linker can't find it. just change the name.
void *memset_int(void *dest, unsigned char val, int count);
/*
extern unsigned short *memsetw(unsigned short *dest, unsigned short val, int count);
extern int strlen(const char *str);
extern unsigned char inportb (unsigned short _port);
extern void outportb (unsigned short _port, unsigned char _data);
*/
unsigned int lguest_guest(void);
void lguest_safe_halt(void);
void lguest_power_off(void);
void notify_lguest(void);

extern unsigned int guest;
#endif

