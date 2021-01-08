hlsget: main.o
	cc -o hlsget hls/hls.o main.o -lcurl -lcrypto

main.o: main.c
	cc -c -Wall main.c

.PHONY: clean
clean:
	rm -f hlsget main.o

.PHONY: install
install: hlsget
	sudo install hlsget /usr/local/bin

.PHONY: uninstall
uninstall:
	-killall hlsget
	sudo rm -f /usr/local/bin/hlsget
