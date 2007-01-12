#!make
# $Id: configure 570 2005-12-01 23:15:33Z Yor $

%.o: %.c
	@echo -e "    CC\t$<"
	@$(CC) $(CFLAGS) -c -o $@ $<

all:
	@echo Please run make in the top directory

../../ladmin.exe:  ladmin.o  ../lib/libcommon.a
	@echo -e "    LD\t$@"
	@$(CC)   $(LDFLAGS) -o $@  ladmin.o  ../lib/libcommon.a $(LIBS)

ladmin.o: ladmin.c ../include/config.h ../common/core.h ../common/timer.h \
  ../common/socket.h ladmin.h ../common/version.h ../include/mmo.h \
  ../include/../common/utils.h ../common/md5calc.h

clean:
	rm -f *.o ladmin.o ../../ladmin.exe

