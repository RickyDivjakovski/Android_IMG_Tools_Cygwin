## Android IMG tools to be compiled with cygwin.

Install cygwin in the default directory with the following packages.
libpcre1, zlib-devel, cmake, make, automake, gcc-core, gcc-g++, libgcc1, libstdc++6, cpio, flex, bison, cygwin32-gcc-core, patch, libintl


# To build simply open CMD and
libpcre1, zlib-devel, cmake, make, automake, gcc-core, gcc-g++, libgcc1, libstdc++6, cpio, flex, bison, cygwin32-gcc-core


# To build simply open CMD and
path=%path%C:\cygwin64\bin;
cd to each directory
make

# To build e2fsprogs
Open cygwin
cd to the directory with the patch files
patch -p1 < 1.44.5-as-needed.patch
patch -p1 < 1.44.5-uuidd.patch
cd e2fsprogs-1.44.5
mkdir build
cd build
../configure
make


# Original Sources
* Sparse utillities based on sources https://github.com/anestisb/android-simg2img
* mkbootimg based on sources https://github.com/osm0sis/mkbootimg
* mkbootfs based on sources https://github.com/osm0sis/mkbootfs
* make_ext4fs based on sources https://github.com/jamflux/make_ext4fs
* sefcontext_decompile based on sources https://github.com/wuxianlin/sefcontext_decompile
* dtc based on sources https://github.com/dgibson/dtc
* e2fsprogs based on sources from cygwin


# Generates - 
* append2simg.exe
* cpio.exe
* dtc.exe
* gzip.exe
* img2simg.exe
* make_ext4fs.exe
* mkbootfs.exe
* mkbootimg.exe
* sefcontext_decompile.exe
* simg2img.exe
* simg2simg.exe
* unpackbootimg.exe
* mke2fs.exe

## Contributors
SuperR, anestisb, rkhat2, osm0sis, jamflux, wuxianlin, Xannytech, CarmineRovito, github, tytso, andreas-schwab, Cygwin

