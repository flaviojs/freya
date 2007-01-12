#!make
# $Id: configure 570 2005-12-01 23:15:33Z Yor $

%.o: %.c
	@echo -e "    CC\t$<"
	@$(CC) $(CFLAGS) -c -o $@ $<

all:
	@echo Please run make in the top directory

libcommon.a:  core.o console.o socket.o timer.o db.o lock.o utils.o db_mysql.o addons.o md5calc.o 
	@echo -e "    AR\t$@"
	@$(AR) rcs $@  core.o console.o socket.o timer.o db.o lock.o utils.o db_mysql.o addons.o md5calc.o
	@cp $@ ../lib

core.o: core.c ../include/config.h core.h socket.h timer.h version.h \
  console.h utils.h
console.o: console.c ../include/config.h console.h keycodes.h
socket.o: socket.c ../include/config.h ../include/mmo.h \
  ../include/../common/utils.h socket.h malloc.h console.h
timer.o: timer.c ../include/config.h timer.h malloc.h console.h
db.o: db.c ../include/config.h db.h malloc.h
lock.o: lock.c ../include/config.h lock.h utils.h
utils.o: utils.c ../include/config.h utils.h ../common/timer.h
db_mysql.o: db_mysql.c ../include/config.h
addons.o: addons.c ../include/config.h ../common/malloc.h \
  ../common/timer.h addons.h addons_exports.h core.h
md5calc.o: md5calc.c ../include/config.h md5calc.h

clean:
	rm -f *.o core.o console.o socket.o timer.o db.o lock.o utils.o db_mysql.o addons.o md5calc.o libcommon.a
	rm -f ../lib/libcommon.a

