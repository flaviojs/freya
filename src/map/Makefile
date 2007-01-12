#!make
# $Id: configure 570 2005-12-01 23:15:33Z Yor $

%.o: %.c
	@echo -e "    CC\t$<"
	@$(CC) $(CFLAGS) -c -o $@ $<

all:
	@echo Please run make in the top directory

../../map-server.exe:  map.o nullpo.o grfio.o chrif.o clif.o pc.o npc.o chat.o path.o itemdb.o mob.o script.o storage.o skill.o atcommand.o battle.o intif.o trade.o party.o vending.o guild.o pet.o status.o mail.o addons.o ranking.o  ../lib/libcommon.a
	@echo -e "    LD\t$@"
	@$(CC)   $(LDFLAGS) -o $@  map.o nullpo.o grfio.o chrif.o clif.o pc.o npc.o chat.o path.o itemdb.o mob.o script.o storage.o skill.o atcommand.o battle.o intif.o trade.o party.o vending.o guild.o pet.o status.o mail.o addons.o ranking.o  ../lib/libcommon.a $(LIBS)

map.o: map.c ../include/config.h ../common/core.h ../common/timer.h \
  ../common/db.h ../common/malloc.h ../common/socket.h ../common/addons.h \
  ../common/console.h ../common/version.h grfio.h map.h ../include/mmo.h \
  ../include/../common/utils.h chrif.h clif.h storage.h intif.h npc.h \
  pc.h mob.h chat.h itemdb.h skill.h trade.h party.h battle.h script.h \
  guild.h pet.h atcommand.h nullpo.h status.h ../include/gettext.h
nullpo.o: nullpo.c ../include/config.h nullpo.h
grfio.o: grfio.c ../include/config.h ../common/utils.h grfio.h \
  ../include/mmo.h ../include/../common/utils.h ../common/malloc.h
chrif.o: chrif.c ../include/config.h ../common/socket.h ../common/timer.h \
  ../common/malloc.h map.h ../include/mmo.h ../include/../common/utils.h \
  battle.h chrif.h clif.h storage.h intif.h npc.h pc.h nullpo.h \
  atcommand.h script.h status.h trade.h vending.h ranking.h
clif.o: clif.c ../include/config.h ../common/socket.h ../common/core.h \
  ../common/timer.h ../common/malloc.h ../common/version.h nullpo.h map.h \
  ../include/mmo.h ../include/../common/utils.h chrif.h clif.h storage.h \
  pc.h npc.h itemdb.h chat.h trade.h script.h skill.h atcommand.h intif.h \
  battle.h mob.h party.h guild.h vending.h pet.h status.h ranking.h
pc.o: pc.c ../include/config.h ../common/socket.h ../common/timer.h \
  ../common/db.h ../common/malloc.h ../common/lock.h ../common/utils.h \
  map.h ../include/mmo.h ../include/../common/utils.h chrif.h clif.h \
  storage.h intif.h pc.h npc.h mob.h pet.h itemdb.h script.h battle.h \
  skill.h party.h guild.h chat.h trade.h vending.h nullpo.h atcommand.h \
  status.h ranking.h
npc.o: npc.c ../include/config.h ../common/db.h ../common/timer.h \
  nullpo.h ../common/malloc.h map.h ../include/mmo.h \
  ../include/../common/utils.h npc.h clif.h storage.h intif.h pc.h \
  itemdb.h script.h mob.h pet.h battle.h skill.h grfio.h status.h \
  atcommand.h chrif.h
chat.o: chat.c ../include/config.h ../common/db.h nullpo.h \
  ../common/malloc.h map.h ../include/mmo.h ../include/../common/utils.h \
  clif.h storage.h pc.h chat.h npc.h atcommand.h
path.o: path.c ../include/config.h map.h ../include/mmo.h \
  ../include/../common/utils.h battle.h nullpo.h
itemdb.o: itemdb.c ../include/config.h ../common/db.h ../common/utils.h \
  ../common/malloc.h grfio.h nullpo.h map.h ../include/mmo.h \
  ../include/../common/utils.h battle.h itemdb.h script.h pc.h
mob.o: mob.c ../include/config.h ../common/timer.h ../common/socket.h \
  ../common/db.h ../common/malloc.h ../common/utils.h nullpo.h map.h \
  ../include/mmo.h ../include/../common/utils.h clif.h storage.h intif.h \
  pc.h mob.h guild.h itemdb.h skill.h battle.h party.h npc.h status.h \
  atcommand.h ranking.h
