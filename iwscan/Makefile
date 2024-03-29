##
## Please check the configurion parameters below
##

## Installation directory. By default, go in /usr/local.
## Distributions should probably use /, but they probably know better...
ifndef PREFIX
  PREFIX = /usr/local
endif

#CC = gcc
## Change below to the location of your arm compiler
CC = /home/saga/pi/staging/.build/arm-unknown-linux-gnueabi/buildtools/bin/arm-unknown-linux-gnueabi-gcc

AR = ar
RANLIB = ranlib

## Uncomment this to build tools using static version of the library.
## Mostly useful for embedded platforms without ldd, or to create
## a local version (non-root).
## Standard distros should comment that option to save space and to
## build libiw.so used by third parties...
BUILD_STATIC = y

## Uncomment this to strip binary from symbols. This reduce binary size.
## by a few percent but make debug worse...
 BUILD_STRIPPING = y


# ***************************************************************************
# ***** Most users should not need to change anything beyond this point *****
# ***************************************************************************

# Version of the Wireless Tools
WT_VERSION := $(shell sed -ne "/WT_VERSION/{s:\([^0-9]*\)::;p;q;}" < iwlib.h )

# Version of Wireless Extensions.
WE_VERSION := $(shell sed -ne "/WE_VERSION/{s:\([^0-9]*\)::;p;q;}" < iwlib.h )

# Always use local header for wireless extensions
WEXT_HEADER = wireless.$(WE_VERSION).h

# Targets to build
STATIC=libiw.a
DYNAMIC=libiw.so.$(WT_VERSION)
PROGS= iwscan
EXTRAPROGS= macaddr iwmulticall

# Composition of the library :
OBJS = iwlib.o

# Select which library to build and to link tool with
ifdef BUILD_STATIC
  IWLIB=$(STATIC)
  IWLIB_INSTALL=install-static
else
  IWLIB=$(DYNAMIC)
  IWLIB_INSTALL=install-dynamic
endif

# Standard name for dynamic library so that the dynamic linker can pick it.
# We will just create a symbolic link to the real thing.
DYNAMIC_LINK= libiw.so

# Install directories
INSTALL_DIR= $(PREFIX)/sbin
INSTALL_LIB= $(PREFIX)/lib
INSTALL_INC= $(PREFIX)/include
INSTALL_MAN= $(PREFIX)/man

# Various commands
RM = rm -f
RM_CMD = $(RM) *.BAK *.bak *.d *.o *.so ,* *~ *.a *.orig *.rej *.out
LDCONFIG = ldconfig

# Do we want to build with or without libm ?
ifdef BUILD_NOLIBM
  LIBS=
  WELIB_FLAG= -DWE_NOLIBM=y
else
  LIBS= -lm
endif

# Stripping or not ?
ifdef BUILD_STRIPPING
  STRIPFLAGS= -Wl,-s
else
  STRIPFLAGS=
endif

# Do we want to build with only essential functionality ?
ifdef BUILD_WE_ESSENTIAL
  WEDEF_FLAG= -DWE_ESSENTIAL=y
endif

# Other flags
CFLAGS=-Os -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow \
	-Wpointer-arith -Wcast-qual -Winline -I.
#CFLAGS=-O2 -W -Wall -Wstrict-prototypes -I.
DEPFLAGS=-MMD
XCFLAGS=$(CFLAGS) $(DEPFLAGS) $(WARN) $(HEADERS) $(WELIB_FLAG) $(WEDEF_FLAG)
PICFLAG=-fPIC

# Standard compilation targets
all:: $(IWLIB) $(PROGS)

%: %.o
	$(CC) $(LDFLAGS) $(STRIPFLAGS) $(XCFLAGS) -o $@ $^ $(LIBS)
%.o: %.c wireless.h
	$(CC) $(XCFLAGS) -c $<
%.so: %.c wireless.h
	$(CC) $(XCFLAGS) $(PICFLAG) -c -o $@ $<

iwscan: iwscan.o $(IWLIB)

# Always do symbol stripping here
iwmulticall: iwmulticall.o
	$(CC) $(LDFLAGS) -Wl,-s $(XCFLAGS) -o $@ $^ $(LIBS)

# It's a kind of magic...
wireless.h:
	cp $(WEXT_HEADER) wireless.h

# Compilation of the dynamic library
$(DYNAMIC): $(OBJS:.o=.so)
	$(CC) -shared -o $@ -Wl,-soname,$@ $(STRIPFLAGS) $(LIBS) -lc $^

# Compilation of the static library
$(STATIC): $(OBJS:.o=.so)
	$(RM) $@
	$(AR) cru $@ $^
	$(RANLIB) $@

# Installation : So crude but so effective ;-)
# Less crude thanks to many contributions ;-)
install:: $(IWLIB_INSTALL) install-bin install-hdr

# Install the dynamic library
install-dynamic:: $(DYNAMIC)
	install -m 755 -d $(INSTALL_LIB)
	install -m 755 $(DYNAMIC) $(INSTALL_LIB)
	ln -sfn $(DYNAMIC) $(INSTALL_LIB)/$(DYNAMIC_LINK)
	@echo "*** Don't forget to add $(INSTALL_LIB) to /etc/ld.so.conf, and run ldconfig as root. ***"
	@$(LDCONFIG) || echo "*** Could not run ldconfig ! ***"

# Install the static library
install-static:: $(STATIC)
	install -m 755 -d $(INSTALL_LIB)
	install -m 644 $(STATIC) $(INSTALL_LIB)

# All the binaries. Careful, no dependancy on install-dynamic
install-bin:: all
	install -m 755 -d $(INSTALL_DIR)
	install -m 755 $(PROGS) $(INSTALL_DIR)

# Headers to go with the wireless lib (dev)
install-hdr:: wireless.h
	install -m 755 -d $(INSTALL_INC)
	install -m 644 iwlib.h $(INSTALL_INC)
	install -m 644 wireless.h $(INSTALL_INC)

clean::
	$(RM_CMD) 

realclean::
	$(RM_CMD) 
	$(RM) $(STATIC) $(DYNAMIC) $(PROGS) $(EXTRAPROGS) libiw* wireless.h

# Include dependancies
-include *.d
