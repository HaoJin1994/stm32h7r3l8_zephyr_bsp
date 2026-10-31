/*
 * Minimal self-contained SHA-256 implementation.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "sha256.h"

static const uint32_t sha256_k[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static uint32_t be32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void put_be32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >> 8);
	p[3] = (uint8_t)v;
}

static void sha256_compress(struct sha256_ctx *ctx, const uint8_t block[64])
{
	uint32_t w[64];
	uint32_t a, b, c, d, e, f, g, h;
	int i;

	for (i = 0; i < 16; i++) {
		w[i] = be32(block + 4 * i);
	}
	for (i = 16; i < 64; i++) {
		uint32_t s0 = ROR32(w[i - 15], 7) ^ ROR32(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = ROR32(w[i - 2], 17) ^ ROR32(w[i - 2], 19) ^ (w[i - 2] >> 10);

		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; i++) {
		uint32_t s1 = ROR32(e, 6) ^ ROR32(e, 11) ^ ROR32(e, 25);
		uint32_t ch = (e & f) ^ (~e & g);
		uint32_t t1 = h + s1 + ch + sha256_k[i] + w[i];
		uint32_t s0 = ROR32(a, 2) ^ ROR32(a, 13) ^ ROR32(a, 22);
		uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
		uint32_t t2 = s0 + maj;

		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void sha256_init(struct sha256_ctx *ctx)
{
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
	ctx->count = 0;
	ctx->buflen = 0;
	memset(ctx->buf, 0, sizeof(ctx->buf));
}

void sha256_update(struct sha256_ctx *ctx, const void *data, size_t len)
{
	const uint8_t *p = data;

	ctx->count += len;

	/* Fill the pending partial block first */
	if (ctx->buflen != 0) {
		size_t n = 64 - ctx->buflen;

		if (n > len) {
			n = len;
		}
		memcpy(ctx->buf + ctx->buflen, p, n);
		ctx->buflen += n;
		p += n;
		len -= n;

		if (ctx->buflen == 64) {
			sha256_compress(ctx, ctx->buf);
			ctx->buflen = 0;
		}
	}

	while (len >= 64) {
		sha256_compress(ctx, p);
		p += 64;
		len -= 64;
	}

	if (len != 0) {
		memcpy(ctx->buf + ctx->buflen, p, len);
		ctx->buflen += len;
	}
}

void sha256_final(struct sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_SIZE])
{
	uint64_t bits = ctx->count * 8U;
	uint8_t tail[64 + 8];
	size_t tail_len;
	size_t pad_len;
	int i;

	/* 0x80, then zeros until length % 64 == 56 */
	memcpy(tail, ctx->buf, ctx->buflen);
	tail[ctx->buflen] = 0x80;
	tail_len = ctx->buflen + 1;
	pad_len = (64 - ((tail_len + 8U) % 64U)) % 64U;
	memset(tail + tail_len, 0, pad_len);
	tail_len += pad_len;

	put_be32(tail + tail_len, (uint32_t)(bits >> 32));
	put_be32(tail + tail_len + 4, (uint32_t)bits);
	tail_len += 8;

	for (i = 0; (size_t)i < tail_len; i += 64) {
		sha256_compress(ctx, tail + i);
	}

	for (i = 0; i < 8; i++) {
		put_be32(out + 4 * i, ctx->state[i]);
	}
}

void sha256_hex(const uint8_t digest[SHA256_DIGEST_SIZE], char *out)
{
	static const char hex[] = "0123456789abcdef";
	int i;

	for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
		out[2 * i] = hex[digest[i] >> 4];
		out[2 * i + 1] = hex[digest[i] & 0x0f];
	}
	out[2 * SHA256_DIGEST_SIZE] = '\0';
}
