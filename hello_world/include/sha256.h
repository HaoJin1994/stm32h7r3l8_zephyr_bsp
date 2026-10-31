/*
 * Minimal self-contained SHA-256 (no crypto library dependency).
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTA_SHA256_H
#define OTA_SHA256_H

#include <stdint.h>
#include <stddef.h>

#define SHA256_DIGEST_SIZE 32

struct sha256_ctx {
	uint32_t state[8];
	uint64_t count;		/* total bytes fed */
	uint8_t buf[64];
	size_t buflen;
};

void sha256_init(struct sha256_ctx *ctx);
void sha256_update(struct sha256_ctx *ctx, const void *data, size_t len);
void sha256_final(struct sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_SIZE]);

/* digest -> lowercase hex string, out must be >= 2*SHA256_DIGEST_SIZE+1 */
void sha256_hex(const uint8_t digest[SHA256_DIGEST_SIZE], char *out);

#endif /* OTA_SHA256_H */
