AS=nasm
ASFLAGS=-f bin

%.bin: %.s
	${AS} ${ASFLAGS} -o $@ -l $*.lst $^


.PHONY: real_image.img
real_image.img: boot.bin
	cat ./boot.bin ./boot.img > ./real_boot.img

.PHONY: clean
clean:
	rm -f boot.bin
	rm -f boot.lst
	rm -f real_boot.img

.PHONY: all
all: clean boot.bin 
