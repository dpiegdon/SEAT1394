all:
	gcc physical-tester.c -lphysical -L/home/lostrace/lib -I/home/lostrace/include -o physical-tester
	gcc logical-tester.c -lphysical -llogical -L/home/lostrace/lib -I/home/lostrace/include -o logical-tester

clean:
	-rm pagedump pagedump.hex
	-rm physical-tester logical-tester
