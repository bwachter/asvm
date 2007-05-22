ALL=asvm svc

all: $(ALL)

#DIET?=/opt/diet/bin/diet
CC?=gcc
CFLAGS=-Wall -W -pipe -fomit-frame-pointer -Os -I../ibaard/src 
LDFLAGS=-L../ibaard -libaard

CROSS=
#LDFLAGS=-s

asvm: asvm.o 
	$(DIET) $(CROSS)$(CC) -o $@ $^ $(LDFLAGS)

svc: svc.o
	$(DIET) $(CROSS)$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $<

.PHONY: all clean install
clean:
	rm -f *.o $(ALL)

install:
	install -m 755 asvm $(DESTDIR)/usr/sbin
	install -m 755 svc $(DESTDIR)/usr/bin
	ln -s $(DESTDIR)/usr/bin/svc $(DESTDIR)/usr/bin/svstat
	ln -s $(DESTDIR)/usr/bin/svc $(DESTDIR)/usr/bin/svok