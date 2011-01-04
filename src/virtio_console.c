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

#include <linux/virtio.h>
#include <linux/virtio_ring.h>
#include <linux/virtio_console.h>
#include <linux/lguest_launcher.h>

//this is used in the print function. which should be removed
#include <asm/lguest_hcall.h>

#include "system.h"
#include "isr.h"
#include "paging.h"
#include "virtio_console.h"

#define to_vvq(_vq) container_of(_vq, struct vring_virtqueue, vq)

inline long kvm_hypercall2(unsigned int nr, unsigned long p1, unsigned long p2);
void print_pointer (unsigned long p);
static void execute(char * command);

extern char *welcome;
char static_out_buffer[100];
char static_in_buffer[100];

vq_callback_t *callbacks[] = { NULL, NULL}; //la primera era hvc_handle_input
const char *names[] = { "input", "output" };
struct virtqueue *vqs[2];
struct virtqueue *in_vq, *out_vq;
static unsigned int in_len;
static char *in, *inbuf;

#define set_cs( cs ) asm volatile ( "ljmp %0, $fake_label \n\t fake_label: \n\t" :: "i"(cs) )

/*D:140 This is the information we remember about each virtqueue. */
struct lguest_vq_info
{
	/* A copy of the information contained in the device config. */
	struct lguest_vqconfig config;

	/* The address where we mapped the virtio ring, so we can unmap it. */
	void *pages;
} in_vq_info, out_vq_info;

struct lguest_device {
	struct virtio_device vdev;
	/* The entry in the lguest_devices page for this device. */
	struct lguest_device_desc *desc;
} console;


struct vring_virtqueue
{
	struct virtqueue vq;
	/* Actual memory layout for this queue */
	struct vring vring;
	/* Other side has made a mess, don't try any more. */
	bool broken;
	/* Host supports indirect buffers */
	bool indirect;
	/* Number of free buffers */
	unsigned int num_free;
	/* Head of free buffer list. */
	unsigned int free_head;
	/* Number we've added since last sync. */
	unsigned int num_added;
	/* Last used index we've seen. */
	u16 last_used_idx;
	/* How to notify other side. FIXME: commonalize hcalls! */
	void (*notify)(struct virtqueue *vq);
	/* Tokens for callbacks. */
	void *data[];
} vring_in_vq, vring_out_vq;

inline bool more_used(const struct vring_virtqueue *vq)
{
	return vq->last_used_idx != vq->vring.used->idx;
}

static void vring_kick(struct virtqueue *_vq)
{
	struct vring_virtqueue *vq = to_vvq(_vq);
	wmb();
	vq->vring.avail->idx ++; //= vq->num_added;
	vq->num_added = 0;

	/* Need to update avail index before checking if we should notify */
	mb();
	vq->notify(&vq->vq);
}

static struct lguest_vqconfig *lg_vq(const struct lguest_device_desc *desc)
{
	return (void *)(desc + 1);
}

static void *vring_get_buf(struct virtqueue *_vq, unsigned int *len)
{
	struct vring_virtqueue *vq = to_vvq(_vq);
	void *ret;
	unsigned int i;
	static unsigned long long counter = 0;
	static once = 0;

	if (!once) {
		once++;
		early_put_chars("\n VRING.USED: ", 1);
		early_print_pointer(vq->vring.used);
		early_put_chars("\n", 1);
	}

	if (!more_used(vq)) {
		return NULL;
	}

//	i = vq->vring.used->ring[vq->last_used_idx%vq->vring.num].id;
	*len = vq->vring.used->ring[vq->last_used_idx%vq->vring.num].len;

//	if (unlikely(i >= vq->vring.num)) {
//		print("id out of range\n", 10);
//		return NULL;
//	}
//	if (unlikely(!vq->data[i])) {
//		print("id is not a head!\n", 15);
//		return NULL;
//	}

	/* detach_buf clears data, so grab it now. */
	ret = vq->vring.desc[0].addr;//vq->data[0];

	vq->last_used_idx++;
//	print ("\nlast_used_idx incremented: ", 28);
//	print_pointer(vq->last_used_idx);
	return ret;
}

static struct virtqueue_ops vring_vq_ops = {
	.kick = vring_kick,
	.get_buf = vring_get_buf,
};

