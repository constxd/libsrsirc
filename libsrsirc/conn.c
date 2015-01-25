/* conn.c - irc connection handling
 * libsrsirc - a lightweight serious IRC lib - (C) 2012-14, Timo Buhrmester
 * See README for contact-, COPYING for license information. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#define LOG_MODULE MOD_ICONN

/* C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

/* POSIX */
#include <unistd.h>
#include <sys/select.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <inttypes.h>

#ifdef WITH_SSL
/* ssl */
# include <openssl/ssl.h>
# include <openssl/err.h>
#endif

#include "common.h"
#include <libsrsirc/util.h>
#include "io.h"
#include <intlog.h>

#include "px.h"

#include "conn.h"

#define INV -1
#define OFF 0
#define ON 1


#ifdef WITH_SSL
static bool s_sslinit;
#endif

iconn
conn_init(void)
{
	iconn r = NULL;
	int preverrno = errno;
	errno = 0;

	if (!(r = com_malloc(sizeof *r)))
		goto conn_init_fail;

	r->host = NULL;

	if (!(r->host = com_strdup(DEF_HOST)))
		goto conn_init_fail;

	errno = preverrno;
	r->rctx.wptr = r->rctx.eptr = r->rctx.workbuf;
	r->port = 0;
	r->phost = NULL;
	r->pport = 0;
	r->ptype = -1;
	r->state = OFF;
	r->eof = false;
	r->colon_trail = false;
	r->ssl = false;
	r->sh.shnd = NULL;
	r->sh.sck = -1;
	r->sctx = NULL;

	D("(%p) iconn initialized", r);

	return r;

conn_init_fail:
	EE("failed to initialize iconn handle");
	if (r) {
		free(r->host);
		free(r);
	}

	return NULL;
}

void
conn_reset(iconn hnd)
{
	D("(%p) resetting", hnd);

#ifdef WITH_SSL
	if (hnd->sh.shnd) {
		D("(%p) shutting down ssl", hnd);
		SSL_shutdown(hnd->sh.shnd);
		SSL_free(hnd->sh.shnd);
		hnd->sh.shnd = NULL;
	}
#endif

	if (hnd->sh.sck != -1) {
		D("(%p) closing socket %d", hnd, hnd->sh.sck);
		close(hnd->sh.sck);
	}

	hnd->sh.sck = -1;
	hnd->state = OFF;
	hnd->rctx.wptr = hnd->rctx.eptr = hnd->rctx.workbuf;
}

void
conn_dispose(iconn hnd)
{
	conn_reset(hnd);

#ifdef WITH_SSL
	conn_set_ssl(hnd, false); //dispose ssl context if existing
#endif

	free(hnd->host);
	free(hnd->phost);
	hnd->state = INV;

	D("(%p) disposed", hnd);
	free(hnd);
}

bool
conn_connect(iconn hnd, uint64_t softto_us, uint64_t hardto_us)
{
	if (!hnd || hnd->state != OFF)
		return false;

	uint64_t tsend = hardto_us ? com_timestamp_us() + hardto_us : 0;

	uint16_t realport = hnd->port;
	if (!realport)
		realport = hnd->ssl ? DEF_PORT_SSL : DEF_PORT_PLAIN;

	char *host = hnd->ptype != -1 ? hnd->phost : hnd->host;
	uint16_t port = hnd->ptype != -1 ? hnd->pport : realport;

	{
		char ps[64];
		ps[0] = '\0';
		if (hnd->ptype != -1)
			snprintf(ps, sizeof ps, " via %s:%s:%" PRIu16,
			    px_typestr(hnd->ptype), hnd->phost, hnd->pport);

		I("(%p) wanna connect to %s:%"PRIu16"%s, "
		    "sto: %"PRIu64"us, hto: %"PRIu64"us",
		    hnd, hnd->host, realport, ps, softto_us, hardto_us);
	}

	char peerhost[256];
	uint16_t peerport;

	sckhld sh;
	sh.sck = com_consocket(host, port, peerhost, sizeof peerhost,
	    &peerport, softto_us, hardto_us);
	sh.shnd = NULL;

	if (sh.sck < 0) {
		W("(%p) com_consocket failed for %s:%"PRIu16"", hnd, host, port);
		return false;
	}

	D("(%p) connected socket %d for %s:%"PRIu16"", hnd, sh.sck, host, port);

	hnd->sh = sh; //must be set here for px_logon

	uint64_t trem = 0;
	if (hnd->ptype != -1) {
		if (com_check_timeout(tsend, &trem)) {
			W("(%p) timeout", hnd);
			close(sh.sck);
			hnd->sh.sck = -1;
			return false;
		}

		bool ok = false;
		D("(%p) logging on to proxy", hnd);
		if (hnd->ptype == IRCPX_HTTP)
			ok = px_logon_http(hnd->sh.sck, hnd->host,
			    realport, trem);
		else if (hnd->ptype == IRCPX_SOCKS4)
			ok = px_logon_socks4(hnd->sh.sck, hnd->host,
			    realport, trem);
		else if (hnd->ptype == IRCPX_SOCKS5)
			ok = px_logon_socks5(hnd->sh.sck, hnd->host,
			    realport, trem);

		if (!ok) {
			W("(%p) proxy logon failed", hnd);
			close(sh.sck);
			hnd->sh.sck = -1;
			return false;
		}
		D("(%p) sent proxy logon sequence", hnd);
	}

	D("(%p) setting to blocking mode", hnd);
	errno = 0;
	if (fcntl(sh.sck, F_SETFL, 0) == -1) {
		WE("(%p) failed to clear nonblocking mode", hnd);
		close(sh.sck);
		hnd->sh.sck = -1;
		return false;
	}
#ifdef WITH_SSL
	if (hnd->ssl) {
		bool fail = false;
		fail = !(hnd->sh.shnd = SSL_new(hnd->sctx));
		fail = fail || !SSL_set_fd(hnd->sh.shnd, sh.sck);
		int r = SSL_connect(hnd->sh.shnd);
		D("ssl_connect: %d", r);
		if (r != 1) {
			int rr = SSL_get_error(hnd->sh.shnd, r);
			D("rr: %d", rr);
		}
		fail = fail || (r != 1);
		if (fail) {
			ERR_print_errors_fp(stderr);
			if (hnd->sh.shnd) {
				SSL_free(hnd->sh.shnd);
				hnd->sh.shnd = NULL;
			}

			close(sh.sck);
			W("connect bailing out; couldn't initiate ssl");
			return false;
		}
	}
#endif

	hnd->state = ON;

	D("(%p) %s connection to ircd established",
	    hnd, hnd->ptype == -1?"TCP":"proxy");

	return true;
}

