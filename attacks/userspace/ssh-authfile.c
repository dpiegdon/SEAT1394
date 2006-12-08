/*  $Id$
 *  vim: fdm=marker
 *
 *  authfile : structures and functions to handle keys
 *             private keys and load/save them from/to a file
 *
 *             structs have been taken from openssh,
 *             the load/save functions are stripped versions from
 *             openssh.
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

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

#include "ssh-authfile.h"

// load a private key from a file
Key * privatekey_from_file(char* filename)
{{{
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
}}}

// save a private key to a file
int privatekey_to_file(Key* key, char* filename)
{{{
	FILE *f;
	int suc = -1;

	// open file
	f = fopen(filename, "w");
	if(f == NULL) {
		printf("failed to open file \"%s\".\n", filename);
		return -1;
	}

	// dump a rsa key
	if(key->rsa)
		suc = PEM_write_RSAPrivateKey(f, key->rsa, NULL, NULL, 0, NULL, NULL);
	// dump a dsa key
	if(key->dsa)
		suc = PEM_write_DSAPrivateKey(f, key->dsa, NULL, NULL, 0, NULL, NULL);

printf("dumping done\n"); fflush(stdout);
	fclose(f);
	return suc;
}}}

