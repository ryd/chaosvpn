
#include <stdio.h>
#include <unistd.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include "string/string.h"
#include "fs.h"

/*

This checks data in a struct string against a signature in a second
struct string against a RSA public key.

We use a 4096 bit RSA key to sign the data using a SHA512 hash, which
is hopefully enough for the next few years.


# key created using:
openssl genrsa -aes256 -out privkey.pem 4096

# public key exported:
openssl rsa -in privkey.pem -pubout -out pubkey.pem

# signing command used on the backend:
openssl dgst \
    -sha512 \
    -sign privkey.pem \
    -out tinc-chaosvpn.txt.sig \
    tinc-chaosvpn.txt

# how to verify the signature using a commandline tool:
# (essentially the same as the c code below does)
openssl dgst \
    -sha512 \
    -verify pubkey.pem \
    -signature tinc-chaosvpn.txt.sig \
    tinc-chaosvpn.txt

*/

EVP_PKEY *
crypto_load_key(const char *key, bool is_private) {
        EVP_PKEY *pkey;
	char *tmpname;
	int keyfd;
	FILE *keyfp;
    
        /* create tempfile and store key into it */
        tmpname = strdup("/tmp/tmp.XXXXXX");
        keyfd = mkstemp(tmpname);
        free(tmpname);
        if (keyfd == -1) {
            fprintf(stderr, "crypto_load_key: error creating tempfile\n");
            return NULL;
        }
        if (write(keyfd, key, strlen(key)) != strlen(key)) {
            close(keyfd);
            fprintf(stderr, "crypto_load_key: tempfile write error\n");
            return NULL;
        }
        keyfp = fdopen(keyfd, "rw");
        if (keyfp == NULL) {
            close(keyfd);
            fprintf(stderr, "crypto_load_key: tempfile fdopen() failed\n");
            return NULL;
        }
        fseek(keyfp, 0, SEEK_SET);

        /* read and parse key */
        if (is_private) {
            pkey = PEM_read_PrivateKey(keyfp, NULL, NULL, NULL);
        } else {
            pkey = PEM_read_PUBKEY(keyfp, NULL, NULL, NULL);
        }
        fclose(keyfp);
        if (pkey == NULL) {
            fprintf(stderr, "crypto_load_key: loading and parsing key failed\n");
            ERR_print_errors_fp(stderr);
            return NULL;
        }
        
        return pkey;
}


int
crypto_verify_signature(struct string *databuffer, struct string *signature, const char *pubkey)
{
	int err;
	EVP_MD_CTX md_ctx;
	EVP_PKEY *pkey;

	/* Just load the crypto library error strings, not SSL */
	ERR_load_crypto_strings();
	
	/* load public key into openssl structure */
        pkey = crypto_load_key(pubkey, false);
        if (pkey == NULL) {
            fprintf(stderr, "crypto_verify_signature: key loading failed\n");
            return 1;
        }
        
        /* Verify the signature */
        if (EVP_VerifyInit(&md_ctx, EVP_sha512()) != 1) {
            fprintf(stderr, "crypto_verify_signature: libcrypto verify init failed\n");
            EVP_PKEY_free(pkey);
            return 1;
        }
        EVP_VerifyUpdate(&md_ctx, string_get(databuffer), string_length(databuffer));
        err = EVP_VerifyFinal(&md_ctx, (unsigned char*)string_get(signature), string_length(signature), pkey);
        EVP_PKEY_free(pkey);
        
        if (err != 1) {
            fprintf(stderr, "crypto_verify_signature: signature verify failed, received bogus data from backend.\n");
            ERR_print_errors_fp(stderr);
            err = 1;
            goto bailout_ctx_cleanup;
        }

        err = 0;

bailout_ctx_cleanup:
        EVP_MD_CTX_cleanup(&md_ctx);

        //printf ("Signature Verified Ok.\n");
	return err;
}
    