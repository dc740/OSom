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

#ifndef VIRTIO_CONSOLE_H_
#define VIRTIO_CONSOLE_H_
void scan_console_device (void *lguest_devices);
void init_static_out_buffer(void);
void init_static_in_buffer(void);
void add_inbuf(void);
int put_chars(const char *buf, int count);
void print_pointer (unsigned long p);
int get_buffer_chars(char *buf, int count);
int getchar(void);
char * gets ( char * str );
void init_tiny_console(void);

#endif /* VIRTIO_CONSOLE_H_ */
