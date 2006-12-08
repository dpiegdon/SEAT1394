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
#include <string.h>

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

#include "ssh-authfile.h"
#include "sshkey-sanitychecks.h"
#include "term.h"

Key * privatekey_from_file(char* filename)
{
	Key *key = NULL;
	FILE *f;
	EVP_PKEY *pk = NULL;

	f = fopen(filename, "r");
	if(f == NULL) {
		printf("failed to open file \"%s\".\n", filename);
		return NULL;
	}

	pk = PEM_read_PrivateKey(f, NULL, NULL, NULL);
	if(pk == NULL) {
		printf("failed to read or parse key!\n");
		goto pff_close;
	}

	key = malloc(sizeof(Key));
	key->type = KEY_UNSPEC;
	key->flags = 0;
	key->rsa = NULL;
	key->dsa = NULL;

	switch(pk->type) {
		case EVP_PKEY_RSA:
			key->rsa = EVP_PKEY_get1_RSA(pk);
			if(1 != RSA_blinding_on(key->rsa, NULL))
				printf("rsa blinding failed\n");
			key->type = KEY_RSA;
			break;

		case EVP_PKEY_DSA:
			key->dsa = EVP_PKEY_get1_DSA(pk);
			key->type = KEY_DSA;
			break;

		default:
			printf("this key is neither RSA nor DSA!\n");
			free(key);
			key = NULL;
			break;
	}

	EVP_PKEY_free(pk);
pff_close:
	fclose(f);
	return key;
}

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
		printf( "please give filename of private key to dump.\n"
			"please note, that the key must not be encrypted.\n");
		return -1;
	}

	// load private key
	key = privatekey_from_file(argv[1]);

	if(!key) {
		printf("failed to load ``%s''!\n(wrong password or permissions?)\n", argv[1]);
		return -1;
	}

	printf("key ``%s'' loaded successfully\n", argv[1]);

	if(key->rsa)
		dump_rsa(key->rsa);

	if(key->dsa)
		dump_dsa(key->dsa);

	return 0;
}