static void lg_notify(struct virtqueue *vq)
{
	/* We store our virtqueue information in the "priv" pointer of the
	 * virtqueue structure. */
	struct lguest_vq_info *lvq = vq->priv;
	kvm_hypercall2(17, lvq->config.pfn << PAGE_SHIFT, 0); //LHCALL_NOTIFY
}

struct virtqueue *vring_new_virtqueue(unsigned int num,
				      unsigned int vring_align,
				      struct virtio_device *vdev,
				      void *pages,
				      void (*notify)(struct virtqueue *),
				      void (*callback)(struct virtqueue *),
				      const char *name)
{
	struct vring_virtqueue *vq;
	unsigned int i;

	/* We assume num is a power of 2. */
	if (num & (num - 1)) {
		early_put_chars("Bad virtqueue length\n", 15);
		return NULL;
	}

	if (!strcmp(name, "input")){
		vq = &vring_in_vq;
//		print ("input", 5);

	}
	else{
		vq = &vring_out_vq;
//		print ("output", 6);
	}

	vring_init(&vq->vring, num, pages, vring_align);

	vq->vq.callback = callback;
	vq->vq.vdev = vdev;
	vq->vq.vq_ops = &vring_vq_ops;
	vq->vq.name = name;
	vq->notify = notify;
	vq->broken = false;
	vq->last_used_idx = 0;
	vq->num_added = 0;
	list_add_tail(&vq->vq.list, &vdev->vqs);

	/* No callback?  Tell other side not to bother us. */
//	if (!callback)
	vq->vring.avail->flags |= VRING_AVAIL_F_NO_INTERRUPT;

	/* Put everything in free lists. */
	vq->num_free = num;
	vq->free_head = 0;
	for (i = 0; i < num-1; i++)
		vq->vring.desc[i].next = i+1;

	return &vq->vq;
}

static struct virtqueue *lg_find_vq(struct virtio_device *vdev,
				    struct lguest_device *ldev,
				    unsigned index,
				    void (*callback)(struct virtqueue *vq),
				    const char *name)
{
//	struct lguest_device *ldev = to_lgdev(vdev);
	struct virtqueue *vq;
	int err;

	struct lguest_vq_info *lvq;

	if (!strcmp(name, "input"))
		lvq = &in_vq_info;
	else
		lvq = &out_vq_info;

	/* We must have this many virtqueues. */
	if (index >= ldev->desc->num_vq)
		return -1;

	/* Make a copy of the "struct lguest_vqconfig" entry, which sits after
	 * the descriptor.  We need a copy because the config space might not
	 * be aligned correctly. */
	__builtin_memcpy(&lvq->config, lg_vq(ldev->desc)+index, sizeof(lvq->config));

	/* Figure out how many pages the ring will take, and map that memory */

	/* aca nos dicen la dir física y ioremap la mapearía a una dir virtual del kernel
           pero nosotros no soportamos ioremap, entonces vamos a meter los dispositivos en nuestra
           memoria y este paso pasa a ser trivial*/
//	lvq->pages = lguest_map((unsigned long)lvq->config.pfn << PAGE_SHIFT,
//			DIV_ROUND_UP(vring_size(lvq->config.num, LGUEST_VRING_ALIGN), PAGE_SIZE));
	lvq->pages = lvq->config.pfn << PAGE_SHIFT;
	
	/* OK, tell virtio_ring.c to set up a virtqueue now we know its size
	 * and we've got a pointer to its pages. */
	vq = vring_new_virtqueue(lvq->config.num, LGUEST_VRING_ALIGN, vdev, lvq->pages, lg_notify, 						callback, name);
	vq->priv = lvq;
	return vq;

destroy_vring:
unmap:
free_lvq:
	return -1;
}

void scan_console_device (void *lguest_devices)  //dir virtual o fisica del frame despues de mem
{

	struct lguest_device_desc *d = lguest_devices;
	struct lguest_device *ldev = &console;  // la hacemos estática
	struct virtio_device *vdev = &ldev->vdev;

	ldev->desc = d;
	int nvqs = 2, i;

	vdev->id.device = d->type;

	INIT_LIST_HEAD(&vdev->vqs);

	for (i = 0; i < nvqs; ++i) {
		vqs[i] = lg_find_vq(vdev, ldev, i, NULL, names[i]);
	}
	lguest_flush_tlb_kernel();
	in_vq = vqs[0];
	out_vq = vqs[1];
	lguest_flush_tlb_kernel();
}


