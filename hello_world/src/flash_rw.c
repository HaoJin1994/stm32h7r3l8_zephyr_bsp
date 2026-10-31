/*
 * MCUboot RAM_LOAD OTA over TCP.
 *
 * Protocol (client -> server):
 *   [4 bytes big-endian]  total length of the signed image
 *   [32 bytes]            expected SHA-256 of the whole signed image
 *   [N bytes]             signed image (zephyr.signed.bin)
 *
 * Server replies with a single line: "OK <sha256>\n" or "ERR <reason>\n".
 * On success the device reboots and MCUboot picks the newer image.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/socket.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "sha256.h"

LOG_MODULE_REGISTER(ota, LOG_LEVEL_INF);

#define OTA_PORT         14500
#define OTA_STACK_SIZE   4096
#define OTA_PRIO         7
#define OTA_ACCEPT_TO_MS 2000

#define OTA_WBUF_SIZE    1024
#define OTA_RX_SIZE      1460

/* MCUboot image header, little endian, 32 bytes (see MCUboot docs/design.md) */
#define IMAGE_MAGIC 0x96f3b83dU

struct mcuboot_hdr {
	uint32_t magic;
	uint32_t load_addr;
	uint16_t hdr_size;
	uint16_t prot_tlv_size;
	uint32_t img_size;
	uint32_t flags;
	uint8_t ver_major;
	uint8_t ver_minor;
	uint16_t ver_revision;
	uint32_t ver_build;
	uint32_t pad;
} __packed;

/* ------------------------------------------------------------------ */
/* helpers                                                             */
/* ------------------------------------------------------------------ */

static int recv_exact(int fd, uint8_t *buf, size_t len)
{
	size_t got = 0;

	while (got < len) {
		int n = zsock_recv(fd, buf + got, len - got, 0);

		if (n < 0) {
			return -errno;
		}
		if (n == 0) {
			return -ECONNRESET;	/* peer closed early */
		}
		got += (size_t)n;
	}
	return 0;
}

static int send_line(int fd, const char *s)
{
	size_t len = strlen(s);
	size_t sent = 0;

	while (sent < len) {
		int n = zsock_send(fd, s + sent, len - sent, 0);

		if (n < 0) {
			return -errno;
		}
		sent += (size_t)n;
	}
	return 0;
}

