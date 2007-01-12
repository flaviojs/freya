#!make
# $Id: configure 570 2005-12-01 23:15:33Z Yor $

%.o: %.c
	@echo -e "    CC\t$<"
	@$(CC) $(CFLAGS) -c -o $@ $<

all:
	@echo Please run make in the top directory

../../char-server.exe:  char.o inter.o int_party.o int_guild.o int_storage.o int_pet.o addons.o  ../lib/libcommon.a
	@echo -e "    LD\t$@"
	@$(CC)   $(LDFLAGS) -o $@  char.o inter.o int_party.o int_guild.o int_storage.o int_pet.o addons.o  ../lib/libcommon.a $(LIBS)

char.o: char.c ../include/config.h ../common/core.h ../common/socket.h \
  ../common/timer.h ../include/mmo.h ../include/../common/utils.h \
  ../common/version.h ../common/lock.h char.h ../common/malloc.h \
  ../common/utils.h ../common/addons.h ../common/console.h inter.h \
  int_pet.h int_guild.h int_party.h int_storage.h
inter.o: inter.c ../include/config.h ../include/mmo.h \
  ../include/../common/utils.h ../common/socket.h ../common/timer.h \
  char.h inter.h int_party.h int_guild.h int_storage.h int_pet.h \
  ../common/lock.h ../common/malloc.h
int_party.o: int_party.c ../include/config.h inter.h int_party.h \
  ../include/mmo.h ../include/../common/utils.h char.h ../common/socket.h \
  ../common/db.h ../common/lock.h ../common/malloc.h
int_guild.o: int_guild.c ../include/config.h ../include/mmo.h \
  ../include/../common/utils.h ../common/db.h ../common/lock.h \
  ../common/malloc.h ../common/socket.h char.h inter.h int_guild.h \
  int_storage.h
int_storage.o: int_storage.c ../include/config.h int_storage.h int_pet.h \
  int_guild.h ../include/mmo.h ../include/../common/utils.h char.h \
  ../common/socket.h ../common/lock.h ../common/malloc.h
int_pet.o: int_pet.c ../include/config.h inter.h int_pet.h \
  ../include/mmo.h ../include/../common/utils.h char.h ../common/socket.h \
  ../common/db.h ../common/lock.h ../common/malloc.h
addons.o: addons.c ../include/config.h ../common/addons.h addons.h

clean:
	rm -f *.o char.o inter.o int_party.o int_guild.o int_storage.o int_pet.o addons.o ../../char-server.exe

