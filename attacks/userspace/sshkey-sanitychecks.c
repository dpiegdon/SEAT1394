/*  $Id$
 *  vim: fdm=marker
 *
 *  sshkey-sanitychecks : check RSA or DSA keys
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

#include <stdio.h>

#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/bn.h>

#include "term.h"

// sanity-check a RSA-key
void test_rsa(RSA* rsa)
{{{
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

	int len;
	int i;
	int ret;
	BN_CTX* ctx;

	ctx = BN_CTX_new();

	// is p a prime?
	len = BN_num_bits(rsa->p);
	for(i = 0; i < BN_prime_checks_for_size(len); i++) {
		ret = BN_is_prime_fasttest_ex(rsa->p, BN_prime_checks, ctx, 1, NULL);
		if(ret == 0) {
			printf("\t\t\t"TERM_RED"(WARN)"TERM_RESET" p is not a prime!\n");
			break;
		}
		if(ret == -1) {
			printf("\t\t\t"TERM_FAULT"(ERR)"TERM_RESET"  some strange error during test, if p is a prime\n");
			break;
		}
	}
	if(ret == 1)
		printf("\t\t\t"TERM_GREEN"(ok)"TERM_RESET"   p (%d bits) seems to be a prime (after %d tests)\n", len, i);

	// is q a prime?
	len = BN_num_bits(rsa->q);
	for(i = 0; i < BN_prime_checks_for_size(len); i++) {
		ret = BN_is_prime_fasttest_ex(rsa->q, BN_prime_checks, ctx, 1, NULL);
		if(ret == 0) {
			printf("\t\t\t"TERM_RED"(WARN)"TERM_RESET" q is not a prime!\n");
			break;
		}
		if(ret == -1) {
			printf("\t\t\t"TERM_FAULT"(ERR)"TERM_RESET"  some strange error during test, if q is a prime\n");
			break;
		}
	}
	if(ret == 1)
		printf("\t\t\t"TERM_GREEN"(ok)"TERM_RESET"   q (%d bits) seems to be a prime (after %d tests)\n", len, i);

	// n:
	printf("\t\t\t(info) n is "TERM_CYAN"%d bits"TERM_RESET" long.\n",BN_num_bits(rsa->n));


	printf("\t\t\t" TERM_GREEN "OK." TERM_RESET "\n");

	BN_CTX_free(ctx);
}}}

// sanity-check a DSA-key
void test_dsa(DSA* dsa)
{{{
	// p,q and g are public DSA parameters; 
	// 	p = a prime number L bits long (DSA standard say L \in [512,1024]
	// 	q = a 160-bit prime factor of p-1
	//	g = h^(p-1)/q mod p , where h is any number less than p-1 such that g>1
	//	priv_key = a number less than q
	//	pub_key = g^priv_key mod p

	int len;
	int i;
	int ret;
	BN_CTX* ctx;
	BIGNUM *o,*n;

	ctx = BN_CTX_new();
	o = BN_new();
	n = BN_new();

	// p: size
	len = BN_num_bits(dsa->p);
	printf("\t\t\t(info) p is "TERM_CYAN"%d bits"TERM_RESET" long.\n", len);
	if(len%64)
		printf("\t\t\t"TERM_RED"(WARN)"TERM_RESET" p is not a multiple of 64 bits long! (len%%64 = %d)\n", len%64);
	else
		printf("\t\t\t"TERM_GREEN"(ok)"TERM_RESET"   p is a multiple of 64 bits long (64*%d)\n", len/64);
	// p: test for prime:
	for(i = 0; i < BN_prime_checks_for_size(len); i++) {
		ret = BN_is_prime_fasttest_ex(dsa->p, BN_prime_checks, ctx, 1, NULL);
		if(ret == 0) {
			printf("\t\t\t"TERM_RED"(WARN)"TERM_RESET" p is not a prime!\n");
			break;
		}
		if(ret == -1) {
			printf("\t\t\t"TERM_FAULT"(ERR)"TERM_RESET"  some strange error during test, if p is a prime\n");
			break;
		}
	}
	if(ret == 1)
		printf("\t\t\t"TERM_GREEN"(ok)"TERM_RESET"   p seems to be a prime (after %d tests)\n", i);

	// q: size
	len = BN_num_bits(dsa->q);
	if(len != 160)
		printf("\t\t\t"TERM_RED"(WARN)"TERM_RESET" q is not 160 bits but %d bits long!\n", len);
	else
		printf("\t\t\t"TERM_GREEN"(ok)"TERM_RESET"   q is 160 bits long.\n");
	// q: prime-factor of p-1? then q is the greatest common divisor of q and p-1.
	BN_one(n);				// n := 1;
	BN_sub(o,dsa->p,n);		// o := p-1;
	BN_gcd(n,o,dsa->q,ctx);		// n := GCD(p-1,q);
	BN_sub(o,n,dsa->q);		// o := GCD(p-1,q) - q; -- this should be 0 if q is a prime-factor or p-1
	if(BN_is_zero(o))
		printf("\t\t\t"TERM_GREEN"(ok)"TERM_RESET"   q is a prime-factor of p-1.\n");
	else
		printf("\t\t\t"TERM_RED"(WARN)"TERM_RESET"   q is NO prime-factor of p-1!\n");

	// pub_key:
	BN_mod_exp(o, dsa->g, dsa->priv_key, dsa->p, ctx);
	BN_sub(n, o, dsa->pub_key);
	if(BN_is_zero(n))
		printf("\t\t\t"TERM_GREEN"(ok)"TERM_RESET"   pubkey matches to parameters\n");
	else
		printf("\t\t\t"TERM_RED"(WARN)"TERM_RESET" pubkey does not match to parameters\n");

	// this should suffice as a test
	// show length of priv_key as info
	printf("\t\t\t(info) priv_key is %d bits long (should be <= len(q))\n", BN_num_bits(dsa->priv_key));

	printf("\t\t\t" TERM_GREEN "OK." TERM_RESET "\n");

	BN_CTX_free(ctx);

	BN_free(n);
	BN_free(o);
}}}

