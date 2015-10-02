/* common.c - lib-internal IRC-unrelated common routines
 * libsrsirc - a lightweight serious IRC lib - (C) 2012-15, Timo Buhrmester
 * See README for contact-, COPYING for license information. */

#define LOG_MODULE MOD_COMMON

#if HAVE_CONFIG_H
# include <config.h>
#endif


#include "common.h"


#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <platform/base_net.h>
#include <platform/base_string.h>
#include <platform/base_time.h>

#include <logger/intlog.h>

#include <libsrsirc/defs.h>


static int tryhost(struct addrlist *ai, char *remaddr, size_t remaddr_sz,
    uint16_t *peerport, uint64_t to_us);

void
lsi_com_strNcat(char *dest, const char *src, size_t destsz)
{ T("trace");
	size_t len = strlen(dest);
	if (len + 1 >= destsz)
		return;
	size_t rem = destsz - (len + 1);

	char *ptr = dest + len;
	while (rem-- && *src) {
		*ptr++ = *src++;
	}
	*ptr = '\0';
}

size_t
lsi_com_strCchr(const char *dst, char c)
{ T("trace");
	size_t r = 0;
	while (*dst)
		if (*dst++ == c)
			r++;
	return r;
}


char *
lsi_com_strNcpy(char *dst, const char *src, size_t dst_sz)
{ T("trace");
	dst[dst_sz-1] = '\0';
	while (--dst_sz)
		if (!(*dst++ = *src++))
			break;
	return dst;
}

int
lsi_com_consocket(const char *host, uint16_t port, char *remaddr,
    size_t remaddr_sz, uint16_t *peerport, uint64_t softto, uint64_t hardto)
{ T("trace");
	uint64_t hardtsend = hardto ? lsi_b_tstamp_us() + hardto : 0;

	struct addrlist *alist;
	int count = lsi_b_mkaddrlist(host, port, &alist);
	if (count <= 0)
		return -1;

	if (softto && hardto && softto * count < hardto)
		softto = hardto / count;

	int sck = -1;
	for (struct addrlist *ai = alist; ai; ai = ai->next) {
		uint64_t trem = 0;

		if (lsi_com_check_timeout(hardtsend, &trem)) {
			W("hard timeout");
			return false;
		}

		if (trem > softto)
			trem = softto;

		sck = tryhost(ai, remaddr, remaddr_sz, peerport, trem);

		if (sck != -1)
			break;
	}

	lsi_b_freeaddrlist(alist);

	return sck;
}

static int
tryhost(struct addrlist *ai, char *remaddr, size_t remaddr_sz,
    uint16_t *peerport, uint64_t to_us)
{ T("trace");
	int sck = lsi_b_socket(ai->ipv6);

	if (sck == -1)
		return -1;

	if (!lsi_b_blocking(sck, false))
		W("failed to set socket non-blocking, timeout will not work");

	int r = lsi_b_connect(sck, ai);
	if (r == 1)
		return sck;
	else if (r == -1)
		goto tryhost_fail;

	r = lsi_b_select(sck, false, to_us);

	if (r == 1) {
		if (!lsi_b_blocking(sck, true))
			W("failed to clear socket non-blocking mode");

		if (lsi_b_sock_ok(sck)) {
			if (remaddr && remaddr_sz)
				lsi_b_strNcpy(remaddr, ai->addrstr, remaddr_sz);
			if (peerport)
				*peerport = ai->port;

			return sck;
		} else
			W("could not connect socket");

	}

	/* fall-thru */
tryhost_fail:
	if (sck != -1)
		lsi_b_close(sck);
	return -1;
}


bool
lsi_com_update_strprop(char **field, const char *val)
{ T("trace");
	char *n = NULL;
	if (val && !(n = lsi_b_strdup(val)))
		return false;

	free(*field);
	*field = n;

	return true;
}

bool
lsi_com_check_timeout(uint64_t tsend, uint64_t *trem)
{ T("trace");
	if (!tsend) {
		if (trem)
			*trem = 0;
		return false;
	}

	uint64_t now = lsi_b_tstamp_us();
	if (now >= tsend) {
		if (trem)
			*trem = 0;
		return true;
	}

	if (trem)
		*trem = tsend - now;

	return false;
}

void *
lsi_com_malloc(size_t sz)
{ T("trace");
	void *r = malloc(sz);
	if (!r)
		EE("malloc"); /* NOTE: This does NOT call exit() or anything */
	return r;
}

/*dumb heuristic to tell apart domain name/ip4/ip6 addr XXX FIXME */
enum hosttypes
lsi_guess_hosttype(const char *host)
{ T("trace");
	if (strchr(host, '['))
		return HOSTTYPE_IPV6;
	int dc = 0;
	while (*host) {
		if (*host == '.')
			dc++;
		else if (!isdigit((unsigned char)*host))
			return HOSTTYPE_DNS;
		host++;
	}
	return dc == 3 ? HOSTTYPE_IPV4 : HOSTTYPE_DNS;
}
