/*
 * Music messaging plugin for Gaim
 *
 * Copyright (C) 2005 Christian Muise.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "internal.h"
#include "gtkgaim.h"

#include "conversation.h"

#include "gtkconv.h"
#include "gtkplugin.h"
#include "gtkutils.h"

#include "notify.h"
#include "version.h"
#include "debug.h"

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include "dbus-maybe.h"
#include "dbus-bindings.h"
#include "dbus-server.h"
#include "dbus-gaim.h"

#define MUSICMESSAGING_PLUGIN_ID "gtk-hazure-musicmessaging"
#define MUSICMESSAGING_PREFIX "##MM##"
#define MUSICMESSAGING_START_MSG "A music messaging session has been requested. Please click the MM icon to accept."
#define MUSICMESSAGING_CONFIRM_MSG "Music messaging session confirmed."

typedef struct {
	GaimConversation *conv; /* pointer to the conversation */
	GtkWidget *seperator; /* seperator in the conversation */
	GtkWidget *button; /* button in the conversation */
	GPid pid; /* the pid of the score editor */
	
	gboolean started; /* session has started and editor run */
	gboolean originator; /* started the mm session */
	gboolean requested; /* received a request to start a session */
	
} MMConversation;

static gboolean start_session(MMConversation *mmconv);
static void run_editor(MMConversation *mmconv);
static void kill_editor(MMConversation *mmconv);
static void add_button (MMConversation *mmconv);
static void remove_widget (GtkWidget *button);
static void init_conversation (GaimConversation *conv);
static void conv_destroyed(GaimConversation *conv);
static gboolean intercept_sent(GaimAccount *account, GaimConversation *conv, char **message, void* pData);
static gboolean intercept_received(GaimAccount *account, char **sender, char **message, GaimConversation *conv, int *flags);
static gboolean send_change_request (const int session, const char *id, const char *command, const char *parameters);
static gboolean send_change_confirmed (const int session, const char *command, const char *parameters);
static void session_end (MMConversation *mmconv);

/* Globals */
/* List of sessions */
GList *conversations;

/* Pointer to this plugin */
GaimPlugin *plugin_pointer;

/* Define types needed for DBus */
DBusGConnection *connection;
DBusGProxy *proxy;
#define DBUS_SERVICE_GSCORE "org.gscore.GScoreService"
#define DBUS_PATH_GSCORE "/org/gscore/GScoreObject"
#define DBUS_INTERFACE_GSCORE "org.gscore.GScoreInterface"

/* Define the functions to export for use with DBus */
DBUS_EXPORT void music_messaging_change_request (const int session, const char *command, const char *parameters);
DBUS_EXPORT void music_messaging_change_confirmed (const int session, const char *command, const char *parameters);
DBUS_EXPORT void music_messaging_change_failed (const int session, const char *id, const char *command, const char *parameters);
DBUS_EXPORT void music_messaging_done_session (const int session);

/* This file has been generated by the #dbus-analize-functions.py
   script.  It contains dbus wrappers for the four functions declared
   above. */
#include "music-messaging-bindings.c"

/* Exported functions */
void music_messaging_change_request(const int session, const char *command, const char *parameters)
{
	
	MMConversation *mmconv = (MMConversation *)g_list_nth_data(conversations, session);
	
	if (mmconv->started)
	{
		if (mmconv->originator)
		{
			char *name = (mmconv->conv)->name;
			send_change_request (session, name, command, parameters);
		} else
		{
			GString *to_send = g_string_new("");
			g_string_append_printf(to_send, "##MM## request %s %s##MM##", command, parameters);
			
			gaim_conv_im_send(GAIM_CONV_IM(mmconv->conv), to_send->str);
			
			gaim_debug_misc("Sent request: %s\n", to_send->str);
		}
	}
			
}

void music_messaging_change_confirmed(const int session, const char *command, const char *parameters)
{
	
	MMConversation *mmconv = (MMConversation *)g_list_nth_data(conversations, session);
	
	if (mmconv->started)
	{
		if (mmconv->originator)
		{
			GString *to_send = g_string_new("");
			g_string_append_printf(to_send, "##MM## confirm %s %s##MM##", command, parameters);
			
			gaim_conv_im_send(GAIM_CONV_IM(mmconv->conv), to_send->str);
		} else
		{
			/* Do nothing. If they aren't the originator, then they can't confirm. */
		}
	}
	
}

