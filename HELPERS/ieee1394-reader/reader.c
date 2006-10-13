
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>

#define MAX_PORTS	20

int main(int argc, char**argv)
{
	raw1394handle_t h;				// global handle
	struct raw1394_portinfo pinf[MAX_PORTS];	// array of all available ports
	int port = -1;					// our chosen ieee1394 port

	nodeid_t self, target;				// nodeid of self and target
	nodeid_t somenode;

	int dump;					// dump file

	int p, i;

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

