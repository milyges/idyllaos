MAKEFLAGS += --no-print-directory

all: all-kernel all-sbin all-bin
	@true

clean: clean-kernel clean-sbin clean-bin
	@true

install: install-kernel install-sbin install-bin
	@true

disk: all
	@mkdir -p .tmp
	@sudo losetup /dev/loop0 -o 32256 disk.img
	@-sudo fsck.ext2 -y /dev/loop0
	@sudo mount -text2 /dev/loop0 .tmp
	@sudo DESTDIR=$(PWD)/.tmp PATH="$(PATH):$(PWD)/../toolchain/bin" make install
	@-sudo bash
	@sudo umount .tmp
	@sudo losetup -d /dev/loop0
	@rm -rf .tmp

vdisk: disk
	@rm -f disk.vdi
	@VBoxManage convertfromraw -format VDI disk.img disk.vdi

qtest: disk
	@qemu -hda disk.img -m 256 -boot c -smp 1 -s -no-reboot -vga std

all-kernel:
	@make -C sys all

install-kernel:
	@make -C sys install modules_install

clean-kernel:
	@make -C sys clean


all-sbin:
	@make -C sbin all

install-sbin:
	@make -C sbin install

clean-sbin:
	@make -C sbin clean

all-bin:
	@make -C bin all

install-bin:
	@make -C bin install

clean-bin:
	@make -C bin clean