void music_messaging_change_failed(const int session, const char *id, const char *command, const char *parameters)
{
	gaim_notify_message(plugin_pointer, GAIM_NOTIFY_MSG_INFO, command,
                        parameters, NULL, NULL, NULL);
	
	MMConversation *mmconv = (MMConversation *)g_list_nth_data(conversations, session);
	
	if (mmconv->started)
	{
		if (mmconv->originator)
		{
			GString *to_send = g_string_new("");
			g_string_append_printf(to_send, "##MM## failed %s %s %s##MM##", id, command, parameters);
			
			gaim_conv_im_send(GAIM_CONV_IM(mmconv->conv), to_send->str);
		} else
		{
			/* Do nothing. If they aren't the originator, then they can't confirm. */
		}
	}
}

void music_messaging_done_session(const int session)
{
	gaim_notify_message(plugin_pointer, GAIM_NOTIFY_MSG_INFO, "Session",
						"Session Complete", NULL, NULL, NULL);
	
	MMConversation *mmconv = (MMConversation *)g_list_nth_data(conversations, session);
	
	session_end(mmconv);
}


/* DBus commands that can be sent to the editor */
G_BEGIN_DECLS
DBusConnection *gaim_dbus_get_connection(void);
G_END_DECLS

static gboolean send_change_request (const int session, const char *id, const char *command, const char *parameters)
{
	DBusMessage *message;
	
	/* Create the signal we need */
	message = dbus_message_new_signal (DBUS_PATH_GAIM, DBUS_INTERFACE_GAIM, "GscoreChangeRequest");
	
	/* Append the string "Ping!" to the signal */
	dbus_message_append_args (message,
							DBUS_TYPE_INT32, &session,
							DBUS_TYPE_STRING, &id,
							DBUS_TYPE_STRING, &command,
							DBUS_TYPE_STRING, &parameters,
							DBUS_TYPE_INVALID);
	
	/* Send the signal */
	dbus_connection_send (gaim_dbus_get_connection(), message, NULL);
	
	/* Free the signal now we have finished with it */
	dbus_message_unref (message);
	
	/* Tell the user we sent a signal */
	g_printerr("Sent change request signal: %d %s %s %s\n", session, id, command, parameters);
	
	return TRUE;
}

static gboolean send_change_confirmed (const int session, const char *command, const char *parameters)
{
	DBusMessage *message;
	
	/* Create the signal we need */
	message = dbus_message_new_signal (DBUS_PATH_GAIM, DBUS_INTERFACE_GAIM, "GscoreChangeConfirmed");
	
	/* Append the string "Ping!" to the signal */
	dbus_message_append_args (message,
							DBUS_TYPE_INT32, &session,
							DBUS_TYPE_STRING, &command,
							DBUS_TYPE_STRING, &parameters,
							DBUS_TYPE_INVALID);
	
	/* Send the signal */
	dbus_connection_send (gaim_dbus_get_connection(), message, NULL);
	
	/* Free the signal now we have finished with it */
	dbus_message_unref (message);
	
	/* Tell the user we sent a signal */
	g_printerr("Sent change confirmed signal.\n");
	
	return TRUE;
}


static int
mmconv_from_conv_loc(GaimConversation *conv)
{
	MMConversation *mmconv_current = NULL;
	guint i;
	
	for (i = 0; i < g_list_length(conversations); i++)
	{
		mmconv_current = (MMConversation *)g_list_nth_data(conversations, i);
		if (conv == mmconv_current->conv)
		{
			return i;
		}
	}
	return -1;
}

static MMConversation*
mmconv_from_conv(GaimConversation *conv)
{
	return (MMConversation *)g_list_nth_data(conversations, mmconv_from_conv_loc(conv));
}

