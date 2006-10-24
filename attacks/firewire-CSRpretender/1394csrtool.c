
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>

#define MAX_PORTS	20

int csr_read_to_file(int port, char* filename)
{
	raw1394handle_t h;				// handle to access raw1394

	h = raw1394_new_handle();
}

int csr_write_from_file(int port, char* filename)
{
	raw1394handle_t h;				// handle to access raw1394

	h = raw1394_new_handle();
}

int csr_dump(int port, nodeid_t target, char* filename)
{
	raw1394handle_t h;				// handle to access raw1394
	int p;

	h = raw1394_new_handle();
	p = raw1394_set_port(h, port);
	if(p != 0)
		printf("port %d failed", port);
		goto release;
	}

}

int ieee1394scan()
{
	raw1394handle_t h;				// handle to access raw1394
	struct raw1394_portinfo pinf[MAX_PORTS];	// array of all available ports
	nodeid_t self;					// nodeid of self
	int p;

	h = raw1394_new_handle();
	p = raw1394_get_port_info(h, pinf, MAX_PORTS);
	printf("got %d ieee1394 ports:\n", p);
	// print info on all ports
	for(i=0; i<p; i++) {
		printf("\tport %2d: \"%s\", %d nodes\n",
				i, pinf[i].name, pinf[i].nodes);
	}

	return 0;

	// per port:
	printf("\nnodes:\n");
	self = raw1394_get_local_id(h);
	printf("self   is %d\n", (u_int16_t)self);
	somenode = raw1394_get_irm_id(h);
	printf("ISM    is %d\n", (u_int16_t)somenode);
}




void usage(char* progname)
{
	printf("%s < -r | -w | -d | -s > [-p <port> -t <target> -f <filename>]\n"
	       "\t -r : read  own config rom of -p <port> to -f <filename>\n"
	       "\t -w : write own config rom of -p <port> from -f <filename>\n"
	       "\t -d : dump config rom of -p <port> -t <target> to -f <filename>\n"
	       "\t -s : scan all ports\n");
}

union command {
	COMMAND_NONE,
	COMMAND_READ,
	COMMAND_WRITE,
	COMMAND_DUMP,
	COMMAND_SCAN
};

int main(int argc, char**argv)
{
	int c;						// char for getopt

	// arguments:
	union command cmd = COMMAND_NONE;
	int port = -1;
	nodeid_t target = -1;
	char* filename = NULL;

	// parse options
	while( -1 != (c = getopt(argc, argv, "rwdsp:b:f:")) ) {
		switch (c) {
			case 'r':
				if(cmd != COMMAND_NONE) {
					printf("error: multiple commands given\n");
					usage(argv[0]);
					return -1;
				}
				cmd = COMMAND_READ;
				break;
			case 'w':
				if(cmd != COMMAND_NONE) {
					printf("error: multiple commands given\n");
					usage(argv[0]);
					return -1;
				}
				cmd = COMMAND_WRITE;
				break;
			case 'd':
				if(cmd != COMMAND_NONE) {
					printf("error: multiple commands given\n");
					usage(argv[0]);
					return -1;
				}
				cmd = COMMAND_DUMP;
				break;
			case 's':
				if(cmd != COMMAND_NONE) {
					printf("error: multiple commands given\n");
					usage(argv[0]);
					return -1;
				}
				cmd = COMMAND_SCAN;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'b':
				target = atoi(optarg);
				break;
			case 'f':
				filename = optarg;
				break;
		}
	}

	switch (cmd) {
		case COMMAND_NONE:
			printf("error: missing command\n");
			usage(argv[0]);
			return -1;
			break;
		case COMMAND_READ:
			if( (port != -1) || (target != -1) || (filename == NULL) ) {
				printf("error: -r does not use -t and requires -p and -f\n");
				return -1;
			}
			return csr_read_to_file(port, filename);
			break;
		case COMMAND_WRITE:
			if( (port != -1) || (target != -1) || (filename == NULL) ) {
				printf("error: -w does not use -t and requires -p and -f\n");
				return -1;
			}
			return csr_write_from_file(port, filename);
			break;
		case COMMAND_DUMP:
			if( (port == -1) || (target == -1) || (filename == NULL)) {
				printf("error: -w requires -p and -t and -f\n");
				return -1;
			}
			return csr_dump(port, target, filename);
			break;
		case COMMAND_SCAN:
			if( (port != -1) || (target != -1) || (filename != NULL)) {
				printf("error: -s does not require any parameters\n");
				return -1;
			}
			return ieee1394scan();
			break;
	}
}

/*


{

	// get ieee1394 handle
	h = raw1394_new_handle();
	// get info on first MAX_PORTS ports
	p = raw1394_get_port_info(h, pinf, MAX_PORTS);
	printf("got %d ieee1394 ports:\n", p);
	// print info on all ports
	for(i=0; i<p; i++) {
		printf("\tport %2d: \"%s\", %d nodes\n",
				i, pinf[i].name, pinf[i].nodes);
		if( (pinf[i].nodes > 1) && (-1 == port) ) {
			printf("\tchose port %d.\n",i);
			port = i;
		}
	}
	// choose a port
	p = raw1394_set_port(h, port);
	if(p == 0)
		printf("choice ok.\n");
	else {
		printf("choice failed!\n");
		goto release;
	}

	// some debugging: self nodeid
	printf("\nnodes:\n");
	self = raw1394_get_local_id(h);
	printf("self   is %d\n", (u_int16_t)self);
	somenode = raw1394_get_irm_id(h);
	printf("ISM    is %d\n", (u_int16_t)somenode);

	printf("trying target: %s (argument 1)\n", argv[1]);
	target = atoi(argv[1]);
	printf("target is %d\n", (u_int16_t)target);

	somenode = raw1394_get_nodecount(h);
	printf("%d nodes.\n", (u_int16_t)somenode);


	// open dump file
	dump = open(argv[1], O_RDWR | O_CREAT, 0644);

	// read something from target
	printf("\ntrying to read:\n");
	{
#define BLOCKSIZE 1024
		char* iobuf[BLOCKSIZE];
		nodeaddr_t adr = 0;
		size_t length = BLOCKSIZE;
		p = 0;

		memset(iobuf, 0x23, BLOCKSIZE * sizeof(char));

		while(!p) {
			p = raw1394_read(h, target, adr, length, (quadlet_t*)iobuf);
			printf("read 0x%x bytes from @0x%llx. result: %s",
					length, adr, (p == 0) ? "ok\n" : "error: ");
			adr += length;

			if(p) {
				printf("%s\n", strerror(errno));
			} else {
				write(dump, iobuf, length);
			}
		}
	}
	
	// EXIT:
	close(dump);

release:
	// release handle
	raw1394_destroy_handle(h);

	// exit
	return 1;
}

*/

