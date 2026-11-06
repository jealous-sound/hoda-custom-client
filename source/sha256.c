#define SHA256_BLOCK_BYTES 64
#define SHA256_DIGEST_BYTES 32
#define SHA256_HEX_CAP (SHA256_DIGEST_BYTES * 2 + 1)
#define SHA256_STATE_WORDS 8
#define SHA256_SCHEDULE_WORDS 64
#define SHA256_LENGTH_BYTES 8
#define SHA256_PADDING_MARKER 0x80u
#define SHA256_BITS_PER_BYTE 8u

typedef struct {
    uint32_t state[SHA256_STATE_WORDS];
    uint64_t length;
    unsigned char block[SHA256_BLOCK_BYTES];
    size_t used;
} Sha256;

static const uint32_t SHA256_INITIAL_STATE[SHA256_STATE_WORDS] = {
    0x6a09e667u,
    0xbb67ae85u,
    0x3c6ef372u,
    0xa54ff53au,
    0x510e527fu,
    0x9b05688cu,
    0x1f83d9abu,
    0x5be0cd19u,
};

static const uint32_t SHA256_ROUND_CONSTANTS[SHA256_SCHEDULE_WORDS] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

static uint32_t Sha256Rotate(uint32_t value, unsigned int bits) {
    return (value >> bits) | (value << (32u - bits));
}

static uint32_t Sha256ReadBigEndian(const unsigned char *bytes) {
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

static void Sha256Compress(Sha256 *hash, const unsigned char *block) {
    uint32_t schedule[SHA256_SCHEDULE_WORDS];
    for (int i = 0; i < 16; i++) schedule[i] = Sha256ReadBigEndian(block + i * 4);
    for (int i = 16; i < SHA256_SCHEDULE_WORDS; i++) {
        uint32_t early = schedule[i - 15], late = schedule[i - 2];
        uint32_t sigma0 = Sha256Rotate(early, 7) ^ Sha256Rotate(early, 18) ^ (early >> 3);
        uint32_t sigma1 = Sha256Rotate(late, 17) ^ Sha256Rotate(late, 19) ^ (late >> 10);
        schedule[i] = schedule[i - 16] + sigma0 + schedule[i - 7] + sigma1;
    }
    uint32_t a = hash->state[0], b = hash->state[1], c = hash->state[2], d = hash->state[3];
    uint32_t e = hash->state[4], f = hash->state[5], g = hash->state[6], h = hash->state[7];
    for (int i = 0; i < SHA256_SCHEDULE_WORDS; i++) {
        uint32_t sum1 = Sha256Rotate(e, 6) ^ Sha256Rotate(e, 11) ^ Sha256Rotate(e, 25);
        uint32_t choice = (e & f) ^ (~e & g);
        uint32_t first = h + sum1 + choice + SHA256_ROUND_CONSTANTS[i] + schedule[i];
        uint32_t sum0 = Sha256Rotate(a, 2) ^ Sha256Rotate(a, 13) ^ Sha256Rotate(a, 22);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t second = sum0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + first;
        d = c;
        c = b;
        b = a;
        a = first + second;
    }
    hash->state[0] += a;
    hash->state[1] += b;
    hash->state[2] += c;
    hash->state[3] += d;
    hash->state[4] += e;
    hash->state[5] += f;
    hash->state[6] += g;
    hash->state[7] += h;
}

static void Sha256Begin(Sha256 *hash) {
    *hash = (Sha256){0};
    memcpy(hash->state, SHA256_INITIAL_STATE, sizeof(hash->state));
}

static void Sha256Update(Sha256 *hash, const void *data, size_t size) {
    const unsigned char *bytes = data;
    hash->length += size;
    while (size > 0) {
        size_t taken = SHA256_BLOCK_BYTES - hash->used;
        if (taken > size) taken = size;
        memcpy(hash->block + hash->used, bytes, taken);
        hash->used += taken;
        bytes += taken;
        size -= taken;
        if (hash->used < SHA256_BLOCK_BYTES) break;
        Sha256Compress(hash, hash->block);
        hash->used = 0;
    }
}

static void Sha256Finish(Sha256 *hash, unsigned char *digest) {
    uint64_t bits = hash->length * SHA256_BITS_PER_BYTE;
    unsigned char marker = SHA256_PADDING_MARKER, zero = 0;
    Sha256Update(hash, &marker, 1);
    while (hash->used != SHA256_BLOCK_BYTES - SHA256_LENGTH_BYTES) Sha256Update(hash, &zero, 1);
    unsigned char length[SHA256_LENGTH_BYTES];
    for (int i = 0; i < SHA256_LENGTH_BYTES; i++)
        length[i] = (unsigned char)(bits >> ((SHA256_LENGTH_BYTES - 1 - i) * SHA256_BITS_PER_BYTE));
    Sha256Update(hash, length, sizeof(length));
    for (int i = 0; i < SHA256_STATE_WORDS; i++) {
        digest[i * 4] = (unsigned char)(hash->state[i] >> 24);
        digest[i * 4 + 1] = (unsigned char)(hash->state[i] >> 16);
        digest[i * 4 + 2] = (unsigned char)(hash->state[i] >> 8);
        digest[i * 4 + 3] = (unsigned char)hash->state[i];
    }
}

static void Sha256FinishHex(Sha256 *hash, char *hex) {
    static const char digits[] = "0123456789abcdef";
    unsigned char digest[SHA256_DIGEST_BYTES];
    Sha256Finish(hash, digest);
    for (int i = 0; i < SHA256_DIGEST_BYTES; i++) {
        hex[i * 2] = digits[digest[i] >> 4];
        hex[i * 2 + 1] = digits[digest[i] & 0x0fu];
    }
    hex[SHA256_DIGEST_BYTES * 2] = '\0';
}