static gboolean
plugin_load(GaimPlugin *plugin) {
    /* First, we have to register our four exported functions with the
       main gaim dbus loop.  Without this statement, the gaim dbus
       code wouldn't know about our functions. */
    GAIM_DBUS_REGISTER_BINDINGS(plugin);
	
	
	gaim_notify_message(plugin, GAIM_NOTIFY_MSG_INFO, "Welcome",
                        "Welcome to music messaging.", NULL, NULL, NULL);
	/* Keep the plugin for reference (needed for notify's) */
	plugin_pointer = plugin;
	
	/* Add the button to all the current conversations */
	gaim_conversation_foreach (init_conversation);
	
	/* Listen for any new conversations */
	void *conv_list_handle = gaim_conversations_get_handle();
	
	gaim_signal_connect(conv_list_handle, "conversation-created", 
					plugin, GAIM_CALLBACK(init_conversation), NULL);
	
	/* Listen for conversations that are ending */
	gaim_signal_connect(conv_list_handle, "deleting-conversation",
					plugin, GAIM_CALLBACK(conv_destroyed), NULL);
					
	/* Listen for sending/receiving messages to replace tags */
	gaim_signal_connect(conv_list_handle, "displaying-im-msg",
					plugin, GAIM_CALLBACK(intercept_sent), NULL);
	gaim_signal_connect(conv_list_handle, "receiving-im-msg",
					plugin, GAIM_CALLBACK(intercept_received), NULL);
	
	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin) {
	
	gaim_notify_message(plugin, GAIM_NOTIFY_MSG_INFO, "Unloaded",
						DATADIR, NULL, NULL, NULL);
	
	MMConversation *mmconv = NULL;
	while (g_list_length(conversations) > 0)
	{
		mmconv = g_list_first(conversations)->data;
		conv_destroyed(mmconv->conv);
	}
	return TRUE;
}



static gboolean
intercept_sent(GaimAccount *account, GaimConversation *conv, char **message, void* pData)
{
	
	if (0 == strncmp(*message, MUSICMESSAGING_PREFIX, strlen(MUSICMESSAGING_PREFIX)))
	{
		gaim_debug_misc("gaim-musicmessaging", "Sent MM Message: %s\n", *message);
		message = 0;
	}
	else if (0 == strncmp(*message, MUSICMESSAGING_START_MSG, strlen(MUSICMESSAGING_START_MSG)))
	{
		gaim_debug_misc("gaim-musicmessaging", "Sent MM request.\n");
		return FALSE;
	}
	else if (0 == strncmp(*message, MUSICMESSAGING_CONFIRM_MSG, strlen(MUSICMESSAGING_CONFIRM_MSG)))
	{
		gaim_debug_misc("gaim-musicmessaging", "Sent MM confirm.\n");
		return FALSE;
	}
	else if (0 == strncmp(*message, "test1", strlen("test1")))
	{
		gaim_debug_misc("gaim-musicmessaging", "\n\nTEST 1\n\n");
		send_change_request(0, "test-id", "test-command", "test-parameters");
		return FALSE;
	}
	else if (0 == strncmp(*message, "test2", strlen("test2")))
	{
		gaim_debug_misc("gaim-musicmessaging", "\n\nTEST 2\n\n");
		send_change_confirmed(1, "test-command", "test-parameters");
		return FALSE;
	}
	else
	{
		return FALSE;
		/* Do nothing...procceed as normal */
	}
	return TRUE;
}

