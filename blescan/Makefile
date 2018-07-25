#DEBUG = -O0 -Xlinker
DEBUG = -O1 -Wno-unused-variable
CC = gcc
AS = as
AR = ar
SRC_DIR_0 = /home/blescan
INCLUDE = -I/usr/local/include /home/blescan
CFLAGS = $(DEBUG) -Wall $(INCLUDE) -Winline -pipe
LDFLAGS = $(DEBUG) -lrt -lbcm2835 -lcrypto -lpthread -lbluetooth -lcurses -lzmq
##############################################################################
OBJS=blescan.c utils.c zeroadv.c jsmn/jsmn.c
OBJS2=sha1.c
OBJS3=sha2.c
OBJS4=sha3.c

part: $(OBJS)
	$(AR) $(OBJS)
	rm -rf $(SRC_DIR_0)/*.o
%.o:%.c $(INCLUDE)
	$(CC) -c -o $@
all:
	$(CC) $(OBJS) -o blescan $(LDFLAGS)
sha1:
	$(CC) $(OBJS2) -o sha1 $(LDFLAGS)
sha2:
	$(CC) $(OBJS3) -o sha2 $(LDFLAGS)
sha3:
	$(CC) $(OBJS4) -o sha3 $(LDFLAGS)

.PHONY : clean
clean:
	rm -rf $(SRC_DIR_0)/*.o

      # DO NOT DELETE 
