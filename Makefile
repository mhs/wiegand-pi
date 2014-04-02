CC=cc

all: wiegand.out

wiegand.out: wiegand.c
	$(CC) -o wiegand.out wiegand.c -L/usr/local/lib -lwiringPi -lpthread

install: wiegand.out
	cp wiegand.out /usr/local/bin/wiegand

uninstall:
	rm /usr/local/bin/wiegand

clean:
	rm -rf wiegand.out