static gboolean
intercept_received(GaimAccount *account, char **sender, char **message, GaimConversation *conv, int *flags)
{
	MMConversation *mmconv = mmconv_from_conv(conv);
	
	gaim_debug_misc("gaim-musicmessaging", "Intercepted: %s\n", *message);
	if (strstr(*message, MUSICMESSAGING_PREFIX))
	{
		char *parsed_message = strtok(strstr(*message, MUSICMESSAGING_PREFIX), "<");
		gaim_debug_misc("gaim-musicmessaging", "Received an MM Message: %s\n", parsed_message);
				
		if (mmconv->started)
		{
			if (strstr(parsed_message, "request"))
			{
				if (mmconv->originator)
				{
					gaim_debug_misc("gaim-musicmessaging", "Sending request to gscore.\n");
					
					int session = mmconv_from_conv_loc(conv);
					char *id = (mmconv->conv)->name;
					
					/* Get past the first two terms - '##MM##' and 'request' */
					strtok(parsed_message, " "); /* '##MM##' */
					strtok(NULL, " "); /* 'request' */
					
					char *command = strtok(NULL, " ");
					char *parameters = strtok(NULL, "#");
					
					send_change_request (session, id, command, parameters);
					
				}
			} else if (strstr(parsed_message, "confirm"))
			{
				if (!mmconv->originator)
				{
					gaim_debug_misc("gaim-musicmessaging", "Sending confirmation to gscore.\n");
					
					int session = mmconv_from_conv_loc(conv);
					
					/* Get past the first two terms - '##MM##' and 'confirm' */
					strtok(parsed_message, " "); /* '##MM##' */
					strtok(NULL, " "); /* 'confirm' */
					
					char *command = strtok(NULL, " ");
					char *parameters = strtok(NULL, "#");
					
					send_change_confirmed (session, command, parameters);
				}
			} else if (strstr(parsed_message, "failed"))
			{
				/* Get past the first two terms - '##MM##' and 'confirm' */
				strtok(parsed_message, " "); /* '##MM##' */
				strtok(NULL, " "); /* 'failed' */
				
				char *id = strtok(NULL, " ");
				char *command = strtok(NULL, " ");
				/* char *parameters = strtok(NULL, "#"); DONT NEED PARAMETERS */
				
				if ((mmconv->conv)->name == id)
				{
					gaim_notify_message(plugin_pointer, GAIM_NOTIFY_MSG_ERROR, "Music Messaging",
						"There was a conflict in running the command:", command, NULL, NULL);
				}
			}
		}
		
		message = 0;
	}
	else if (strstr(*message, MUSICMESSAGING_START_MSG))
	{
		gaim_debug_misc("gaim-musicmessaging", "Received MM request.\n");
		if (!(mmconv->originator))
		{
			mmconv->requested = TRUE;
			return FALSE;
		}
		
	}
	else if (strstr(*message, MUSICMESSAGING_CONFIRM_MSG))
	{
		gaim_debug_misc("gaim-musicmessagin", "Received MM confirm.\n");
		
		if (mmconv->originator)
		{
			start_session(mmconv);
			return FALSE;
		}
	}
	else
	{
		return FALSE;
		/* Do nothing. */
	}
	return TRUE;
}

static void send_request(MMConversation *mmconv)
{
	GaimConnection *connection = gaim_conversation_get_gc(mmconv->conv);
	const char *convName = gaim_conversation_get_name(mmconv->conv);
	serv_send_im(connection, convName, MUSICMESSAGING_START_MSG, GAIM_MESSAGE_SEND);
}

static void send_request_confirmed(MMConversation *mmconv)
{
	GaimConnection *connection = gaim_conversation_get_gc(mmconv->conv);
	const char *convName = gaim_conversation_get_name(mmconv->conv);
	serv_send_im(connection, convName, MUSICMESSAGING_CONFIRM_MSG, GAIM_MESSAGE_SEND);
}
	

static gboolean
start_session(MMConversation *mmconv)
{	
	run_editor(mmconv);
	return TRUE;
}

static void session_end (MMConversation *mmconv)
{
	mmconv->started = FALSE;
	mmconv->originator = FALSE;
	mmconv->requested = FALSE;
	kill_editor(mmconv);
}

static void music_button_toggled (GtkWidget *widget, gpointer data)
{
	MMConversation *mmconv = mmconv_from_conv(((MMConversation *) data)->conv);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) 
    {
		if (((MMConversation *) data)->requested)
		{
			start_session(mmconv);
			send_request_confirmed(mmconv);
		}
		else
		{
			((MMConversation *) data)->originator = TRUE;
			send_request((MMConversation *) data);
		}
    } else {
		session_end((MMConversation *)data);
    }
}

static void set_editor_path (GtkWidget *button, GtkWidget *text_field)
{
	const char * path = gtk_entry_get_text((GtkEntry*)text_field);
	gaim_prefs_set_string("/plugins/gtk/musicmessaging/editor_path", path);
	
}

static void run_editor (MMConversation *mmconv)
{
	GError *spawn_error = NULL;
	gchar * args[4];
	args[0] = (gchar *)gaim_prefs_get_string("/plugins/gtk/musicmessaging/editor_path");
	
	args[1] = "-session_id";
	GString *session_id = g_string_new("");
	g_string_sprintfa(session_id, "%d", mmconv_from_conv_loc(mmconv->conv));
	args[2] = session_id->str;
	
	args[3] = NULL;
	
	if (!(g_spawn_async (".", args, NULL, 4, NULL, NULL, &(mmconv->pid), &spawn_error)))
	{
		gaim_notify_error(plugin_pointer, "Error Running Editor",
						"The following error has occured:", spawn_error->message);
		mmconv->started = FALSE;
	}
	else
	{
		mmconv->started = TRUE;
	}
}

static void kill_editor (MMConversation *mmconv)
{
	if (mmconv->pid)
	{
		kill(mmconv->pid, SIGINT);
		mmconv->pid = 0;
	}
}

