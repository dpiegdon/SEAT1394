
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <errno.h>
#include <stdio.h>

int main(int argc, char**argv)
{
	char *buffer;
	int fd;
	void(*callme)(void);
	int r;

	if(argc != 2) {
		printf("please give file with shellcode as argument!\n");
		exit(-1);
	}

	buffer = calloc(1024,4);
	fd = open(argv[1], O_RDONLY);
	if(fd < 0) {
		printf("failed to open file \"%s\": %s.\n", argv[1],strerror(errno));
		free(buffer);
		exit(-2);
	}

	if(1 > (r = read(fd, buffer, 4096))) {
		printf("failed to read from file \"%s\": %s.\n", argv[1], strerror(errno));
		free(buffer);
		exit(-3);
	} else {
		printf("read %d bytes from \"%s\".\n", r, argv[1]);
	}

	close(fd);

	callme = (void(*)(void)) buffer;

	printf(">>> %s  DO  >>>\n", argv[1]);

	callme();

	printf("<<< %s DONE <<<\n", argv[1]);

	free(buffer);

	return 0;
}

