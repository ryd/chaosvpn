#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "chaosvpn.h"

#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

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
crypto_load_key(const char *key, const bool is_private)
{
	BIO *bio;
	EVP_PKEY *pkey;

	bio = BIO_new_mem_buf((void *)key, strlen(key));
	if (bio == NULL) {
            log_err("crypto_load_key: BIO_new_mem_buf() failed\n");
	    return NULL;
	}

        /* read and parse key */
        if (is_private) {
            pkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
        } else {
            pkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
        }
        BIO_free(bio);
        if (pkey == NULL) {
            log_err("crypto_load_key: loading and parsing key failed\n");
            ERR_print_errors_fp(stderr);
            return NULL;
        }
        
        return pkey;
}


bool
crypto_rsa_verify_signature(struct string *databuffer, struct string *signature, const char *pubkey)
{
	int err;
	bool retval;
	EVP_MD_CTX md_ctx;
	EVP_PKEY *pkey;

	/* load public key into openssl structure */
        pkey = crypto_load_key(pubkey, false);
        if (pkey == NULL) {
            log_err("crypto_verify_signature: key loading failed\n");
            return false;
        }
        
        /* Verify the signature */
        if (EVP_VerifyInit(&md_ctx, EVP_sha512()) != 1) {
            log_err("crypto_verify_signature: libcrypto verify init failed\n");
            EVP_PKEY_free(pkey);
            return false;
        }
        EVP_VerifyUpdate(&md_ctx, string_get(databuffer), string_length(databuffer));
        err = EVP_VerifyFinal(&md_ctx, (unsigned char*)string_get(signature), string_length(signature), pkey);
        EVP_PKEY_free(pkey);
        
        if (err != 1) {
            log_err("crypto_verify_signature: signature verify failed, received bogus data from backend.\n");
            ERR_print_errors_fp(stderr);
            retval = false;
            goto bailout_ctx_cleanup;
        }

        retval = true;

bailout_ctx_cleanup:
        EVP_MD_CTX_cleanup(&md_ctx);

        //log_info("Signature Verified Ok.\n");
	return retval;
}

bool
crypto_rsa_decrypt(struct string *ciphertext, const char *privkey, struct string *decrypted)
{
        bool retval = false;
        int len;
        EVP_PKEY *pkey;

        /* load private key into openssl */
        pkey = crypto_load_key(privkey, true);
        if (pkey == NULL) {
            log_err("crypto_rsa_decrypt: key loading failed.\n");
            return false;
        }

        /* check length of ciphertext */
        if (string_length(ciphertext) != EVP_PKEY_size(pkey)) {
            log_err("crypto_rsa_decrypt: ciphertext should match length of key (%" PRIuPTR " vs %d).\n",
                    string_length(ciphertext),EVP_PKEY_size(pkey));
            goto bail_out;
        }

        string_free(decrypted); /* just to be sure */
        string_init(decrypted, EVP_PKEY_size(pkey), 512);
        if (string_size(decrypted) < EVP_PKEY_size(pkey)) {
            log_err("crypto_rsa_decrypt: malloc error.\n");
        }
        
        len = RSA_private_decrypt(string_length(ciphertext),
            (unsigned char*)string_get(ciphertext),
            (unsigned char*)string_get(decrypted),
            pkey->pkey.rsa,
            RSA_PKCS1_OAEP_PADDING);
        if (len >= 0) {
            /* TODO: need cleaner way: */
            decrypted->_u._s.length = len;
            retval = true;
        } else {
            retval = false;
            log_err("crypto_rsa_decrypt: rsa decrypt failed.\n");
            ERR_print_errors_fp(stderr);
        }

bail_out:
        EVP_PKEY_free(pkey);
        return retval;
}

bool
crypto_aes_decrypt(struct string *ciphertext, struct string *aes_key, struct string *aes_iv, struct string *decrypted)
{
    bool retval = false;
    EVP_CIPHER_CTX ctx;
    int decryptspace;
    int decryptdone;

    EVP_CIPHER_CTX_init(&ctx);
    if (!EVP_DecryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL,
        (unsigned char *)string_get(aes_key),
        (unsigned char *)string_get(aes_iv))) {
        log_err("crypto_aes_decrypt: init failed\n");
        ERR_print_errors_fp(stderr);
        goto bail_out;
    }
    EVP_CIPHER_CTX_set_padding(&ctx, 1);
    
    if (string_length(aes_key) != EVP_CIPHER_CTX_key_length(&ctx)) {
        log_err("crypto_aes_decrypt: invalid key size (%" PRIuPTR " vs expected %d)\n",
                string_length(aes_key), EVP_CIPHER_CTX_key_length(&ctx));
        goto bail_out;
    }
    if (string_length(aes_iv) != EVP_CIPHER_CTX_iv_length(&ctx)) {
        log_err("crypto_aes_decrypt: invalid iv size (%" PRIuPTR " vs expected %d)\n",
                string_length(aes_iv), EVP_CIPHER_CTX_iv_length(&ctx));
        goto bail_out;
    }

    decryptspace = string_length(ciphertext) + EVP_MAX_BLOCK_LENGTH;

    string_free(decrypted); /* free previous buffer */
    string_init(decrypted, decryptspace, 1024);
    if (string_size(decrypted) < decryptspace) {
        log_err("crypto_aes_decrypt: decrypt buffer malloc error\n");
        goto bail_out;
    }
    
    if (EVP_DecryptUpdate(&ctx, (unsigned char*)string_get(decrypted),
            &decryptdone, (unsigned char*)string_get(ciphertext),
            string_length(ciphertext))) {
        /* TODO: need cleaner way: */
        decrypted->_u._s.length = decryptdone;
    } else {
        log_err("crypto_aes_decrypt: decrypt failed\n");
        ERR_print_errors_fp(stderr);
        goto bail_out;
    }
    
    if (EVP_DecryptFinal_ex(&ctx,
            (unsigned char*)string_get(decrypted)+string_length(decrypted),
            &decryptdone)) {
        /* TODO: need cleaner way: */
        decrypted->_u._s.length += decryptdone;
    } else {
        log_err("crypto_aes_decrypt: decrypt final failed\n");
        ERR_print_errors_fp(stderr);
        goto bail_out;
    }

    retval = true;

bail_out:
    EVP_CIPHER_CTX_cleanup(&ctx);
    return retval;
}

void
crypto_init(void)
{
    /* Just load the crypto library error strings, not SSL */
    ERR_load_crypto_strings();
}

void
crypto_finish(void)
{
    ERR_free_strings();
}

void
crypto_warn_openssl_version_changed(void)
{
    /*
     * Check that the OpenSSL headers used match the version of the
     * OpenSSL library used.
     * Output a warning if not.
     */
    if (SSLeay() != OPENSSL_VERSION_NUMBER) {
        log_info("Note: compiled using OpenSSL version '%s' headers, but linked to "
          "OpenSSL version '%s' library",
          OPENSSL_VERSION_TEXT,
          SSLeay_version(SSLEAY_VERSION));
    }
}
