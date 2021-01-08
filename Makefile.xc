hlsget: main.o
	cc -o hlsget hls/hls.o main.o -lcurl -Lopenssl -lcrypto

main.o: main.c
	cc -c -Iopenssl/include -Wall main.c

.PHONY: clean
clean:
	rm -f hlsget main.o

.PHONY: install
install: hlsget
	sudo mkdir -p /usr/local/{bin,lib}
	sudo install hlsget /usr/local/bin
	sudo install openssl/libcrypto.1.1.dylib /usr/local/lib

.PHONY: uninstall
uninstall:
	-killall hlsget
	sudo rm -f /usr/local/bin/hlsget /usr/local/lib/libcrypto.1.1.dylib
	-sudo rmdir /usr/local/{bin,lib}
