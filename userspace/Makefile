# 
CC=gcc

DEBUG=YES
USE_GM=YES


OPTIONS_COMPILE_DEBUG=-D_DEBUG -DDEBUG -DUNIX -DUNIX_LINUX -DCPU_64 -D_THREADSAFE -D_FILE_OFFSET_BITS=64 -DHAVE_CONFIG_H
ifeq ($(USE_GM),YES)
	OPTIONS_COMPILE_DEBUG += -DUSE_GMSSL
endif
OPTIONS_COMPILE_DEBUG += -g -fsigned-char -m64 
OPTIONS_COMPILE_DEBUG += -I/root/libnl-3.5.0/include

OPTIONS_LINK_DEBUG=-g -fsigned-char -m64 -lm -ldl -lrt -pthread  -lreadline -lncurses -lz 

LIBNL = /root/libnl-3.5.0/lib/.libs/libnl-3.a  /root/libnl-3.5.0/lib/.libs/libnl-genl-3.a


ifeq ($(DEBUG),YES)
	OPTIONS_COMPILE=$(OPTIONS_COMPILE_DEBUG)
	OPTIONS_LINK=$(OPTIONS_LINK_DEBUG)
else
	OPTIONS_COMPILE=$(OPTIONS_COMPILE_RELEASE)
	OPTIONS_LINK=$(OPTIONS_LINK_RELEASE)
endif

# Files
HEADERS=../common.h

OBJECTS=objs/userspace.o

TARGET=userApp
# Build Action
default:	build

build:	$(OBJECTS) $(TARGET)



objs/userspace.o: userspace.c $(OBJECTS)
	@mkdir -p objs/
	$(CC) $(OPTIONS_COMPILE) -c userspace.c -o objs/userspace.o


$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(OPTIONS_LINK) $(LIBNL) -o $(TARGET)


# Clean
clean:
	rm -f $(OBJECTS) $(TARGET)



