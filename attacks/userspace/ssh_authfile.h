/*  $Id$
 *  sshkey-structs
 *  stolen from key.h from openssh
 */

#ifndef __SSHKEY_STRUCTS_H__
# define __SSHKEY_STRUCTS_H__

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


int key_save_private(Key *key, const char *filename, const char *passphrase, const char *comment);


#endif // __SSHKEY_STRUCTS_H__