void init_static_out_buffer(void)
{
	struct vring_virtqueue *vq = to_vvq(out_vq);
	vq->vring.desc[0].flags = 0;// = VRING_DESC_F_NEXT;
	vq->vring.desc[0].addr = (unsigned long) static_out_buffer - PAGE_OFFSET;
	vq->vring.desc[0].len = sizeof(static_out_buffer);
	lguest_flush_tlb_kernel();
}

void init_static_in_buffer(void)
{
	struct vring_virtqueue *vq = to_vvq(in_vq);
	vq->vring.desc[0].flags = VRING_DESC_F_WRITE;// = VRING_DESC_F_NEXT;
	vq->vring.desc[0].addr = (unsigned long) static_in_buffer - PAGE_OFFSET;
	vq->vring.desc[0].len = sizeof(static_in_buffer);
	lguest_flush_tlb_kernel();
}

/*D:310 The put_chars() callback is pretty straightforward.
 *
 * We turn the characters into a scatter-gather list, add it to the output
 * queue and then kick the Host.  Then we sit here waiting for it to finish:
 * inefficient in theory, but in practice implementations will do it
 * immediately (lguest's Launcher does). */
int put_chars(const char *buf, int count)
{

	__builtin_memcpy (static_out_buffer, buf, count);
	struct vring_virtqueue *vq = to_vvq(out_vq);
	vq->vring.desc[0].len = count;
	int len;

	/* add_buf wants a token to identify this buffer: we hand it any
	 * non-NULL pointer, since there's only ever one buffer. */
	//if (out_vq->vq_ops->add_buf(out_vq, sg, 1, 0, (void *)1) == 0) {
		/* Tell Host to go! */
	out_vq->vq_ops->kick(out_vq);
	lguest_flush_tlb_kernel();


	/* Chill out until it's done with the buffer. */
	while (!more_used(vq))
		cpu_relax();
	//}
	vq->last_used_idx++;
	///////

	return count;
}


/* Create a scatter-gather list representing our input buffer and put it in the
 * queue. */
void add_inbuf(void)
{

//	ponemos nuestro unico buffer estatico en la pos 0 y ahora avisamos que
//	esta disponible
	init_static_in_buffer();
	in_vq->vq_ops->kick(in_vq);
	lguest_flush_tlb_kernel();
}


/*D:350 get_chars() is the callback from the hvc_console infrastructure when
 * an interrupt is received.
 *
 * Most of the code deals with the fact that the hvc_console() infrastructure
 * only asks us for 16 bytes at a time.  We keep in_offset and in_used fields
 * for partially-filled buffers. */
int get_buffer_chars(char *buf, int count)
{

	in = in_vq->vq_ops->get_buf(in_vq, &in_len);
	lguest_flush_tlb_kernel();

	if (in_len) {
		__builtin_memcpy(buf, in, in_len);
		add_inbuf();
		count = in_len;

		in_len = 0;
		return count;
	}
	else
		return 0;
}
/**
 * Returns the next character from the standard input (stdin).
 * @return The character read is returned as an int value.
 */
int getchar(void){
	char inbuf;
	while (! get_buffer_chars(&inbuf,1) ); //Just wait until there is something in the buffer
	return inbuf;
}
/**
 * Reads characters from stdin and stores them as a string into str  until a newline character ('\n') or the End-of-File is reached.
 * The ending newline character ('\n') is not included in the string.
 * A null character ('\0') is automatically appended after the last character copied to str to signal the end of the C string.
 * @param str
 * @return On success, the function returns the same str parameter.
 */
char * gets ( char * str ){
	char c;
	char * dest = str;
	while ( (c = getchar()) != '\n'){
		*dest++ = c;
	}
	*dest = '\0';
	return str; //we don't actually handle any errors.
}


/* My early_put_chars()
 * This is just temporal and should NOT be used nor exported
 *  */
