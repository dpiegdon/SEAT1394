
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>

#include <endian.h>
#include "../include/endian_swap.h"

#define MAX_PORTS	100
#define NODE_OFFSET	0xffc0
#define BUFFERSIZE	8192
#define CONFIGROMSIZE	1024

static char buffer[BUFFERSIZE];

int configrom_read_to_file(int port, char* filename)
{
	raw1394handle_t h;				// handle to access raw1394
	size_t romsize;
	unsigned char version;
	int fd;
	int r;

	// open handle
	h = raw1394_new_handle_on_port(port);
	if(!h) {
		printf("failed to open raw1394 or port %d\n", port);
		return -1;
	}

	// blank buffer
	memset(buffer, 0, CONFIGROMSIZE);
	// get config rom
	if( (r = raw1394_get_config_rom(h, (quadlet_t*) buffer, CONFIGROMSIZE, &romsize, &version)) ) {
		printf("failed to read config rom (buffer too small?) (%d)\n", r);
		return -1;
	} else {
		printf("read config rom: size: %d, version: %d\n", romsize, version);

		// open dumpfile and dump the config rom
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
		if(fd < 0) {
			printf("failed to open dump \"%s\"\n", filename);
			return -1;
		}
		if(romsize != write(fd, buffer, CONFIGROMSIZE)) {
			close(fd);
			printf("failed to write to dump \"%s\"\n", filename);
			return -1;
		}
		close(fd);
		printf("dumped to \"%s\"\n", filename);
	}

	return 0;
}

int configrom_write_from_file(int port, char* filename)
{
	raw1394handle_t h;				// handle to access raw1394
	size_t romsize;
	unsigned char version;
	int fd;
	int r;

	h = raw1394_new_handle_on_port(port);
	if(!h) {
		printf("failed to open raw1394 or port %d\n", port);
		return -1;
	}

	// read old config rom. needed to get version
	if( raw1394_get_config_rom(h, (quadlet_t*) buffer, CONFIGROMSIZE, &romsize, &version) ) {
		printf("failed to read old config rom (buffer to small?)\n");
		return -1;
	} else {
		printf("read old config rom: version: %d\n", version);

		// blank buffer
		memset(buffer, 0, CONFIGROMSIZE);

		// read new configrom from dump
		fd = open(filename, O_RDONLY);
		if(fd < 0) {
			printf("failed to open dump \"%s\"\n", filename);
			return -1;
		}
		romsize = read(fd, buffer, CONFIGROMSIZE);
		if(romsize <= 0) {
			close(fd);
			printf("failed to read new config rom from \"%s\"\n", filename);
			return -1;
		}
		close(fd);
		printf("read new config rom from \"%s\", size %d\n", filename, romsize);

		// set new config rom
		if((r = raw1394_update_config_rom(h, (quadlet_t*) buffer, CONFIGROMSIZE, version))) {
			printf("failed to set new config rom. reason: %s\n",
					(r == -1) ? "version is incorrect" :
						((r == -2) ? "new rom version is too big" : "unknown"));
			return -1;
		}
	}

	return 0;

}

int configrom_dump(int port, nodeid_t target, char* filename)
{
	raw1394handle_t h;				// handle to access raw1394
	uint64_t config_adr;
	char* buffer_adr;
	size_t romsize;
	unsigned char version;
	int r;
	int p;
	int fd;

	h = raw1394_new_handle_on_port(port);
	if(!h) {
		printf("failed to open raw1394 or port %d\n", port);
		return -1;
	}
	
#define CHUNKSIZE 4

	// read CSR_CONFIG_ROM ... CSR_CONFIG_ROM_END from target
	
printf("dumping %d chunks a %d bytes\n", ((CSR_CONFIG_ROM_END - CSR_CONFIG_ROM) / CHUNKSIZE), CHUNKSIZE);
	for(r = 0; r < ((CSR_CONFIG_ROM_END - CSR_CONFIG_ROM) / CHUNKSIZE); r++) {
		config_adr = CSR_REGISTER_BASE + CSR_CONFIG_ROM + CHUNKSIZE * r;
		buffer_adr = buffer + CHUNKSIZE * r;
//printf("dumping adr 0x%llx, size %x to %p\n", config_adr, CHUNKSIZE, buffer_adr);

		p = raw1394_read(h, target + NODE_OFFSET, config_adr, CHUNKSIZE, (quadlet_t*) buffer_adr);
		if(p < 0) {
			printf("failed to read target %d (0x%04x) at CSR offset 0x%04x\n", target, target + NODE_OFFSET, config_adr);
		}
	}
	printf("read config rom of port %d, target %d (0x%04x)\n", port, target, target + NODE_OFFSET);

	// open dumpfile and dump the config rom
	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if(fd < 0) {
		printf("failed to open dump \"%s\"\n", filename);
		return -1;
	}
	if(CONFIGROMSIZE != write(fd, buffer, CONFIGROMSIZE)) {
		close(fd);
		printf("failed to write to dump \"%s\"\n", filename);
		return -1;
	}
	close(fd);
	printf("dumped to \"%s\"\n", filename);
}

