# Android IMG tools to be compiled with cygwin.

Install cygwin in the default directory with the following packages.

libpcre1, zlib-devel, cmake, make, automake, gcc-core, gcc-g++, libgcc1, libstdc++6, cpio, flex, bison, cygwin32-gcc-core

## To build simply open CMD and
path=%path%C:\cygwin64\bin;

cd to each directory

make

## Original Sources
Sparse utillities based on sources https://github.com/anestisb/android-simg2img

mkbootimg based on sources https://github.com/osm0sis/mkbootimg

mkbootfs based on sources https://github.com/osm0sis/mkbootfs

make_ext4fs based on sources https://github.com/jamflux/make_ext4fs

sefcontext_decompile based on sources https://github.com/wuxianlin/sefcontext_decompile

## Generates - 
append2simg.exe

cpio.exe

dtc.exe

gzip.exe

img2simg.exe

make_ext4fs.exe

mkbootfs.exe

mkbootimg.exe

sefcontext_decompile.exe

simg2img.exe

simg2simg.exe

unpackbootimg.exe

## Contributors

SuperR, anestisb, rkhat2, osm0sis, jamflux, wuxianlin, Xannytech, CarmineRovito
