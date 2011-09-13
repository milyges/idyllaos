## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = cygnus

INCLUDES = -I$(srcdir)/include -I$(srcdir)/machine/$(machine_dir)/include $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS) $(INCLTDL)


SUBDIRS = machine \
	$(EXTRA_SUBDIRS)\
	.

SUBLIBS = \
	$(EXTRA_SUBLIBS) \
	$(IDYLLAOS_MACH_LIB)

GENERAL_SOURCES = \
	chdir.c close.c execve.c _exit.c fork.c fstat.c getdents.c getpid.c gettimeofday.c \
	kill.c link.c lseek.c malloc.c mallocr.c mmap.c munmap.c open.c read.c stat.c unlink.c \
	waitpid.c write.c lstat.c access.c dup.c dup2.c umask.c sysconf.c pathconf.c chmod.c \
	fcntl.c utime.c chown.c mkdir.c rmdir.c ftruncate.c ioctl.c termios.c select.c \
	getlogin.c gethostname.c mknod.c mkfifo.c pipe.c getppid.c getuid.c geteuid.c getgid.c \
	getegid.c alarm.c setuid.c setgid.c getpwent.c getgrent.c vfork.c mount.c umount.c \
	nanosleep.c sigaction.c uname.c


ADD_OBJS = \
	$(IDYLLAOS_MACH_LIB)

libidyllaos_la_LDFLAGS = -Xcompiler -nostdlib

if USE_LIBTOOL
noinst_LTLIBRARIES = libidyllaos.la
libidyllaos_la_SOURCES = $(GENERAL_SOURCES)
libidyllaos_la_LIBADD = $(ADD_OBJS)
libidyllaos_la_DEPENDENCIES = $(ADD_OBJS)
LIBC_COMPILE = $(LTCOMPILE)
noinst_DATA = objectlist.awk.in
else
noinst_LIBRARIES = lib.a
lib_a_SOURCES = $(GENERAL_SOURCES)
lib_a_LIBADD = $(ADD_OBJS)
lib_a_DEPENDENCIES = $(ADD_OBJS)
lib_a_CFLAGS = $(AM_CFLAGS)
LIBC_COMPILE = $(COMPILE)
noinst_DATA =

lib.a:  $(lib_a_OBJECTS)
	rm -f $@
	rm -rf tmp
	mkdir tmp
	cd tmp; \
	for i in $(SUBLIBS); do \
	  $(AR) x ../$$i; \
	done;
	$(AR) $(AR_FLAGS) $@ $(lib_a_OBJECTS) tmp/*.o
	$(RANLIB) $@
	rm -rf tmp

endif # USE_LIBTOOL

objectlist.awk.in: $(noinst_LTLIBRARIES) $(SUBLIBS)
	-rm -f objectlist.awk.in
	for i in `ls *.lo` ; \
	  do \
	    echo $$i `pwd`/$$i >> objectlist.awk.in ; \
	done
	for j in $(SUBLIBS) ; \
	  do \
	    export i=`echo $$j | sed -e 's,\(.*\)\/[^\/]*$$,\1,'`; \
	    if test $$i != "." && test -f $$i/objectlist.awk.in; then \
	    	cat $$i/objectlist.awk.in >> objectlist.awk.in ; \
	    fi; \
	done

all-local: crt0.o

crt0.o: machine/crt0.o
	rm -f $@
	ln machine/crt0.o $@ >/dev/null 2>/dev/null || cp machine/crt0.o $@

machine/crt0.o: ; @true

AM_CFLAGS = -I $(srcdir)/../../stdio
ACLOCAL_AMFLAGS = -I ../../.. -I ../../../..
CONFIG_STATUS_DEPENDENCIES = $(newlib_basedir)/configure.host
