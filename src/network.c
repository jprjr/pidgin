/**
 * @file network.c Network Implementation
 * @ingroup core
 *
 * gaim
 *
 * Gaim is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "internal.h"

#include "debug.h"
#include "account.h"
#include "network.h"
#include "prefs.h"

void
gaim_network_set_local_ip(const char *ip)
{
	g_return_if_fail(ip != NULL);

	gaim_prefs_set_string("/core/network/public_ip", ip);
}

const char *
gaim_network_get_local_ip(void)
{
	const char *ip;

	if (gaim_prefs_get_bool("/core/network/auto_ip"))
		return NULL;

	ip = gaim_prefs_get_string("/core/network/public_ip");

	if (ip == NULL || *ip == '\0')
		return NULL;

	return ip;
}

static const char *
gaim_network_get_local_ip_from_fd(int fd)
{
	struct sockaddr_in addr;
	socklen_t len;
	static char ip[16];
	const char *tmp;

	g_return_val_if_fail(fd > 0, NULL);

	len = sizeof(addr);
	if (getsockname(fd, (struct sockaddr *) &addr, &len) == -1) {
		gaim_debug_warning("network", "getsockname: %s\n", strerror(errno));
		return NULL;
	}

	tmp = inet_ntoa(addr.sin_addr);
	strncpy(ip, tmp, sizeof(ip));
	return ip;
}

const char *
gaim_network_get_local_system_ip(int fd)
{
	struct hostent *host;
	char localhost[129];
	long unsigned add;
	static char ip[46];
	const char *tmp = NULL;

	if (fd != -1)
		tmp = gaim_network_get_local_ip_from_fd(fd);

	if (tmp)
		return tmp;

	if (gethostname(localhost, 128) < 0)
		return NULL;

	if ((host = gethostbyname(localhost)) == NULL)
		return NULL;

	memcpy(&add, host->h_addr_list[0], 4);
	add = htonl(add);

	g_snprintf(ip, 16, "%lu.%lu.%lu.%lu",
			   ((add >> 24) & 255),
			   ((add >> 16) & 255),
			   ((add >>  8) & 255),
			   add & 255);

	return ip;
}

const char *
gaim_network_get_ip_for_account(const GaimAccount *account, int fd)
{
	if (account && (gaim_account_get_public_ip(account) != NULL))
		return gaim_account_get_public_ip(account);
	else if (gaim_network_get_local_ip() != NULL)
		return gaim_network_get_local_ip();
	else
		return gaim_network_get_local_system_ip(fd);
}

static int gaim_network_do_listen(unsigned short port)
{
#if HAVE_GETADDRINFO
	int listenfd;
	const int on = 1;
	struct addrinfo hints, *res, *ressave;
	char serv[5];

	snprintf(serv, sizeof(serv), "%d", port);
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(NULL /* any IP */, serv, &hints, &res) != 0) {
		gaim_debug_warning("network", "getaddrinfo: %s\n", strerror(errno));
		return -1;
	}
	ressave = res;
	do {
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0)
			continue;
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
			break; /* success */
		close(listenfd);
	} while ( (res = res->ai_next) );

	if (!res)
		return -1;

	freeaddrinfo(ressave);
#else
	int listenfd;
	const int on = 1;
	struct sockaddr_in sockin;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		gaim_debug_warning("network", "socket: %s\n", strerror(errno));
		return -1;
	}

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) != 0) {
		gaim_debug_warning("network", "setsockopt: %s\n", strerror(errno));
		close(listenfd);
		return -1;
	}

	memset(&sockin, 0, sizeof(struct sockaddr_in));
	sockin.sin_family = AF_INET;
	sockin.sin_port = htons(port);

	if (bind(listenfd, (struct sockaddr *)&sockin, sizeof(struct sockaddr_in)) != 0) {
		gaim_debug_warning("network", "bind: %s\n", strerror(errno));
		close(listenfd);
		return -1;
	}
#endif

	if (listen(listenfd, 4) != 0) {
		gaim_debug_warning("network", "listen: %s\n", strerror(errno));
		close(listenfd);
		return -1;
	}
	fcntl(listenfd, F_SETFL, O_NONBLOCK);

	gaim_debug_info("network", "Listening on port: %hu\n", gaim_network_get_port_from_fd(listenfd));
	return listenfd;
}

int gaim_network_listen(unsigned short port)
{
	g_return_val_if_fail(port != 0, -1);

	return gaim_network_do_listen(port);
}

int gaim_network_listen_range(unsigned short start, unsigned short end)
{
	int ret = -1;

	if (gaim_prefs_get_bool("/core/network/ports_range_use")) {
		start = gaim_prefs_get_int("/core/network/ports_range_start");
		end = gaim_prefs_get_int("/core/network/ports_range_end");
	} else {
		if (end < start)
			end = start;
	}

	for (; start <= end; start++) {
		ret = gaim_network_do_listen(start);
		if (ret >= 0)
			break;
	}

	return ret;
}

short gaim_network_get_port_from_fd(int fd)
{
	struct sockaddr_in addr;
	socklen_t len;

	g_return_val_if_fail(fd > 0, 0);

	len = sizeof(addr);
	if (getsockname(fd, (struct sockaddr *) &addr, &len) == -1) {
		gaim_debug_warning("network", "getsockname: %s\n", strerror(errno));
		return 0;
	}

	return ntohs(addr.sin_port);
}

void
gaim_network_init(void)
{
	gaim_prefs_add_none  ("/core/network");
	gaim_prefs_add_bool  ("/core/network/auto_ip", TRUE);
	gaim_prefs_add_string("/core/network/public_ip", "");
	gaim_prefs_add_bool  ("/core/network/ports_range_use", FALSE);
	gaim_prefs_add_int   ("/core/network/ports_range_start", 1024);
	gaim_prefs_add_int   ("/core/network/ports_range_end", 2048);
}
