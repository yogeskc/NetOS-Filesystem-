# paths to build
objpath = *.c

# build command
cc = gcc -g

# output filename
outname = netfs

# default build command
all: $(objpath)
	$(cc) -lpthread -lm $(objpath) -o $(outname)
