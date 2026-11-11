#ifndef HOD_RELEASE_SIGNATURE_H
#define HOD_RELEASE_SIGNATURE_H

#include <stdbool.h>
#include <stddef.h>

#define RELEASE_SIGNATURE_BYTES 64
#define RELEASE_PUBLIC_KEY_BYTES 32

bool ReleaseSignatureValid(
    const unsigned char *message, size_t length, const unsigned char *signature, const unsigned char *public_key
);

#endif
