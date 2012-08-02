# -*- Makefile -*-
# VDE2 SPAN port
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

include vde2_config.mak

all: span_port.so
	    
span_port.so: span_port.c
	$(CC) -I$(VDE2_INCLUDE) -shared -o $@ $<

clean:
	rm -f span_port.so

