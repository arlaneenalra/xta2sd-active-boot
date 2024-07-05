AS=nasm
ASFLAGS=-f bin



.PHONY: all
all: clean boot/boot.bin tool.com

%.bin: %.s
	${AS} ${ASFLAGS} -o $@ -l $*.lst $^

%.com: %.s
	${AS} ${ASFLAGS} -o $@ -l $*.lst $^



.PHONY: real_image.img
real_image.img: boot/boot.bin
	cat ./boot/boot.bin ./boot.img > ./real_boot.img

.PHONY: clean
clean:
	rm -f boot/*.bin
	rm -f boot/*.lst
	rm -f *.com
	rm -f real_boot.img


