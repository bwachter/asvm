ALL=asvm svc

all: $(ALL)

#DIET?=/opt/diet/bin/diet
CC?=cc
CFLAGS=-std=c99 -Wall -W -pipe -fomit-frame-pointer -Os -Iibaard/src
LDFLAGS=-Libaard -libaard
VERSION=`date +%Y%m%d`
Q?=@

#LDFLAGS=-s
version.h:
	$(Q)echo "$@"
	$(Q)echo "#define VERSION \"$(VERSION)\"" > version.h

asvm: ibaard/libibaard.a version.h asvm.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) -o $@ $^ $(LDFLAGS)
	$(Q)$(CROSS)strip $@

svc: ibaard/libibaard.a version.h svc.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) -o $@ $^ $(LDFLAGS)
	$(Q)$(CROSS)strip $@

%.o: %.c
	$(Q)echo "CC $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $<

ibaard/libibaard.a:
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
	$(Q)echo "building archive ($(VERSION).tar.bz2)"
	$(Q)git-archive-all.sh --format tar --prefix $(VERSION)/  $(VERSION).tar
	$(Q)gzip -f $(VERSION).tar
