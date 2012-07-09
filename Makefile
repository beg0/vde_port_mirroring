all: span_port.so
	    
span_port.so: span_port.c
	$(CC) -I/home/beg0/vde_svn/include -shared -o $@ $<
