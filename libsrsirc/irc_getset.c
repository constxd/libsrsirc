/* irc_getset.c - getters, setters, determiners
 * libsrsirc - a lightweight serious IRC lib - (C) 2012-15, Timo Buhrmester
 * See README for contact-, COPYING for license information. */

#define LOG_MODULE MOD_IRC

#if HAVE_CONFIG_H
# include <config.h>
#endif


#include <inttypes.h>
#include <stdbool.h>

#include <logger/intlog.h>

#include "common.h"
#include "conn.h"
#include "msg.h"


/* Determiners - read-only access to information we keep track of */

bool
irc_online(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return lsi_conn_online(hnd->con);
}

const char *
irc_mynick(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->mynick;
}

const char *
irc_myhost(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->myhost;
}

int
irc_casemap(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->casemap;
}

bool
irc_service(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->service;
}

const char *
irc_umodes(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->umodes;
}

const char *
irc_cmodes(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->cmodes;
}

const char *
irc_version(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->ver;
}

const char *
irc_lasterror(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->lasterr;
}

const char *
irc_banmsg(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->banmsg;
}

bool
irc_banned(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->banned;
}

bool
irc_colon_trail(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return lsi_conn_colon_trail(hnd->con);
}

int
irc_sockfd(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return lsi_conn_sockfd(hnd->con);
}

tokarr *(* /* ew.  returns pointer to array of 4 pointers to tokarr */
irc_logonconv(irc hnd))[4]
{ T("hnd=%p", (void *)hnd);
	return &hnd->logonconv;
}

const char *
irc_005chanmodes(irc hnd, size_t class) /* suck it, $(CXX) */
{ T("hnd=%p, class=%zu", (void *)hnd, class);
	if (class >= COUNTOF(hnd->m005chanmodes))
		return NULL;
	return hnd->m005chanmodes[class];
}

const char *
irc_005modepfx(irc hnd, bool symbols)
{ T("hnd=%p, symbols=%d", (void *)hnd, symbols);
	return hnd->m005modepfx[symbols];
}


bool
irc_tracking_enab(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->tracking && hnd->tracking_enab;
}

/* Getters - retrieve values previously set by the Setters */

const char *
irc_get_host(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return lsi_conn_get_host(hnd->con);
}

uint16_t
irc_get_port(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return lsi_conn_get_port(hnd->con);
}

const char *
irc_get_px_host(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return lsi_conn_get_px_host(hnd->con);
}

uint16_t
irc_get_px_port(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return lsi_conn_get_px_port(hnd->con);
}

int
irc_get_px_type(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return lsi_conn_get_px_type(hnd->con);
}

const char *
irc_get_pass(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->pass;
}

const char *
irc_get_uname(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->uname;
}

const char *
irc_get_fname(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->fname;
}

uint8_t
irc_get_conflags(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->conflags;
}

const char *
irc_get_nick(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->nick;
}

bool
irc_get_service_connect(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->serv_con;
}

const char *
irc_get_service_dist(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->serv_dist;
}

long
irc_get_service_type(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->serv_type;
}

const char *
irc_get_service_info(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->serv_info;
}

bool
irc_get_ssl(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return lsi_conn_get_ssl(hnd->con);
}


/* Setters - set library parameters (none of these takes effect before the
 * next call to irc_connect() is done */


bool
irc_set_server(irc hnd, const char *host, uint16_t port)
{ T("hnd=%p, host='%s' port=%"PRIu16, (void *)hnd, host, port);
	return lsi_conn_set_server(hnd->con, host, port);
}

bool
irc_set_pass(irc hnd, const char *srvpass)
{ T("hnd=%p, srvpass='%s'", (void *)hnd, srvpass);
	return lsi_com_update_strprop(&hnd->pass, srvpass);
}

bool
irc_set_uname(irc hnd, const char *uname)
{ T("hnd=%p, uname='%s'", (void *)hnd, uname);
	return lsi_com_update_strprop(&hnd->uname, uname);
}

bool
irc_set_fname(irc hnd, const char *fname)
{ T("hnd=%p, fname='%s'", (void *)hnd, fname);
	return lsi_com_update_strprop(&hnd->fname, fname);
}

bool
irc_set_nick(irc hnd, const char *nick)
{ T("hnd=%p, nick='%s'", (void *)hnd, nick);
	return lsi_com_update_strprop(&hnd->nick, nick);
}

bool
irc_set_px(irc hnd, const char *host, uint16_t port, int ptype)
{ T("hnd=%p, host='%s', port=%"PRIu16", ptype=%d", (void *)hnd, host, port, ptype);
	return lsi_conn_set_px(hnd->con, host, port, ptype);
}

void
irc_set_conflags(irc hnd, uint8_t flags)
{ T("hnd=%p, flags=0x%"PRIx8, (void *)hnd, flags);
	hnd->conflags = flags;
}

void
irc_set_service_connect(irc hnd, bool enabled)
{ T("hnd=%p, enabled=%d", (void *)hnd, enabled);
	hnd->serv_con = enabled;
}

bool
irc_set_service_dist(irc hnd, const char *dist)
{ T("hnd=%p, dist='%s'", (void *)hnd, dist);
	return lsi_com_update_strprop(&hnd->serv_dist, dist);
}

bool
irc_set_service_type(irc hnd, long type)
{ T("hnd=%p, type=%ld", (void *)hnd, type);
	hnd->serv_type = type;
	return true;
}

bool
irc_set_service_info(irc hnd, const char *info)
{ T("hnd=%p, info='%s'", (void *)hnd, info);
	return lsi_com_update_strprop(&hnd->serv_info, info);
}

void
irc_set_track(irc hnd, bool on)
{ T("hnd=%p, on=%d", (void *)hnd, on);
	hnd->tracking = on;
}

void
irc_set_connect_timeout(irc hnd, uint64_t soft, uint64_t hard)
{ T("hnd=%p, soft=%"PRIu64", hard=%"PRIu64, (void *)hnd, soft, hard);
	hnd->hcto_us = hard;
	hnd->scto_us = soft;
}

bool
irc_set_ssl(irc hnd, bool on)
{ T("hnd=%p, on=%d", (void *)hnd, on);
	return lsi_conn_set_ssl(hnd->con, on);
}

bool
irc_get_dumb(irc hnd)
{ T("hnd=%p", (void *)hnd);
	return hnd->dumb;
}

void
irc_set_dumb(irc hnd, bool dumbmode)
{ T("hnd=%p, dumbmode=%d", (void *)hnd, dumbmode);
	hnd->dumb = dumbmode;
}

bool
irc_reg_msghnd(irc hnd, const char *cmd, uhnd_fn hndfn, bool pre)
{ T("hnd=%p, cmd='%s', hndfn=(fp), pre=%d", (void *)hnd, cmd, pre);
	return lsi_msg_reguhnd(hnd, cmd, hndfn, pre);
}
