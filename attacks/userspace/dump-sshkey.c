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

void dump_rsa(Key* key)
{
	printf("dumping RSA key:\n");

	printf("\tsanity checks on key:\n");

}

void dump_dsa(Key* key)
{
	printf("dumping DSA key:\n");

	printf("\tsanity checks on key:\n");
	{{{
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
		printf("\t\t(info) p is "TERM_CYAN"%d bits"TERM_RESET" long.\n", len);
		if(len%64)
			printf("\t\t"TERM_RED"(WARN)"TERM_RESET" p is not a multiple of 64 bits long! (len%%64 = %d)\n", len%64);
		else
			printf("\t\t"TERM_GREEN"(ok)"TERM_RESET"   p is a multiple of 64 bits long (64*%d)\n", len/64);
		// p: test for prime:
		for(i = 0; i < BN_prime_checks_for_size(len); i++) {
			ret = BN_is_prime_fasttest_ex(key->dsa->p, BN_prime_checks, ctx, 1, NULL);
			if(ret == 0) {
				printf("\t\t"TERM_RED"(WARN)"TERM_RESET" p is not a prime!\n");
				break;
			}
			if(ret == -1) {
				printf("\t\t"TERM_FAULT"(ERR)"TERM_RESET"  some strange error during test, if p is a prime\n");
				break;
			}
		}
		if(ret == 1)
			printf("\t\t"TERM_GREEN"(ok)"TERM_RESET"   p seems to be a prime (after %d tests)\n", i);

		// q: size
		len = BN_num_bits(key->dsa->q);
		if(len != 160)
			printf("\t\t"TERM_RED"(WARN)"TERM_RESET" q is not 160 bits but %d bits long!\n", len);
		else
			printf("\t\t"TERM_GREEN"(ok)"TERM_RESET"   q is 160 bits long.\n");
		// q: prime-factor of p-1? then q is the greatest common divisor of q and p-1.
		BN_one(n);				// n := 1;
		BN_sub(o,key->dsa->p,n);		// o := p-1;
		BN_gcd(n,o,key->dsa->q,ctx);		// n := GCD(p-1,q);
		BN_sub(o,n,key->dsa->q);		// o := GCD(p-1,q) - q; -- this should be 0 if q is a prime-factor or p-1
		if(BN_is_zero(o))
			printf("\t\t"TERM_GREEN"(ok)"TERM_RESET"   q is a prime-factor of p-1.\n");
		else
			printf("\t\t"TERM_RED"(WARN)"TERM_RESET"   q is NO prime-factor of p-1!\n");

		// pub_key:
		BN_mod_exp(o, key->dsa->g, key->dsa->priv_key, key->dsa->p, ctx);
		BN_sub(n, o, key->dsa->pub_key);
		if(BN_is_zero(n))
			printf("\t\t"TERM_GREEN"(ok)"TERM_RESET"   pubkey matches to parameters\n");
		else
			printf("\t\t"TERM_RED"(WARN)"TERM_RESET" pubkey does not match to parameters\n");

		// this should suffice as a test
		// show length of priv_key as info
		printf("\t\t(info) priv_key is %d bits long (should be <= len(q))\n", BN_num_bits(key->dsa->priv_key));

		printf("\t\t" TERM_GREEN "OK." TERM_RESET "\n");

		BN_CTX_free(ctx);

		BN_free(n);
		BN_free(o);
	}}}
}

int main(int argc, char*argv)
{
	Key* key;


	if(argc != 1) {
		printf( "please give filename of private key to dump as parameter.\n"
			"please note, that the key must not be encrypted.\n");
		return -1;
	}

	key = key_load_private(argv[1], NULL, NULL);

	if(key->rsa)
		dump_rsa(key);

	if(key->dsa)
		dump_dsa(key);
}
