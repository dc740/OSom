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

#ifndef THREADS_INIT_H
#define THREADS_INIT_H
/*
#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Physical memory size, in 4 kB pages.
extern size_t ram_pages;*/

/* Page directory with kernel mappings only.
extern uint32_t *base_page_dir;*/

/* -q: Power off when kernel tasks complete?
extern bool power_off_when_done;*/

/*
void power_off (void) NO_RETURN;
void reboot (void);*/
extern void irq_disable(void);


#endif /* threads/init.h */
