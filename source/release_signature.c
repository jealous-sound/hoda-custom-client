#include "release_signature.h"

#include <stdlib.h>
#include <string.h>

#include "vendors/tweetnacl.c"

void randombytes(unsigned char *out, unsigned long long count) {
    (void)out;
    (void)count;
    abort();
}

bool ReleaseSignatureValid(
    const unsigned char *message, size_t length, const unsigned char *signature, const unsigned char *public_key
) {
    size_t signed_length = length + RELEASE_SIGNATURE_BYTES;
    unsigned char *signed_message = malloc(signed_length);
    unsigned char *opened = malloc(signed_length);
    bool valid = false;
    if (signed_message && opened) {
        unsigned long long opened_length = 0;
        memcpy(signed_message, signature, RELEASE_SIGNATURE_BYTES);
        memcpy(signed_message + RELEASE_SIGNATURE_BYTES, message, length);
        valid = crypto_sign_open(opened, &opened_length, signed_message, signed_length, public_key) == 0 &&
                opened_length == length;
    }
    free(signed_message);
    free(opened);
    return valid;
}
