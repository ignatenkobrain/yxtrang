all:

	make -C src
	make -C examples

debug:

	make "DBG=-g -O0" -C src
	make "DBG=-g -O0" -C examples

profile:

	make "DBG=-pg" -C src
	make "DBG=-pg" -C examples

clean:

	rm -rf db
	make -C src clean
	make -C examples clean
