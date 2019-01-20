
CC = gcc

ifeq ($(windir),)
EXE =
RM = rm -f
else
EXE = .exe
RM = del
endif

CFLAGS = -ffunction-sections -O3
LDFLAGS = -Wl,--gc-sections
OBJECTS = mkbootfs.o

all:mkbootfs$(EXE)

static:mkbootfs-static$(EXE)

mkbootfs$(EXE):$(OBJECTS)
	$(CROSS_COMPILE)$(CC) -o $@ $^ -L. $(LDFLAGS) -s

mkbootfs-static$(EXE):$(OBJECTS)
	$(CROSS_COMPILE)$(CC) -o $@ $^ -L. $(LDFLAGS) -static -s

.c.o:
	$(CROSS_COMPILE)$(CC) -o $@ $(CFLAGS) -c $< -I. -Werror

clean:
	$(RM) mkbootfs mkbootfs-static mkbootfs.exe mkbootfs-static.exe $(OBJECTS) Makefile.~

