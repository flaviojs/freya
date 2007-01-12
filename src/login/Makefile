#!make
# $Id: configure 570 2005-12-01 23:15:33Z Yor $

%.o: %.c
	@echo -e "    CC\t$<"
	@$(CC) $(CFLAGS) -c -o $@ $<

all:
	@echo Please run make in the top directory

../../login-server.exe:  login.o lutils.o ladmin.o addons.o  ../lib/libcommon.a
	@echo -e "    LD\t$@"
	@$(CC)   $(LDFLAGS) -o $@  login.o lutils.o ladmin.o addons.o  ../lib/libcommon.a $(LIBS)

login.o: login.c ../include/config.h ../common/core.h ../common/socket.h \
  ../common/timer.h ../common/version.h ../common/malloc.h \
  ../common/lock.h ../common/addons.h ../common/console.h \
  ../common/utils.h ../common/md5calc.h login.h ../include/mmo.h \
  ../include/../common/utils.h ladmin.h lutils.h
lutils.o: lutils.c ../include/config.h ../include/mmo.h \
  ../include/../common/utils.h ../common/timer.h lutils.h login.h
ladmin.o: ladmin.c ../include/config.h ../common/socket.h \
  ../common/core.h ../common/timer.h ../include/mmo.h \
  ../include/../common/utils.h ../common/version.h ../common/lock.h \
  ../common/malloc.h ../common/md5calc.h login.h lutils.h
addons.o: addons.c ../include/config.h ../common/addons.h \
  ../common/timer.h login.h ../include/mmo.h ../include/../common/utils.h \
  addons.h

clean:
	rm -f *.o login.o lutils.o ladmin.o addons.o ../../login-server.exe

