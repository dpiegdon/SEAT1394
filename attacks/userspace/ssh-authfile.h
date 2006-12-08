/*  $Id$
 *  sshkey-structs
 *  stolen from key.h
 */

#ifndef __SSHKEY_STRUCTS_H__
# define __SSHKEY_STRUCTS_H__

#include <openssl/rsa.h>
#include <openssl/dsa.h>

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

#endif // __SSHKEY_STRUCTS_H__

