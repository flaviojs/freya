#!make
# $Id: configure 570 2005-12-01 23:15:33Z Yor $

%.o: %.c
	@echo -e "    CC\t$<"
	@$(CC) $(CFLAGS) -c -o $@ $<

all:  adduser.exe

adduser.exe: adduser.c ../include/config.h ../common/utils.h
	@echo -e " CC LD\tadduser.c ../../tool/adduser.exe"
	@$(CC) $(CFLAGS) $(LDFLAGS)   -o ../../tool/adduser.exe adduser.c $(LIBS)
	@touch adduser.exe

clean:
	rm -f *.o adduser.exe ../../tool/adduser.exe

