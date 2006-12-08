/*  $Id$
 *  sshkey-structs
 *  stolen from key.h and authfile.h from openssh
 */

#ifndef __SSHKEY_STRUCTS_H__
# define __SSHKEY_STRUCTS_H__

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
//#include <openssl/evp.h>

// from key.h:

enum types {
        KEY_RSA1,
        KEY_RSA,
        KEY_DSA,
        KEY_UNSPEC
};
typedef struct Key {
        int      type;
        int      flags;
        RSA     *rsa;
        DSA     *dsa;
} Key;

// from authfile.h:

//int key_save_private(Key *key, const char *filename, const char *passphrase, const char *comment);
Key * key_load_private(const char *filename, const char *passphrase, char **commentp);

#endif // __SSHKEY_STRUCTS_H__

