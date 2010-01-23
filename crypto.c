
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

int
crypto_verify_signature(struct string *databuffer, struct string *signature, const char *pubkey)
{
	char *tmpname;
	int err;
	EVP_MD_CTX md_ctx;
	EVP_PKEY *pkey;
	int pubkeyfd;
	FILE *pubkeyfp;

	/* Just load the crypto library error strings, not SSL */
	ERR_load_crypto_strings();
	

        /* create tempfile and store public key into it */
        tmpname = strdup("/tmp/tmp.XXXXXX");
        pubkeyfd = mkstemp(tmpname);
        free(tmpname);
        if (pubkeyfd == -1) {
            fprintf(stderr, "crypto_verify_signature: error creating tempfile\n");
            return 1;
        }
        if (write(pubkeyfd, pubkey, strlen(pubkey)) != strlen(pubkey)) {
            close(pubkeyfd);
            fprintf(stderr, "crypto_verify_signature: tempfile write error\n");
            return 1;
        }
        pubkeyfp = fdopen(pubkeyfd, "rw");
        if (pubkeyfp == NULL) {
            close(pubkeyfd);
            fprintf(stderr, "crypto_verify_signature: tempfile fdopen() failed\n");
            return 1;
        }
        fseek(pubkeyfp, 0, SEEK_SET);

        /* read and parse public key */
        pkey = PEM_read_PUBKEY(pubkeyfp, NULL, NULL, NULL);
        fclose(pubkeyfp);
        if (pkey == NULL) {
            fprintf(stderr, "crypto_verify_signature: loading and parsing public key failed\n");
            ERR_print_errors_fp(stderr);
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
    