int ieee1394scan()
{
	raw1394handle_t h;				// handle to access raw1394
	struct raw1394_portinfo pinf[MAX_PORTS];	// array of all available ports
	nodeid_t self;					// nodeid of self
	nodeid_t somenode;
	int p;
	int i;
	int t;
	uint64_t guid;
	uint32_t high;
	uint32_t low;

	h = raw1394_new_handle();
	if(!h) {
		printf("failed to open raw1394\n");
		return -1;
	}

	p = raw1394_get_port_info(h, pinf, MAX_PORTS);
	printf("got %d ieee1394 ports:\n", p);
	// print info on all ports
	for(i=0; i<p; i++) {
		p = raw1394_set_port(h, i);
		if(p < 0)
			printf("failed to set port %d\n", i);
		self = raw1394_get_local_id(h);
		somenode = raw1394_get_irm_id(h);

		printf("\tport %2d: \"%s\", %d nodes\n",
				i, pinf[i].name, pinf[i].nodes);

		for(t = 0; t < pinf[i].nodes; t++) {
			// read GUID of this node
			raw1394_read(h, NODE_OFFSET + t, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x0c, 4, &low);
			raw1394_read(h, NODE_OFFSET + t, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x10, 4, &high);
			
#ifndef __BIG_ENDIAN__
			guid = (((uint64_t)high) << 32) | low;
			guid = endian_swap64(guid);
#else
			guid = (((uint64_t)low) << 32) | high;
#endif

			printf("\t\tnode %d (0x%04x): GUID: %016llX%s%s\n", t, t+NODE_OFFSET, guid,
					(self - NODE_OFFSET) == t ? " (self)" : "",
					(somenode - NODE_OFFSET) == t ? " (CSR)" : "");
		}
	}

	return 0;
}




void usage(char* progname)
{
	printf("%s < -r | -w | -d | -s > [-p <port>] [-t <target>] [-f <filename>]\n"
	       "\t -r : read  own config rom of -p <port> to -f <filename>\n"
	       "\t -w : write own config rom of -p <port> from -f <filename>\n"
	       "\t -d : dump config rom of -p <port> -t <target> to -f <filename>\n"
	       "\t -s : scan all ports\n",
	       progname);
}

enum command {
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
	enum command cmd = COMMAND_NONE;
	int port = -1;
	nodeid_t target = 0;
	char* filename = NULL;

	// parse options
	while( -1 != (c = getopt(argc, argv, "rwdsp:t:f:")) ) {
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
			case 't':
				target = atoi(optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			default:
				usage(argv[0]);
				return -1;
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
			if( (port == -1) || (target) || (!filename) ) {
				printf("error: -r does not use -t and requires -p and -f\n");
				return -1;
			}
			return configrom_read_to_file(port, filename);
			break;
		case COMMAND_WRITE:
			if( (port == -1) || (target) || (!filename) ) {
				printf("error: -w does not use -t and requires -p and -f\n");
				return -1;
			}
			return configrom_write_from_file(port, filename);
			break;
		case COMMAND_DUMP:
			if( (port == -1) || (!target) || (!filename)) {
				printf("error: -d requires -p and -t and -f\n");
				return -1;
			}
			return configrom_dump(port, target, filename);
			break;
		case COMMAND_SCAN:
			if( (port != -1) || (target) || (filename)) {
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

