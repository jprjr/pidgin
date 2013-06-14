/*
 * purple
 *
 * Purple is the legal property of its developers, whose names are too numerous
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */
#include "sha1.h"

/*******************************************************************************
 * Structs
 ******************************************************************************/
#define PURPLE_SHA1_CIPHER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), PURPLE_TYPE_SHA1_CIPHER, PurpleSHA1CipherPrivate))

typedef struct {
  GChecksum *checksum;
} PurpleSHA1CipherPrivate;

/******************************************************************************
 * Globals
 *****************************************************************************/
static GObjectClass *parent_class = NULL;

/******************************************************************************
 * Cipher Stuff
 *****************************************************************************/

static void
purple_sha1_cipher_reset(PurpleCipher *cipher)
{
	PurpleSHA1Cipher *sha1_cipher = PURPLE_SHA1_CIPHER(cipher);
	PurpleSHA1CipherPrivate *priv = PURPLE_SHA1_CIPHER_GET_PRIVATE(sha1_cipher);

	g_return_if_fail(priv != NULL);
	g_return_if_fail(priv->checksum != NULL);

	g_checksum_reset(priv->checksum);
}

static void
purple_sha1_cipher_append(PurpleCipher *cipher, const guchar *data,
								gsize len)
{
	PurpleSHA1Cipher *sha1_cipher = PURPLE_SHA1_CIPHER(cipher);
	PurpleSHA1CipherPrivate *priv = PURPLE_SHA1_CIPHER_GET_PRIVATE(sha1_cipher);

	g_return_if_fail(priv != NULL);
	g_return_if_fail(priv->checksum != NULL);

	while (len >= G_MAXSSIZE) {
		g_checksum_update(priv->checksum, data, G_MAXSSIZE);
		len -= G_MAXSSIZE;
		data += G_MAXSSIZE;
	}

	if (len)
		g_checksum_update(priv->checksum, data, len);
}

static gboolean
purple_sha1_cipher_digest(PurpleCipher *cipher, guchar *digest, size_t buff_len)
{
	PurpleSHA1Cipher *sha1_cipher = PURPLE_SHA1_CIPHER(cipher);
	PurpleSHA1CipherPrivate *priv = PURPLE_SHA1_CIPHER_GET_PRIVATE(sha1_cipher);

	const gssize required_len = g_checksum_type_get_length(G_CHECKSUM_SHA1);
	gsize digest_len = buff_len;

	g_return_val_if_fail(priv != NULL, FALSE);
	g_return_val_if_fail(priv->checksum != NULL, FALSE);
	g_return_val_if_fail(buff_len >= required_len, FALSE);

	g_checksum_get_digest(priv->checksum, digest, &digest_len);

	if (digest_len != required_len)
		return FALSE;

	purple_sha1_cipher_reset(cipher);

	return TRUE;
}

static size_t
purple_sha1_cipher_get_block_size(PurpleCipher *cipher)
{
	return 64;
}

static size_t
purple_sha1_cipher_get_digest_size(PurpleCipher *cipher)
{
	return g_checksum_type_get_length(G_CHECKSUM_SHA1);
}

static const gchar*
purple_sha1_cipher_get_name(PurpleCipher *cipher)
{
	return "sha1";
}

/******************************************************************************
 * Object Stuff
 *****************************************************************************/

static void
purple_sha1_cipher_finalize(GObject *obj)
{
	PurpleSHA1Cipher *sha1_cipher = PURPLE_SHA1_CIPHER(obj);
	PurpleSHA1CipherPrivate *priv = PURPLE_SHA1_CIPHER_GET_PRIVATE(sha1_cipher);

	if (priv->checksum)
		g_checksum_free(priv->checksum);

	parent_class->finalize(obj);
}

static void
purple_sha1_cipher_class_init(PurpleSHA1CipherClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	PurpleCipherClass *cipher_class = PURPLE_CIPHER_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);

	obj_class->finalize = purple_sha1_cipher_finalize;

	cipher_class->reset = purple_sha1_cipher_reset;
	cipher_class->reset_state = purple_sha1_cipher_reset;
	cipher_class->append = purple_sha1_cipher_append;
	cipher_class->digest = purple_sha1_cipher_digest;
	cipher_class->get_digest_size = purple_sha1_cipher_get_digest_size;
	cipher_class->get_block_size = purple_sha1_cipher_get_block_size;
	cipher_class->get_name = purple_sha1_cipher_get_name;

	g_type_class_add_private(klass, sizeof(PurpleSHA1CipherPrivate));
}

static void
purple_sha1_cipher_init(PurpleCipher *cipher)
{
	PurpleSHA1Cipher *sha1_cipher = PURPLE_SHA1_CIPHER(cipher);
	PurpleSHA1CipherPrivate *priv = PURPLE_SHA1_CIPHER_GET_PRIVATE(sha1_cipher);

	priv->checksum = g_checksum_new(G_CHECKSUM_SHA1);

	purple_sha1_cipher_reset(cipher);
}

/******************************************************************************
 * API
 *****************************************************************************/
GType
purple_sha1_cipher_get_gtype(void) {
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo info = {
			sizeof(PurpleSHA1CipherClass),
			NULL,
			NULL,
			(GClassInitFunc)purple_sha1_cipher_class_init,
			NULL,
			NULL,
			sizeof(PurpleSHA1Cipher),
			0,
			(GInstanceInitFunc)purple_sha1_cipher_init,
			NULL,
		};

		type = g_type_register_static(PURPLE_TYPE_CIPHER,
									  "PurpleSHA1Cipher",
									  &info, 0);
	}

	return type;
}

PurpleCipher *
purple_sha1_cipher_new(void) {
	return g_object_new(PURPLE_TYPE_SHA1_CIPHER, NULL);
}