int
conn_read(iconn hnd, tokarr *tok, uint64_t to_us)
{
	if (!hnd || hnd->state != ON)
		return -1;

	int n;
	if (!(n = io_read(hnd->sh, &hnd->rctx, tok, to_us)))
		return 0; /* timeout */

	if (n < 0) {
		W("(%p) io_read %s", hnd, n == -1 ? "failed":"EOF");
		conn_reset(hnd);
		hnd->eof = n == -2;
		return -1;
	}

	size_t last = 2;
	for (; last < COUNTOF(*tok) && (*tok)[last]; last++);

	if (last > 2)
		hnd->colon_trail = (*tok)[last-1][-1] == ':';

	D("(%p) got a msg ('%s', %zu args)", hnd, (*tok)[1], last);

	return 1;
}

bool
conn_write(iconn hnd, const char *line)
{
	if (!hnd || hnd->state != ON || !line)
		return false;


	if (!io_write(hnd->sh, line)) {
		W("(%p) failed to write '%s'", hnd, line);
		conn_reset(hnd);
		hnd->eof = false;
		return false;
	}

	D("(%p) wrote: '%s'", hnd, line);
	return true;

}

bool
conn_online(iconn hnd)
{
	return hnd->state == ON;
}

bool
conn_eof(iconn hnd)
{
	return hnd->eof;
}

bool
conn_colon_trail(iconn hnd)
{
	if (!hnd || hnd->state != ON)
		return false;

	return hnd->colon_trail;
}

bool
conn_set_px(iconn hnd, const char *host, uint16_t port, int ptype)
{
	char *n = NULL;
	switch (ptype) {
	case IRCPX_HTTP:
	case IRCPX_SOCKS4:
	case IRCPX_SOCKS5:
		if (!host || !port) //XXX `most' default port per type?
			return false;

		if (!(n = com_strdup(host)))
			return false;

		hnd->pport = port;
		hnd->ptype = ptype;
		free(hnd->phost);
		hnd->phost = n;
		I("set proxy to %s:%s:%"PRIu16, px_typestr(hnd->ptype), n, port);
		break;
	default:
		E("illegal proxy type %d", ptype);
		return false;
	}

	return true;
}

bool
conn_set_server(iconn hnd, const char *host, uint16_t port)
{
	char *n;
	if (!(n = com_strdup(host?host:DEF_HOST)))
		return false;

	free(hnd->host);
	hnd->host = n;
	hnd->port = port;
	I("set server to %s:%"PRIu16, n, port);
	return true;
}

bool
conn_set_ssl(iconn hnd, bool on)
{
#ifndef WITH_SSL
	(void)hnd;
	if (on)
		W("library was not compiled with SSL support");
	return false;
#else

	if (!s_sslinit && on) {
		SSL_load_error_strings();
		SSL_library_init();
		s_sslinit = true;
	}

	if (on && !hnd->sctx) {
		if (!(hnd->sctx = SSL_CTX_new(SSLv23_client_method()))) {
			W("SSL_CTX_new failed, ssl not enabled!");
			return false;
		}
	} else if (!on && hnd->sctx) {
		SSL_CTX_free(hnd->sctx);
		hnd->sctx = NULL;
	}

	I("ssl %sabled", on ? "en" : "dis");
	hnd->ssl = on;

	return true;
#endif
}

const char*
conn_get_px_host(iconn hnd)
{
	return hnd->phost;
}

uint16_t
conn_get_px_port(iconn hnd)
{
	return hnd->pport;
}

int
conn_get_px_type(iconn hnd)
{
	return hnd->ptype;
}

const char*
conn_get_host(iconn hnd)
{
	return hnd->host;
}

uint16_t
conn_get_port(iconn hnd)
{
	return hnd->port;
}

bool
conn_get_ssl(iconn hnd)
{
#ifdef WITH_SSL
	return hnd->ssl;
#else
	(void)hnd;
	return false;
#endif
}

int
conn_sockfd(iconn hnd)
{
	if (!hnd || hnd->state != ON)
		return -1;

	return hnd->sh.sck;
}


