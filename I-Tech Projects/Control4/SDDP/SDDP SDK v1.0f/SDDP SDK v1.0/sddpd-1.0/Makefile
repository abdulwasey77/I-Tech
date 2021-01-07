CC = gcc
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

EXE = sddpd
OBJS = Sddp.o SddpNetwork.o SddpPacket.o SddpServer.o

all : $(EXE)

$(EXE) : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(EXE)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean :
	rm -f $(EXE) *.[od]


