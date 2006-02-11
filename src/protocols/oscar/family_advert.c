/*
 * Gaim's oscar protocol plugin
 * This file is the legal property of its developers.
 * Please see the AUTHORS file distributed alongside this file.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Family 0x0005 - Advertisements.
 *
 */

#include "oscar.h"

faim_export int aim_ads_requestads(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n(sess, conn, 0x0005, 0x0002);
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	return 0;
}

faim_internal int adverts_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0005;
	mod->version = 0x0001;
	mod->toolid = 0x0001;
	mod->toolversion = 0x0001;
	mod->flags = 0;
	strncpy(mod->name, "advert", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}
