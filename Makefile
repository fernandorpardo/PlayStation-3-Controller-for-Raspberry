CFLAGS = -Wall -g -fmax-errors=2
CC= g++ -std=c++0x
OBJS=ps3.o webclient.o kbd.o ../WEBlib.d/WEBlib.o
BIN=ps3

all: ps3
version: 
	$(CC) $(CFLAGS) -c version.cpp -o version.o	
kbd.o: kbd.cpp kbd.h 
	$(CC) $(CFLAGS) -c kbd.cpp -o kbd.o		
webclient.o: webclient.cpp webclient.h 
	$(CC) $(CFLAGS) -c webclient.cpp -o webclient.o	
ps3.o: ps3.cpp ps3.h webclient.h kbd.h joystick.h effects.h
	$(CC) $(CFLAGS) -c ps3.cpp -o ps3.o
ps3: $(OBJS) version
	$(CC) -o $(BIN) $(OBJS) version.o -lbluetooth
	mv ps3 ~/bin
clean:
	rm -f *.o
	@rm -f $(BIN)
	@rm -f ~/bin/$(BIN)
