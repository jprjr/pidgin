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
 *
 */

static GHashTable *
yahoo_get_account_text_table(PurpleAccount *account)
{
	GHashTable *table;
	table = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_insert(table, "login_label", (gpointer)_("Yahoo ID..."));
	return table;
}

static PurpleWhiteboardPrplOps yahoo_whiteboard_prpl_ops =
{
	yahoo_doodle_start,
	yahoo_doodle_end,
	yahoo_doodle_get_dimensions,
	NULL,
	yahoo_doodle_get_brush,
	yahoo_doodle_set_brush,
	yahoo_doodle_send_draw_list,
	yahoo_doodle_clear,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginProtocolInfo prpl_info =
{
	OPT_PROTO_MAIL_CHECK | OPT_PROTO_CHAT_TOPIC,
	NULL, /* user_splits */
	NULL, /* protocol_options */
	{"png,gif,jpeg", 96, 96, 96, 96, 0, PURPLE_ICON_SCALE_SEND},
	yahoo_list_icon,
	yahoo_list_emblem,
	yahoo_status_text,
	yahoo_tooltip_text,
	yahoo_status_types,
	yahoo_blist_node_menu,
	yahoo_c_info,
	yahoo_c_info_defaults,
	yahoo_login,
	yahoo_close,
	yahoo_send_im,
	NULL, /* set info */
	yahoo_send_typing,
	yahoo_get_info,
	yahoo_set_status,
	yahoo_set_idle,
	NULL, /* change_passwd*/
	yahoo_add_buddy,
	NULL, /* add_buddies */
	yahoo_remove_buddy,
	NULL, /* remove_buddies */
	NULL, /* add_permit */
	yahoo_add_deny,
	NULL, /* rem_permit */
	yahoo_rem_deny,
	yahoo_set_permit_deny,
	yahoo_c_join,
	NULL, /* reject chat invite */
	yahoo_get_chat_name,
	yahoo_c_invite,
	yahoo_c_leave,
	NULL, /* chat whisper */
	yahoo_c_send,
	yahoo_keepalive,
	NULL, /* register_user */
	NULL, /* get_cb_info */
	NULL, /* get_cb_away */
	yahoo_update_alias, /* alias_buddy */
	yahoo_change_buddys_group,
	yahoo_rename_group,
	NULL, /* buddy_free */
	NULL, /* convo_closed */
	purple_normalize_nocase, /* normalize */
	yahoo_set_buddy_icon,
	NULL, /* void (*remove_group)(PurpleConnection *gc, const char *group);*/
	NULL, /* char *(*get_cb_real_name)(PurpleConnection *gc, int id, const char *who); */
	NULL, /* set_chat_topic */
	NULL, /* find_blist_chat */
	yahoo_roomlist_get_list,
	yahoo_roomlist_cancel,
	yahoo_roomlist_expand_category,
	NULL, /* can_receive_file */
	yahoo_send_file,
	yahoo_new_xfer,
	yahoo_offline_message, /* offline_message */
	&yahoo_whiteboard_prpl_ops,
	NULL, /* send_raw */
	NULL, /* roomlist_room_serialize */
	NULL, /* unregister_user */

	yahoo_send_attention,
	yahoo_attention_types,

	sizeof(PurplePluginProtocolInfo),       /* struct_size */
	yahoo_get_account_text_table,    /* get_account_text_table */
	NULL, /* initiate_media */
	NULL  /* can_do_media */
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                            /**< priority       */
	"prpl-yahoo",                                     /**< id             */
	"Yahoo",	                                      /**< name           */
	DISPLAY_VERSION,                                  /**< version        */
	                                                  /**  summary        */
	N_("Yahoo Protocol Plugin"),
	                                                  /**  description    */
	N_("Yahoo Protocol Plugin"),
	NULL,                                             /**< author         */
	PURPLE_WEBSITE,                                     /**< homepage       */
	NULL,                                             /**< load           */
	yahoo_unload_plugin,                              /**< unload         */
	NULL,                                             /**< destroy        */
	NULL,                                             /**< ui_info        */
	&prpl_info,                                       /**< extra_info     */
	NULL,
	yahoo_actions,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
	PurpleAccountOption *option;

	option = purple_account_option_string_new(_("Pager server"), "server", YAHOO_PAGER_HOST);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("Pager port"), "port", YAHOO_PAGER_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new(_("File transfer server"), "xfer_host", YAHOO_XFER_HOST);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("File transfer port"), "xfer_port", YAHOO_XFER_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new(_("Chat room locale"), "room_list_locale", YAHOO_ROOMLIST_LOCALE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Ignore conference and chatroom invitations"), "ignore_invites", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new(_("Encoding"), "local_charset", "UTF-8");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);


#if 0
	option = purple_account_option_string_new(_("Chat room list URL"), "room_list", YAHOO_ROOMLIST_URL);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new(_("Yahoo Chat server"), "ycht-server", YAHOO_YCHT_HOST);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("Yahoo Chat port"), "ycht-port", YAHOO_YCHT_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);
#endif

	my_protocol = plugin;
	yahoopurple_register_commands();
	yahoo_init_colorht();

	purple_signal_connect(purple_get_core(), "uri-handler", plugin,
		PURPLE_CALLBACK(yahoo_uri_handler), NULL);
}

PURPLE_INIT_PLUGIN(yahoo, init_plugin, info);
