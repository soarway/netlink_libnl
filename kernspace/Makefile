obj-m := kernspace.o
THIS_DIR := $(shell pwd)
KERNEL_DIR := /usr/src/kernels/4.18.0-240.10.1.el8_3.x86_64
.phony: all
all: kernspace.ko

kernspace.ko: $(KOBJS) Makefile
	$(MAKE) -C $(KERNEL_DIR) M=$(THIS_DIR) modules c=2

.phony: clean
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(THIS_DIR) clean
