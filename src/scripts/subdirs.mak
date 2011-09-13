
MAKEFLAGS += --no-print-directory

all:
	@$(foreach SUBDIR,$(SUBDIRS),\
	make -C $(SUBDIR) all; )

clean:
	@$(foreach SUBDIR,$(SUBDIRS),\
	make -C $(SUBDIR) clean; )

install:
	@$(foreach SUBDIR,$(SUBDIRS),\
	make -C $(SUBDIR) install; )
