all:	physical-tester linear-tester

physical-tester: physical-tester.c
	gcc physical-tester.c -Wall -W -lphysical -L/home/lostrace/lib -I/home/lostrace/include -o physical-tester

linear-tester: linear-tester.c
	gcc linear-tester.c -Wall -W -lphysical -llinear -L/home/lostrace/lib -I/home/lostrace/include -o linear-tester

clean:
	-rm pagedump pagedump.hex
	-rm physical-tester linear-tester
