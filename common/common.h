/* common.h - Interface to routines of common use
 * libsrsirc - a lightweight serious IRC lib - (C) 2012, Timo Buhrmester
 * See README for contact-, COPYING for license information. */

#ifndef SRS_COMMON_H
#define SRS_COMMON_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/time.h>

#define XCALLOC(num) ic_xcalloc(1, num)
#define XMALLOC(num) ic_xmalloc(num)
#define XREALLOC(p, num) ic_xrealloc((p),(num))
#define XFREE(p) do{if(p) free(p); p=0;}while(0)
#define XSTRDUP(s) ic_xstrdup(s)

void ic_strNcat(char *dest, const char *src, size_t destsz);
char* ic_strNcpy(char *dst, const char *src, size_t len);
size_t ic_strCchr(const char *dst, char c);

void *ic_xcalloc(size_t num, size_t size);
void *ic_xmalloc(size_t num);
void *ic_xrealloc(void *p, size_t num);
char *ic_xstrdup(const char *string);

int64_t ic_timestamp_us();
void ic_tconv(struct timeval *tv, int64_t *ts, bool tv_to_ts);

int ic_consocket(const char *host, unsigned short port,
    struct sockaddr *sockaddr, size_t *addrlen,
    unsigned long softto, unsigned long hardto);
#endif /* SRS_COMMON_H */
