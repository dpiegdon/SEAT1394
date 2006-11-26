/*  $Id$
 *  vim: fdm=marker
 *
 *  dump-sshkey : dump parameters of a ssh private key in hex
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
#include <openssl/evp.h>
#include <openssl/bn.h>

#include "ssh-authfile.h"
#include "sshkey-sanitychecks.h"
#include "term.h"

void dump_rsa(RSA* rsa)
{
	char *bn;

	printf("dumping RSA key:\n");

	printf("\tsanity checks on key:\n");
	test_rsa(rsa);

	printf("\tdumping RSA key members:\n");

	// n e d p q dmp1 dmq1 iqmp

	bn = BN_bn2hex(rsa->n);
	printf("\t\t"TERM_BLUE"n:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(rsa->e);
	printf("\t\t"TERM_BLUE"e:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(rsa->d);
	printf("\t\t"TERM_BLUE"d:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(rsa->p);
	printf("\t\t"TERM_BLUE"p:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(rsa->q);
	printf("\t\t"TERM_BLUE"q:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(rsa->dmp1);
	printf("\t\t"TERM_BLUE"dmp1:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(rsa->dmq1);
	printf("\t\t"TERM_BLUE"dmq1:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(rsa->iqmp);
	printf("\t\t"TERM_BLUE"iqmp:"TERM_RESET" 0x%s\n", bn);
	free(bn);
}

void dump_dsa(DSA* dsa)
{
	char *bn;

	printf("dumping DSA key:\n");

	printf("\tsanity checks on key:\n");
	test_dsa(dsa);

	printf("\tdumping DSA key members:\n");

	// p q g pub_key priv_key
	
	bn = BN_bn2hex(dsa->p);
	printf("\t\t"TERM_BLUE"p:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(dsa->q);
	printf("\t\t"TERM_BLUE"q:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(dsa->g);
	printf("\t\t"TERM_BLUE"g:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(dsa->pub_key);
	printf("\t\t"TERM_BLUE"pub_key:"TERM_RESET" 0x%s\n", bn);
	free(bn);

	bn = BN_bn2hex(dsa->priv_key);
	printf("\t\t"TERM_BLUE"priv_key:"TERM_RESET" 0x%s\n", bn);
	free(bn);
}

int main(int argc, char**argv)
{
	Key* key;

	if(argc != 2) {
		printf( "please give filename of private key to dump as parameter.\n"
			"please note, that the key must not be encrypted.\n");
		return -1;
	}

	key = key_load_private(argv[1], NULL, NULL);

	if(key->rsa)
		dump_rsa(key->rsa);

	if(key->dsa)
		dump_dsa(key->dsa);

	return 1;
}
