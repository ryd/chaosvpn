#include <openssl/evp.h>

extern EVP_PKEY *crypto_load_key(const char *key, bool is_private);
extern int crypto_rsa_verify_signature(struct string *databuffer, struct string *signature, const char *pubkey);
extern int crypto_rsa_decrypt(struct string *ciphertext, char *privkey, struct string *decrypted);
extern int crypto_aes_decrypt(struct string *ciphertext, struct string *aes_key, struct string *aes_iv, struct string *decrypted);
