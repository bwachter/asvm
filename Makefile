ALL=asvm svc

all: $(ALL)

#DIET?=/opt/diet/bin/diet
STRIP?=$(CROSS)strip
CC?=cc
CFLAGS?=-std=c99 -D_GNU_SOURCE -Wall -W -pipe -fomit-frame-pointer
CFLAGS+=-Wall -Iibaard/src
LDFLAGS+=-Libaard/lib -libaard
VERSION?=`date +%Y%m%d`
Q?=@

ifdef NDK_TARGET
CFLAGS+=-target $(NDK_TARGET) -D_POSIX_SOURCE=1
LDFLAGS+=-target $(NDK_TARGET)
endif

#LDFLAGS=-s
version.h:
	$(Q)echo "$@"
	$(Q)echo "#define VERSION \"$(VERSION)\"" > version.h

asvm: ibaard/lib/libibaard.a asvm.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) -o $@ $^ $(LDFLAGS)
	$(Q)$(STRIP) $@

svc: ibaard/lib/libibaard.a svc.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) -o $@ $^ $(LDFLAGS)
	$(Q)$(STRIP) $@

%.o: %.c version.h
	$(Q)echo "CC $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $<

ibaard/lib/libibaard.a:
	$(Q)cd ibaard && make DIET="$(DIET)" CROSS="$(CROSS)" CFLAGS="$(CFLAGS)" CC="$(CC)" dep && make

.PHONY: all clean install
clean:
	$(Q)cd ibaard && make clean
	$(Q)rm -f *.o $(ALL) version.h

install:
	$(Q)echo "$@"
	$(Q)mkdir -p $(DESTDIR)/usr/bin $(DESTDIR)/usr/sbin $(DESTDIR)/etc/asvm/services
	$(Q)install -m 755 asvm $(DESTDIR)/usr/sbin
	$(Q)install -m 755 svc $(DESTDIR)/usr/bin
	$(Q)ln -f -s /usr/bin/svc $(DESTDIR)/usr/bin/svstat
	$(Q)ln -f -s /usr/bin/svc $(DESTDIR)/usr/bin/svok
	$(Q)mkfifo $(DESTDIR)/etc/asvm/in $(DESTDIR)/etc/asvm/out

dist: version.h
	$(Q)echo "building archive (asvm-$(VERSION).tar.bz2)"
	$(Q)git-archive-all.sh --format tar --prefix asvm-$(VERSION)/  asvm-$(VERSION).tar
	$(Q)gzip -f asvm-$(VERSION).tar
