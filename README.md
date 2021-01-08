# hlsget
Download HLS VOD streams

## Build
### Windows
```
git clone https://github.com/tdeveaux/hlsget.git
cd hlsget
git clone https://github.com/tdeveaux/hls.git
cd hls
nmake /f Makefile.vc
cd ..
git clone https://github.com/curl/curl.git
cd curl
git checkout tags/curl-7_74_0
buildconf.bat
cd winbuild
nmake /f Makefile.vc mode=dll MACHINE=x64
cd ..\..
git clone https://github.com/openssl/openssl.git
cd openssl
git checkout tags/OpenSSL_1_1_1i
# Install Strawberry Perl from https://strawberryperl.com/
# Install NASM from https://www.nasm.us/
perl Configure VC-WIN64A
nmake
cd ..
nmake /f Makefile.vc
```

### Ubuntu
```
sudo apt install git make gcc libcurl4-openssl-dev libssl-dev
git clone https://github.com/tdeveaux/hlsget.git
cd hlsget
git clone https://github.com/tdeveaux/hls.git
cd hls
make
cd ..
make
```

### macOS
```
git clone https://github.com/tdeveaux/hlsget.git
cd hlsget
git clone https://github.com/tdeveaux/hls.git
cd hls
make -f Makefile.xc
cd ..
git clone https://github.com/openssl/openssl.git
cd openssl
git checkout tags/OpenSSL_1_1_1i
./config
make
cd ..
make -f Makefile.xc
```

## Install
### Ubuntu/macOS
```
make install
```

## Uninstall
### Ubuntu/macOS
```
make uninstall
```

## Usage
```
hlsget playlist.m3u8 [output.ts]
```
