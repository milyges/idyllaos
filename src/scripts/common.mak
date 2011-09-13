
HOSTCC=gcc
BUILD:=$(shell $(HOSTCC) -dumpmachine)
BUILD_ARCH:=$(shell uname -m)
ARCH ?= $(IDYLLAOS_ARCH)

HOST:=$(ARCH)-pc-idyllaos
CROSS_COMPILER=
ifneq ($(HOST),$(BUILD))
	CROSS_COMPILER:=$(HOST)-
endif

AR:=$(CROSS_COMPILER)ar
AS:=$(CROSS_COMPILER)gcc
CC:=$(CROSS_COMPILER)gcc
NM:=$(CROSS_COMPILER)nm
LD:=$(CROSS_COMPILER)gcc
STRIP:=$(CROSS_COMPILER)strip

CFLAGS+=-pipe -Wall -O2

OBJECTS:=$(SOURCES:.c=.o)
OBJECTS:=$(OBJECTS:.S=.o)

.SUFFIXES: .S .c

.S.o:
	@echo " AS      $@"
	@$(AS) $(ASFLAGS) -c -o $@ $<

.c.o:
	@echo " CC      $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

