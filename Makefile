all:	hidmouse bulkio

hidmouse:
	make -f Makefile.hidmouse -j

bulkio:
	make -f Makefile.bulkio -j

clean:
	make -f Makefile.hidmouse clean