static void init_conversation (GaimConversation *conv)
{
	MMConversation *mmconv;
	mmconv = g_malloc(sizeof(MMConversation));
	
	mmconv->conv = conv;
	mmconv->started = FALSE;
	mmconv->originator = FALSE;
	mmconv->requested = FALSE;
	
	add_button(mmconv);
	
	conversations = g_list_append(conversations, mmconv);
}

static void conv_destroyed (GaimConversation *conv)
{
	MMConversation *mmconv = mmconv_from_conv(conv);
	
	remove_widget(mmconv->button);
	remove_widget(mmconv->seperator);
	if (mmconv->started)
	{
		kill_editor(mmconv);
	}
	conversations = g_list_remove(conversations, mmconv);
}

static void add_button (MMConversation *mmconv)
{
	GaimConversation *conv = mmconv->conv;
	
	GtkWidget *button, *image, *sep;

	button = gtk_toggle_button_new();
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(music_button_toggled), mmconv);

	/* gchar *file_path = g_build_filename (DATADIR, "pixmaps", "gaim", "buttons", "music.png", NULL); */
	gchar *file_path = "/usr/local/share/pixmaps/gaim/buttons/music.png";
	image = gtk_image_new_from_file(file_path);

	gtk_container_add((GtkContainer *)button, image);
	
	sep = gtk_vseparator_new();
	
	mmconv->seperator = sep;
	mmconv->button = button;
	
	gtk_widget_show(sep);
	gtk_widget_show(image);
	gtk_widget_show(button);
	
	gtk_box_pack_start(GTK_BOX(GAIM_GTK_CONVERSATION(conv)->toolbar), sep, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GAIM_GTK_CONVERSATION(conv)->toolbar), button, FALSE, FALSE, 0);
}

static void remove_widget (GtkWidget *button)
{
	gtk_widget_hide(button);
	gtk_widget_destroy(button);		
}

static GtkWidget *
get_config_frame(GaimPlugin *plugin)
{
	GtkWidget *ret;
	GtkWidget *vbox;
	
	GtkWidget *editor_path;
	GtkWidget *editor_path_label;
	GtkWidget *editor_path_button;
	
	/* Outside container */
	ret = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width(GTK_CONTAINER(ret), 10);

	/* Configuration frame */
	vbox = gaim_gtk_make_frame(ret, _("Music Messaging Configuration"));
	
	/* Path to the score editor */
	editor_path = gtk_entry_new();
	editor_path_label = gtk_label_new("Score Editor Path");
	editor_path_button = gtk_button_new_with_mnemonic(_("_Apply"));
	
	gtk_entry_set_text((GtkEntry*)editor_path, "/usr/local/bin/gscore");
	
	g_signal_connect(G_OBJECT(editor_path_button), "clicked",
					 G_CALLBACK(set_editor_path), editor_path);
					 
	gtk_box_pack_start(GTK_BOX(vbox), editor_path_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), editor_path, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), editor_path_button, FALSE, FALSE, 0);
	
	gtk_widget_show_all(ret);

	return ret;
}

static GaimGtkPluginUiInfo ui_info =
{
	get_config_frame
};

static GaimPluginInfo info = {
    GAIM_PLUGIN_MAGIC,
    GAIM_MAJOR_VERSION,
    GAIM_MINOR_VERSION,
    GAIM_PLUGIN_STANDARD,
    GAIM_GTK_PLUGIN_TYPE,
    0,
    NULL,
    GAIM_PRIORITY_DEFAULT,

    MUSICMESSAGING_PLUGIN_ID,
    "Music Messaging",
    VERSION,
    "Music Messaging Plugin for collabrative composition.",
    "The Music Messaging Plugin allows a number of users to simultaneously work on a piece of music by editting a common score in real-time.",
    "Christian Muise <christian.muise@gmail.com>",
    GAIM_WEBSITE,
    plugin_load,
    plugin_unload,
    NULL,
    &ui_info,
    NULL,
    NULL,
    NULL
};

static void
init_plugin(GaimPlugin *plugin) {
	gaim_prefs_add_none("/plugins/gtk/musicmessaging");
	gaim_prefs_add_string("/plugins/gtk/musicmessaging/editor_path", "/usr/local/bin/gscore");
}

GAIM_INIT_PLUGIN(musicmessaging, init_plugin, info);
