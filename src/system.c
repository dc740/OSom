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
#include <asm/lguest_hcall.h>
#include <asm/paravirt_types.h>
#include <linux/lguest.h>
#include "system.h"

extern struct lguest_data lguest_data; //declared in init.c
unsigned int guest = 0;//0 = running on a pc. 1 = running under lguest
void *memcpy(void * destination, const void * source, int num){
    char* dst8 = (char*)destination;
    char* src8 = (char*)source;

        while (num--) {
            *dst8++ = *src8++;
        }
        return destination;
}

/**
 * I have no idea why, but it doesn't like my memset method.
 *
 * @param dest
 * @param val
 * @param count
 */
void *memset_int(void * dest, unsigned char val, int count){
    char* dst8 = (char*)dest;

        while (count--) {
            *dst8++ = val;
        }
        return dest;


}


/* STOP! Until an interrupt comes in. */
void lguest_safe_halt(void)
{
hcall(LHCALL_HALT, 0, 0, 0, 0);
}
/**
 * The SHUTDOWN hypercall takes a string to describe what's happening, and
 * an argument which says whether this to restart (reboot) the Guest or not.
 *
 * Note that the Host always prefers that the Guest speak in physical addresses
 * rather than virtual addresses, so we use __pa() here.
*/
extern void lguest_power_off(void)
{
hcall(LHCALL_SHUTDOWN, __pa("Power down"),
LGUEST_SHUTDOWN_POWEROFF, 0, 0);
}

/**
 * This is the first function run at startup. So we haven't touched anything in the system.
 * If we are running under privileged mode 1. we are a guest
 * @return
 */
unsigned int lguest_guest(void){
	unsigned int guestResult = 0;
	asm volatile("movl %%cs,%%eax;"
			"and $0x1,%%eax;"
			"movl %%eax,%0;":"=r"(guestResult)::"%eax");
	return guestResult;
}

/**
 * Tell hypervisor we are alive!
 */

void notify_lguest(void){

	//kvm_hypercall1(LHCALL_LGUEST_INIT, __pa(&lguest_data));
	hcall(LHCALL_LGUEST_INIT, __pa(&lguest_data),0,0,0);

}

