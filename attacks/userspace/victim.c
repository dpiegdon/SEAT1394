
#include <stdio.h>
#include <unistd.h>

void sub1()
{
	printf("--  in sub 1\n");

	sleep(1);
}

void sub2()
{
	printf("in sub 2   --\n");

	sleep(1);
}




int main()
{
	while(1) {
		sub1();
		sub2();
	}
}
