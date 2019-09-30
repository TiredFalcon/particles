CFLAGS=-g -Wall -framework OpenCL

default: particles.o opencl_util.o
	gcc $(CFLAGS) `pkg-config --cflags gtk+-2.0` -o particles particles.o opencl_util.o `pkg-config --libs gtk+-2.0`

particles.o: particles.c
	gcc $(CFLAGS) `pkg-config --cflags gtk+-2.0` -o particles.o -c particles.c `pkg-config --libs gtk+-2.0`

opencl_util.o: opencl_util.c
	gcc $(CFLAGS) -o opencl_util.o -c opencl_util.c

clean:
	rm particles particles.o opencl_util.o
