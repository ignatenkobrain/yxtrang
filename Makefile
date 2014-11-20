all:

	make -C src
	make -C samples

debug:

	make "DBG=-g -O0" -C src
	make "DBG=-g -O0" -C samples

profile:

	make "DBG=-pg" -C src
	make "DBG=-pg" -C samples

clean:

	make -C src clean
	make -C samples clean