script.o: script.c ../include/config.h ../common/socket.h \
  ../common/timer.h ../common/malloc.h ../common/lock.h ../common/db.h \
  map.h ../include/mmo.h ../include/../common/utils.h clif.h storage.h \
  chrif.h itemdb.h pc.h script.h mob.h npc.h pet.h intif.h skill.h chat.h \
  battle.h party.h guild.h atcommand.h status.h ../include/gettext.h
storage.o: storage.c ../include/config.h ../common/socket.h \
  ../common/db.h itemdb.h map.h ../include/mmo.h \
  ../include/../common/utils.h clif.h storage.h chrif.h intif.h pc.h \
  guild.h atcommand.h nullpo.h ../common/malloc.h
skill.o: skill.c ../include/config.h ../common/timer.h ../common/malloc.h \
  ../common/utils.h nullpo.h skill.h map.h ../include/mmo.h \
  ../include/../common/utils.h clif.h storage.h pc.h pet.h mob.h battle.h \
  party.h itemdb.h script.h intif.h chrif.h guild.h atcommand.h grfio.h \
  status.h ranking.h
atcommand.o: atcommand.c ../include/config.h ../common/socket.h \
  ../common/timer.h ../common/malloc.h ../common/core.h \
  ../common/version.h nullpo.h clif.h map.h ../include/mmo.h \
  ../include/../common/utils.h storage.h chrif.h intif.h itemdb.h pc.h \
  skill.h mob.h pet.h battle.h party.h guild.h atcommand.h script.h npc.h \
  trade.h grfio.h status.h
battle.o: battle.c ../include/config.h battle.h ../common/timer.h \
  ../common/malloc.h nullpo.h map.h ../include/mmo.h \
  ../include/../common/utils.h pc.h skill.h mob.h itemdb.h clif.h \
  storage.h pet.h guild.h party.h status.h
intif.o: intif.c ../include/config.h ../common/socket.h ../common/timer.h \
  map.h ../include/mmo.h ../include/../common/utils.h battle.h chrif.h \
  clif.h storage.h pc.h intif.h party.h guild.h pet.h nullpo.h \
  atcommand.h ../common/malloc.h
trade.o: trade.c ../include/config.h ../common/socket.h clif.h map.h \
  ../include/mmo.h ../include/../common/utils.h storage.h itemdb.h \
  trade.h pc.h npc.h battle.h chrif.h intif.h atcommand.h nullpo.h
party.o: party.c ../include/config.h party.h ../common/db.h \
  ../common/timer.h ../common/socket.h nullpo.h ../common/malloc.h pc.h \
  map.h ../include/mmo.h ../include/../common/utils.h battle.h intif.h \
  clif.h storage.h
vending.o: vending.c ../include/config.h ../common/socket.h \
  ../common/malloc.h clif.h map.h ../include/mmo.h \
  ../include/../common/utils.h storage.h itemdb.h vending.h pc.h skill.h \
  battle.h nullpo.h chrif.h intif.h atcommand.h
guild.o: guild.c ../include/config.h ../common/db.h ../common/timer.h \
  ../common/socket.h ../common/malloc.h ../common/utils.h guild.h \
  storage.h ../include/mmo.h ../include/../common/utils.h nullpo.h \
  battle.h npc.h map.h pc.h mob.h intif.h clif.h skill.h atcommand.h \
  chrif.h status.h
pet.o: pet.c ../include/config.h ../common/db.h ../common/timer.h \
  ../common/socket.h ../common/malloc.h ../common/utils.h nullpo.h pc.h \
  map.h ../include/mmo.h ../include/../common/utils.h intif.h clif.h \
  storage.h chrif.h pet.h itemdb.h battle.h mob.h npc.h script.h skill.h \
  status.h
status.o: status.c ../include/config.h ../common/socket.h \
  ../common/timer.h ../common/utils.h map.h ../include/mmo.h \
  ../include/../common/utils.h nullpo.h itemdb.h pc.h skill.h battle.h \
  clif.h storage.h script.h guild.h mob.h chrif.h atcommand.h pet.h \
  status.h ranking.h
mail.o: mail.c ../include/config.h
addons.o: addons.c ../include/config.h ../common/addons.h addons.h
ranking.o: ranking.c ranking.h map.h ../include/config.h ../include/mmo.h \
  ../include/../common/utils.h clif.h storage.h chrif.h

clean:
	rm -f *.o map.o nullpo.o grfio.o chrif.o clif.o pc.o npc.o chat.o path.o itemdb.o mob.o script.o storage.o skill.o atcommand.o battle.o intif.o trade.o party.o vending.o guild.o pet.o status.o mail.o addons.o ranking.o ../../map-server.exe

