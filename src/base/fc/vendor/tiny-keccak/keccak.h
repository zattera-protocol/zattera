#ifndef KECCAK_H
#define KECCAK_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Keccak-256 hash function (Ethereum compatible)
 *
 * @param input  Input data buffer
 * @param len    Length of input data in bytes
 * @param output Output buffer (must be at least 32 bytes)
 */
void keccak_256(const uint8_t* input, size_t len, uint8_t* output);

/**
 * Keccak-512 hash function
 *
 * @param input  Input data buffer
 * @param len    Length of input data in bytes
 * @param output Output buffer (must be at least 64 bytes)
 */
void keccak_512(const uint8_t* input, size_t len, uint8_t* output);

#ifdef __cplusplus
}
#endif

#endif /* KECCAK_H */
