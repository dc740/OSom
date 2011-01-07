
include config_defines.txt

# I couldn't find stdarg.h, so I hardcoded the include file -I/usr/lib/gcc/i486-linux-gnu/4.4/include/

INCLUDE:= -I/lib/modules/`uname -r`/build/include -I/lib/modules/`uname -r`/build/arch/x86/include/ -I/usr/lib/gcc/i486-linux-gnu/4.4/include/

FLAGS:= -g -m32 -c -msoft-float -O -fno-stack-protector -fno-builtin-memset-nostdinc -D __KERNEL__ $(CONFIG_DEFINES) $(INCLUDE) -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers -Wall -W

BUILD_DIR:= ./build/

SRC_DIR:= ./src/

toy: $(SRC_DIR)kernel.lds.S start.o init.o virtio_console.o system.o descriptor_table.o isr.o isr_asm.o paging.o
	$(LD) --whole-archive -melf_i386 -T $< -o $(BUILD_DIR)$@  $(BUILD_DIR)init.o $(BUILD_DIR)paging.o $(BUILD_DIR)isr_asm.o $(BUILD_DIR)start.o $(BUILD_DIR)virtio_console.o $(BUILD_DIR)system.o $(BUILD_DIR)descriptor_table.o $(BUILD_DIR)isr.o  

init.o: $(SRC_DIR)init.c
	gcc $(FLAGS) -o $(BUILD_DIR)$@ $<
	
paging.o: $(SRC_DIR)paging.c
	gcc $(FLAGS) -o $(BUILD_DIR)$@ $<
	
isr.o: $(SRC_DIR)isr.c
	gcc $(FLAGS) -o $(BUILD_DIR)$@ $<

isr_asm.o: $(SRC_DIR)isr_asm.S
	gcc $(FLAGS) -o $(BUILD_DIR)$@ $<

start.o: $(SRC_DIR)start.S
	gcc $(FLAGS) -o $(BUILD_DIR)$@ $<

system.o: $(SRC_DIR)system.c
	gcc $(FLAGS) -o $(BUILD_DIR)$@ $<

virtio_console.o: $(SRC_DIR)virtio_console.c
	gcc $(FLAGS) -o $(BUILD_DIR)$@ $<
	
descriptor_table.o: $(SRC_DIR)descriptor_table.c
	gcc $(FLAGS) -o $(BUILD_DIR)$@ $<
		
all: toy
	
clean:
	rm -f $(BUILD_DIR)*

	