int early_put_chars (const char *buf, int count)
{
	unsigned char scratch[100];  //why just 17 in boot.c ?
	unsigned int len = count;

	if (len > sizeof(scratch) - 1)
		len = sizeof(scratch) - 1;
	scratch[len] = '\0';
	__builtin_memcpy (scratch, buf, len);
	//kvm_hypercall2(LHCALL_NOTIFY, __pa(scratch), 0);

	hcall(LHCALL_NOTIFY,__pa(scratch),0,0,0);

	return len;
}

void early_print_pointer (unsigned long p)
{
	char buf[9];
	memset (buf, '0', sizeof(buf));
	unsigned long a = p;
	unsigned char h, i = 7;
	buf[8] = 0;

	while (a) {
		h = a & 0xf;
		if (h < 0xa)
			buf[i] = h + '0';
		else
			buf[i] = 'A' + h - 0xa;
		a >>= 4;
		i--;
	}
	early_put_chars (buf, 9);
}

void print_pointer (unsigned long p)
{
	char buf[9];
	__builtin_memset (buf, '0', sizeof(buf));
	unsigned long a = p;
	unsigned char h, i = 7;
	buf[8] = 0;

	while (a) {
		h = a & 0xf;
		if (h < 0xa)
			buf[i] = h + '0';
		else
			buf[i] = 'A' + h - 0xa;
		a >>= 4;
		i--;
	}
	put_chars (buf, 9);
}




//so... yes, lines have a maximun size of 500 chars
//do you really need more than that?
char linebuffer[500];
char consoleshell[100] = {"shell>"};

void init_tiny_console(void){

	int shelllen = strlen(consoleshell);
	//main loop
	while (1){
	put_chars(consoleshell,shelllen);
	char * buffer = linebuffer;
	char * endofarray = &linebuffer[499];

	//we stay inside this loop until the user wants to execute the command (hits enter)
	while(buffer < endofarray){
		char c = getchar();
		switch (c){
			case '\n': //enter
				*buffer= '\0';
				put_chars(&c,1);
				break;
			case '\b': case 0x7F: //backspace or delete
				if (buffer > linebuffer){
				buffer--;
				*buffer='\0';
				//this is a workaround. we print the command in a new line
				char newline = '\n';
				put_chars(&newline,1);
				put_chars(consoleshell,shelllen);
				put_chars(linebuffer, strlen(linebuffer));
				}
				break;
			default:
				if (buffer != endofarray) {
					*buffer = c;
					buffer++;
					put_chars(&c,1);
				}
			}

		if (c == '\n') break; //break the while statement! the user want's to execute!
	}

	//Now we should have a command in the linebuffer
	//of course. we have hardcoded commands... we don't "execute" anything really
	execute(linebuffer);
	}



}

void test_irq0(registers_t regs){
	put_chars("Timer works...\n",15);
}

static void execute(char * command){

	if (strcmp(command,"shutdown") == 0){
		hcall(LHCALL_SHUTDOWN, __pa("Power down"), LGUEST_SHUTDOWN_POWEROFF,0,0);
	}
	else if (strcmp(command,"test") == 0){
		put_chars("It works...\n",12);
	}
	else if (strcmp(command,"irq") == 0){
		//we never link interrupt 32 (irq0) with the handler!!!! that's what's happening!!!!!! get a coffee and keep working! now it's fixed!!!
		register_interrupt_handler(32,test_irq0);
		enable_lguest_irq(0);
		unsigned long delta =100UL;
	/* Please wake us this far in the future. */
	hcall(LHCALL_SET_CLOCKEVENT, delta, 0, 0, 0);
	}
	else if (strcmp(command,"cs") == 0){

		put_chars("cs:",3);
		asm volatile("movw %%cs,%%ax;"
				"pushl %%ax;"
				"call print_pointer;"
				"add $4, %%esp":::"%ax");
		put_chars("\n",1);
	}
	else if (strcmp(command,"ss") == 0){

		put_chars("ss:",3);
		asm volatile("movw %%ss,%%ax;"
				"pushl %%ax;"
				"call print_pointer;"
				"add $4, %%esp":::"%ax");
		put_chars("\n",1);
	}
	else if (strcmp(command,"emilio") == 0){
		put_chars("What have you done?!\n",21);
		u32 *ptr = (u32*)0xA0000000;
		u32 do_page_fault = *ptr;
		print_pointer(do_page_fault);
	}

}




