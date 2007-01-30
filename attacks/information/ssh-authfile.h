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
 *  Copyright (C) 2006,2007
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

#ifndef __SSH_AUTHFILE_H__
# define __SSH_AUTHFILE_H__

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

// load a private key from a file
Key * privatekey_from_file(char* filename);
// save a private key to a file
int privatekey_to_file(Key* key, char* filename);

#endif // __SSH_AUTHFILE_H__

