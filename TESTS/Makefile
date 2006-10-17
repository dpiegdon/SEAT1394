all:
	gcc physical-tester.c -lphysical -L/home/lostrace/lib -I/home/lostrace/include -o physical-tester
	gcc linear-tester.c -lphysical -llinear -L/home/lostrace/lib -I/home/lostrace/include -o linear-tester

clean:
	-rm pagedump pagedump.hex
	-rm physical-tester linear-tester
