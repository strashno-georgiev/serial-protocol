
all: Exe/HIDEcho1


Exe/HIDEcho1: Application/HIDEcho1.c USBHID/USBHID_linux.c
	cc -Wall -O -IUSBHID $^ -o $@ -ludev
