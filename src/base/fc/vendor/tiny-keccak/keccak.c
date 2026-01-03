/*
 * Keccak-256 implementation
 * Based on the Keccak reference implementation
 * Public Domain
 */

#include "keccak.h"
#include <string.h>

#define KECCAK_ROUNDS 24

static const uint64_t keccak_round_constants[KECCAK_ROUNDS] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

static const unsigned keccak_rotc[24] = {
    1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
    27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
};

static const unsigned keccak_piln[24] = {
    10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
};

#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))

static void keccak_f1600(uint64_t state[25])
{
    uint64_t t, bc[5];

    for (int round = 0; round < KECCAK_ROUNDS; round++) {
        /* Theta */
        for (int i = 0; i < 5; i++)
            bc[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];

        for (int i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
            for (int j = 0; j < 25; j += 5)
                state[j + i] ^= t;
        }

        /* Rho Pi */
        t = state[1];
        for (int i = 0; i < 24; i++) {
            int j = keccak_piln[i];
            bc[0] = state[j];
            state[j] = ROTL64(t, keccak_rotc[i]);
            t = bc[0];
        }

        /* Chi */
        for (int j = 0; j < 25; j += 5) {
            for (int i = 0; i < 5; i++)
                bc[i] = state[j + i];
            for (int i = 0; i < 5; i++)
                state[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        /* Iota */
        state[0] ^= keccak_round_constants[round];
    }
}

static void keccak(const uint8_t* input, size_t len, uint8_t* output, size_t output_len, size_t rate)
{
    uint64_t state[25] = {0};
    uint8_t* state_bytes = (uint8_t*)state;
    size_t rate_bytes = rate / 8;

    /* Absorb */
    while (len >= rate_bytes) {
        for (size_t i = 0; i < rate_bytes; i++)
            state_bytes[i] ^= input[i];

        keccak_f1600(state);
        input += rate_bytes;
        len -= rate_bytes;
    }

    /* Absorb last block */
    for (size_t i = 0; i < len; i++)
        state_bytes[i] ^= input[i];

    /* Padding (original Keccak: pad10*1) */
    state_bytes[len] ^= 0x01;
    state_bytes[rate_bytes - 1] ^= 0x80;

    keccak_f1600(state);

    /* Squeeze */
    memcpy(output, state_bytes, output_len);
}

void keccak_256(const uint8_t* input, size_t len, uint8_t* output)
{
    keccak(input, len, output, 32, 1088);  /* rate = 1600 - 2*256 = 1088 bits */
}

void keccak_512(const uint8_t* input, size_t len, uint8_t* output)
{
    keccak(input, len, output, 64, 576);   /* rate = 1600 - 2*512 = 576 bits */
}