/* Erase the slot lazily, one sector at a time, up to (but not past) 'end' */
static int ensure_erased(const struct flash_area *fa, off_t end,
			 off_t *erased_end, size_t sector)
{
	while (*erased_end < end) {
		int rc = flash_area_erase(fa, *erased_end, sector);

		if (rc != 0) {
			LOG_ERR("erase @0x%lx failed: %d", (long)*erased_end, rc);
			return rc;
		}
		*erased_end += sector;
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/* one OTA session                                                     */
/* ------------------------------------------------------------------ */

static int ota_session(int cli)
{
	const struct flash_area *fa = NULL;
	struct flash_pages_info pi;
	struct sha256_ctx sha;
	static uint8_t rx[OTA_RX_SIZE];
	static uint8_t wbuf[OTA_WBUF_SIZE];
	uint8_t expect[SHA256_DIGEST_SIZE];
	uint8_t calc[SHA256_DIGEST_SIZE];
	uint8_t lenbuf[4];
	uint8_t hdrbuf[sizeof(struct mcuboot_hdr)];
	char hex[2 * SHA256_DIGEST_SIZE + 1];
	char line[96];
	struct mcuboot_hdr new_hdr;
	off_t off = 0;
	off_t erased_end = 0;
	size_t sector;
	size_t wbs;
	size_t wlen = 0;
	uint32_t total;
	uint32_t remaining;
	int rc;

	/* ---- 1. header of the transfer ---- */
	rc = recv_exact(cli, lenbuf, sizeof(lenbuf));
	if (rc != 0) {
		send_line(cli, "ERR recv length\n");
		return rc;
	}
	total = ((uint32_t)lenbuf[0] << 24) | ((uint32_t)lenbuf[1] << 16) |
		((uint32_t)lenbuf[2] << 8) | (uint32_t)lenbuf[3];

	rc = recv_exact(cli, expect, sizeof(expect));
	if (rc != 0) {
		send_line(cli, "ERR recv sha256\n");
		return rc;
	}

	sha256_hex(expect, hex);
	LOG_INF("OTA request: %u bytes, expect sha256=%s", (unsigned)total, hex);

	rc = flash_area_open(FIXED_PARTITION_ID(slot1_partition), &fa);
	if (rc != 0) {
		LOG_ERR("flash_area_open(slot1) failed: %d", rc);
		send_line(cli, "ERR open slot1\n");
		return rc;
	}

	if (total == 0 || total > fa->fa_size) {
		LOG_ERR("bad image size %u (slot is %zu)", (unsigned)total, fa->fa_size);
		send_line(cli, "ERR bad size\n");
		flash_area_close(fa);
		return -EINVAL;
	}

	/* erase granularity = page size at the beginning of the slot */
	rc = flash_get_page_info_by_offs(fa->fa_dev, fa->fa_off, &pi);
	if (rc != 0 || pi.size == 0) {
		LOG_WRN("page info unavailable (%d), assuming 4K sectors", rc);
		sector = 4096;
	} else {
		sector = pi.size;
	}

	wbs = flash_get_write_block_size(fa->fa_dev);
	if (wbs == 0) {
		wbs = 1;
	}

	LOG_INF("slot1: dev_off=0x%lx size=%zu sector=%zu write_block=%zu",
		(long)fa->fa_off, fa->fa_size, sector, wbs);

	/* ---- 2. receive, hash and write ---- */
	sha256_init(&sha);
	remaining = total;

	while (remaining > 0) {
		int n = zsock_recv(cli, rx, MIN((size_t)remaining, sizeof(rx)), 0);

		if (n < 0) {
			LOG_ERR("recv failed: %d", -errno);
			send_line(cli, "ERR recv data\n");
			flash_area_close(fa);
			return -errno;
		}
		if (n == 0) {
			LOG_ERR("peer closed, %u bytes missing", (unsigned)remaining);
			send_line(cli, "ERR truncated\n");
			flash_area_close(fa);
			return -ECONNRESET;
		}

		remaining -= (uint32_t)n;
		sha256_update(&sha, rx, (size_t)n);

		/* buffer -> flash, always in full 1K chunks */
		{
			const uint8_t *p = rx;
			size_t left = (size_t)n;

			while (left > 0) {
				size_t take = MIN(left, sizeof(wbuf) - wlen);

				memcpy(wbuf + wlen, p, take);
				wlen += take;
				p += take;
				left -= take;

				if (wlen == sizeof(wbuf)) {
					rc = ensure_erased(fa, off + wlen, &erased_end, sector);
					if (rc != 0) {
						send_line(cli, "ERR erase\n");
						flash_area_close(fa);
						return rc;
					}
					rc = flash_area_write(fa, off, wbuf, wlen);
					if (rc != 0) {
						LOG_ERR("write @0x%lx failed: %d", (long)off, rc);
						send_line(cli, "ERR write\n");
						flash_area_close(fa);
						return rc;
					}
					off += wlen;
					wlen = 0;
				}
			}
		}
	}

	/* flush the tail: pad with 0xff up to a write-block multiple (NOR flash) */
	if (wlen != 0) {
		size_t rem = wlen % wbs;

		if (rem != 0) {
			memset(wbuf + wlen, 0xff, wbs - rem);
			wlen += wbs - rem;
		}
		rc = ensure_erased(fa, off + wlen, &erased_end, sector);
		if (rc != 0) {
			send_line(cli, "ERR erase\n");
			flash_area_close(fa);
			return rc;
		}
		rc = flash_area_write(fa, off, wbuf, wlen);
		if (rc != 0) {
			LOG_ERR("write @0x%lx failed: %d", (long)off, rc);
			send_line(cli, "ERR write\n");
			flash_area_close(fa);
			return rc;
		}
		off += wlen;
		wlen = 0;
	}

	sha256_final(&sha, calc);
	sha256_hex(calc, hex);
	LOG_INF("written %lu bytes, sha256=%s", (unsigned long)off, hex);

	/* ---- 3. verify ---- */
	if (memcmp(calc, expect, SHA256_DIGEST_SIZE) != 0) {
		sha256_hex(expect, hex);
		LOG_ERR("sha256 mismatch, expected %s", hex);
		send_line(cli, "ERR sha256 mismatch\n");
		flash_area_close(fa);
		return -EBADMSG;
	}
	LOG_INF("sha256 OK");

	/* read the MCUboot header back from flash and sanity check it */
	rc = flash_area_read(fa, 0, hdrbuf, sizeof(hdrbuf));
	if (rc != 0) {
		LOG_ERR("read back header failed: %d", rc);
		send_line(cli, "ERR readback\n");
		flash_area_close(fa);
		return rc;
	}
	memcpy(&new_hdr, hdrbuf, sizeof(new_hdr));

	if (new_hdr.magic != IMAGE_MAGIC) {
		LOG_ERR("bad image magic 0x%08x", (unsigned)new_hdr.magic);
		send_line(cli, "ERR bad magic\n");
		flash_area_close(fa);
		return -EBADMSG;
	}

	LOG_INF("new image: v%u.%u.%u+%u, load_addr=0x%08x, img_size=%u, hdr_size=%u",
		new_hdr.ver_major, new_hdr.ver_minor, new_hdr.ver_revision,
		(unsigned)new_hdr.ver_build, (unsigned)new_hdr.load_addr,
		(unsigned)new_hdr.img_size, (unsigned)new_hdr.hdr_size);

	if (new_hdr.hdr_size + new_hdr.img_size > total) {
		LOG_ERR("header %u + payload %u exceeds %u",
			(unsigned)new_hdr.hdr_size, (unsigned)new_hdr.img_size,
			(unsigned)total);
		send_line(cli, "ERR bad img_size\n");
		flash_area_close(fa);
		return -EBADMSG;
	}

	/* RAM_LOAD picks the image with the higher version: warn if not newer */
	{
		const struct flash_area *fa0 = NULL;

		rc = flash_area_open(FIXED_PARTITION_ID(slot0_partition), &fa0);
		if (rc == 0) {
			struct mcuboot_hdr cur;

			rc = flash_area_read(fa0, 0, hdrbuf, sizeof(hdrbuf));
			if (rc == 0) {
				memcpy(&cur, hdrbuf, sizeof(cur));
				if (cur.magic == IMAGE_MAGIC) {
					uint32_t new_v = ((uint32_t)new_hdr.ver_major << 16) |
							 ((uint32_t)new_hdr.ver_minor << 8) |
							 new_hdr.ver_revision;
					uint32_t cur_v = ((uint32_t)cur.ver_major << 16) |
							 ((uint32_t)cur.ver_minor << 8) |
							 cur.ver_revision;

					LOG_INF("slot0 image: v%u.%u.%u+%u", cur.ver_major,
						cur.ver_minor, cur.ver_revision,
						(unsigned)cur.ver_build);
					if (new_v <= cur_v) {
						LOG_WRN("new image version is not higher "
							"-> RAM_LOAD will keep booting slot0");
					}
				}
			}
			flash_area_close(fa0);
		}
	}

	flash_area_close(fa);

	/* ---- 4. done ---- */
	snprintf(line, sizeof(line), "OK %s\n", hex);
	send_line(cli, line);

	LOG_INF("OTA success, rebooting in 500 ms ...");
	k_msleep(500);

	/* MCUboot validates the signature and copies the newer image to RAM */
	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

/* ------------------------------------------------------------------ */
/* TCP server thread                                                   */
/* ------------------------------------------------------------------ */

static void ota_thread(void *p1, void *p2, void *p3)
{
	struct sockaddr_in addr;
	struct zsock_timeval tv;
	int srv;
	int rc;
	int opt;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* give the network stack some time to come up */
	k_msleep(3000);

	srv = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (srv < 0) {
		LOG_ERR("socket() failed: %d", errno);
		return;
	}

	opt = 1;
	zsock_setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = sys_cpu_to_be16(OTA_PORT);
	addr.sin_addr.s_addr = 0;	/* INADDR_ANY */

	rc = zsock_bind(srv, (const struct sockaddr *)&addr, sizeof(addr));
	if (rc < 0) {
		LOG_ERR("bind() failed: %d", errno);
		zsock_close(srv);
		return;
	}

	rc = zsock_listen(srv, 1);
	if (rc < 0) {
		LOG_ERR("listen() failed: %d", errno);
		zsock_close(srv);
		return;
	}

	LOG_INF("OTA server listening on TCP port %u", OTA_PORT);

	while (1) {
		int cli = zsock_accept(srv, NULL, NULL);

		if (cli < 0) {
			LOG_DBG("accept() failed: %d", errno);
			k_msleep(OTA_ACCEPT_TO_MS);
			continue;
		}

		/* do not hang forever on a dead peer */
		tv.tv_sec = 30;
		tv.tv_usec = 0;
		zsock_setsockopt(cli, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		zsock_setsockopt(cli, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

		ota_session(cli);

		zsock_close(cli);
	}
}

K_THREAD_DEFINE(ota_tid, OTA_STACK_SIZE, ota_thread, NULL, NULL, NULL,
		OTA_PRIO, 0, 0);
