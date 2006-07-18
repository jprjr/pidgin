/*
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
 *
 */

#define DBUS_API_SUBJECT_TO_CHANGE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "account.h"
#include "blist.h"
#include "conversation.h"
#include "dbus-gaim.h"
#include "dbus-server.h"
#include "dbus-useful.h"
#include "dbus-bindings.h"
#include "debug.h"
#include "core.h"
#include "internal.h"
#include "savedstatuses.h"
#include "value.h"


/**************************************************************************/
/** @name Gaim DBUS pointer registration mechanism                        */
/**************************************************************************/

/*
 * Here we include the list of #GAIM_DBUS_DEFINE_TYPE statements for
 * all structs defined in gaim.  This file has been generated by the
 * #dbus-analize-types.py script.
 */

#include "dbus-types.c"

/*
 * The following three hashtables map are used to translate between
 * pointers (nodes) and the corresponding handles (ids).
 */

static GHashTable *map_node_id;
static GHashTable *map_id_node;
static GHashTable *map_id_type;

static gchar *init_error;

/**
 * This function initializes the pointer-id traslation system.  It
 * creates the three above hashtables and defines parents of some types.
 */
void
gaim_dbus_init_ids(void)
{
	map_id_node = g_hash_table_new(g_direct_hash, g_direct_equal);
	map_id_type = g_hash_table_new(g_direct_hash, g_direct_equal);
	map_node_id = g_hash_table_new(g_direct_hash, g_direct_equal);

	GAIM_DBUS_TYPE(GaimBuddy)->parent   = GAIM_DBUS_TYPE(GaimBlistNode);
	GAIM_DBUS_TYPE(GaimContact)->parent = GAIM_DBUS_TYPE(GaimBlistNode);
	GAIM_DBUS_TYPE(GaimChat)->parent    = GAIM_DBUS_TYPE(GaimBlistNode);
	GAIM_DBUS_TYPE(GaimGroup)->parent   = GAIM_DBUS_TYPE(GaimBlistNode);
}

void
gaim_dbus_register_pointer(gpointer node, GaimDBusType *type)
{
	static gint last_id = 0;

	g_return_if_fail(map_node_id);
	g_return_if_fail(g_hash_table_lookup(map_node_id, node) == NULL);

	last_id++;
	g_hash_table_insert(map_node_id, node, GINT_TO_POINTER(last_id));
	g_hash_table_insert(map_id_node, GINT_TO_POINTER(last_id), node);
	g_hash_table_insert(map_id_type, GINT_TO_POINTER(last_id), type);
}

void
gaim_dbus_unregister_pointer(gpointer node)
{
	gpointer id = g_hash_table_lookup(map_node_id, node);

	g_hash_table_remove(map_node_id, node);
	g_hash_table_remove(map_id_node, GINT_TO_POINTER(id));
	g_hash_table_remove(map_id_type, GINT_TO_POINTER(id));
}

gint
gaim_dbus_pointer_to_id(gpointer node)
{
	gint id = GPOINTER_TO_INT(g_hash_table_lookup(map_node_id, node));
	if ((id == 0) && (node != NULL))
	{
		gaim_debug_warning("dbus",
				"Need to register an object with the dbus subsystem.\n");
		g_return_val_if_reached(0);
	}
	return id;
}

gpointer
gaim_dbus_id_to_pointer(gint id, GaimDBusType *type)
{
	GaimDBusType *objtype;

	objtype = (GaimDBusType*)g_hash_table_lookup(map_id_type,
			GINT_TO_POINTER(id));

	while (objtype != type && objtype != NULL)
		objtype = objtype->parent;

	if (objtype == type)
		return g_hash_table_lookup(map_id_node, GINT_TO_POINTER(id));
	else
		return NULL;
}

gint
gaim_dbus_pointer_to_id_error(gpointer ptr, DBusError *error)
{
	gint id = gaim_dbus_pointer_to_id(ptr);

	if (ptr != NULL && id == 0)
		dbus_set_error(error, "net.sf.gaim.ObjectNotFound",
				"The return object is not mapped (this is a Gaim error)");

	return id;
}

gpointer
gaim_dbus_id_to_pointer_error(gint id, GaimDBusType *type,
		const char *typename, DBusError *error)
{
	gpointer ptr = gaim_dbus_id_to_pointer(id, type);

	if (ptr == NULL && id != 0)
		dbus_set_error(error, "net.sf.gaim.InvalidHandle",
				"%s object with ID = %i not found", typename, id);

	return ptr;
}


/**************************************************************************/
/** @name Modified versions of some DBus functions                        */
/**************************************************************************/

dbus_bool_t
gaim_dbus_message_get_args(DBusMessage *message,
		DBusError *error, int first_arg_type, ...)
{
	dbus_bool_t retval;
	va_list var_args;

	va_start(var_args, first_arg_type);
	retval = gaim_dbus_message_get_args_valist(message, error, first_arg_type, var_args);
	va_end(var_args);

	return retval;
}

dbus_bool_t
gaim_dbus_message_get_args_valist(DBusMessage *message,
		DBusError *error, int first_arg_type, va_list var_args)
{
	DBusMessageIter iter;

	dbus_message_iter_init(message, &iter);
	return gaim_dbus_message_iter_get_args_valist(&iter, error, first_arg_type, var_args);
}

dbus_bool_t
gaim_dbus_message_iter_get_args(DBusMessageIter *iter,
		DBusError *error, int first_arg_type, ...)
{
	dbus_bool_t retval;
	va_list var_args;

	va_start(var_args, first_arg_type);
	retval = gaim_dbus_message_iter_get_args_valist(iter, error, first_arg_type, var_args);
	va_end(var_args);

	return retval;
}

#define TYPE_IS_CONTAINER(typecode)        \
	((typecode) == DBUS_TYPE_STRUCT ||     \
	 (typecode) == DBUS_TYPE_DICT_ENTRY || \
	 (typecode) == DBUS_TYPE_VARIANT ||    \
	 (typecode) == DBUS_TYPE_ARRAY)


dbus_bool_t
gaim_dbus_message_iter_get_args_valist(DBusMessageIter *iter,
		DBusError *error, int first_arg_type, va_list var_args)
{
	int spec_type, msg_type, i;

	spec_type = first_arg_type;

	for (i = 0; spec_type != DBUS_TYPE_INVALID; i++)
	{
		msg_type = dbus_message_iter_get_arg_type(iter);

		if (msg_type != spec_type)
		{
			dbus_set_error(error, DBUS_ERROR_INVALID_ARGS,
					"Argument %d is specified to be of type \"%i\", but "
					"is actually of type \"%i\"\n", i,
					spec_type, msg_type);
			return FALSE;
		}

		if (!TYPE_IS_CONTAINER(spec_type))
		{
			gpointer ptr;
			ptr = va_arg (var_args, gpointer);
			dbus_message_iter_get_basic(iter, ptr);
		}
		else
		{
			DBusMessageIter *sub;
			sub = va_arg (var_args, DBusMessageIter*);
			dbus_message_iter_recurse(iter, sub);
			gaim_debug_info("dbus", "subiter %p:%p\n", sub, * (gpointer*) sub);
			break; /* for testing only! */
		}

		spec_type = va_arg(var_args, int);
		if (!dbus_message_iter_next(iter) && spec_type != DBUS_TYPE_INVALID)
		{
			dbus_set_error (error, DBUS_ERROR_INVALID_ARGS,
					"Message has only %d arguments, but more were expected", i);
			return FALSE;
		}
	}

	return TRUE;
}



/**************************************************************************/
/** @name Useful functions                                                */
/**************************************************************************/

const char *empty_to_null(const char *str)
{
	if (str == NULL || str[0] == 0)
		return NULL;
	else
		return str;
}

const char *
null_to_empty(const char *s)
{
	if (s)
		return s;
	else
		return "";
}

dbus_int32_t *
gaim_dbusify_GList(GList *list, gboolean free_memory, dbus_int32_t *len)
{
	dbus_int32_t *array;
	int i;
	GList *elem;

	*len = g_list_length(list);
	array = g_new0(dbus_int32_t, g_list_length(list));
	for (i = 0, elem = list; elem != NULL; elem = elem->next, i++)
		array[i] = gaim_dbus_pointer_to_id(elem->data);

	if (free_memory)
		g_list_free(list);

	return array;
}

dbus_int32_t *
gaim_dbusify_GSList(GSList *list, gboolean free_memory, dbus_int32_t *len)
{
	dbus_int32_t *array;
	int i;
	GSList *elem;

	*len = g_slist_length(list);
	array = g_new0(dbus_int32_t, g_slist_length(list));
	for (i = 0, elem = list; elem != NULL; elem = elem->next, i++)
		array[i] = gaim_dbus_pointer_to_id(elem->data);

	if (free_memory)
		g_slist_free(list);

	return array;
}

gpointer *
gaim_GList_to_array(GList *list, gboolean free_memory, dbus_int32_t *len)
{
	gpointer *array;
	int i;
	GList *elem;

	*len = g_list_length(list);
	array = g_new0(gpointer, g_list_length(list));
	for (i = 0, elem = list; elem != NULL; elem = elem->next, i++)
		array[i] = elem->data;

	if (free_memory)
		g_list_free(list);

	return array;
}

gpointer *
gaim_GSList_to_array(GSList *list, gboolean free_memory, dbus_int32_t *len)
{
	gpointer *array;
	int i;
	GSList *elem;

	*len = g_slist_length(list);
	array = g_new0(gpointer, g_slist_length(list));
	for (i = 0, elem = list; elem != NULL; elem = elem->next, i++)
		array[i] = elem->data;

	if (free_memory)
		g_slist_free(list);

	return array;
}

GHashTable *
gaim_dbus_iter_hash_table(DBusMessageIter *iter, DBusError *error)
{
	GHashTable *hash;

	/* we do not need to destroy strings because they are part of the message */
	hash = g_hash_table_new(g_str_hash, g_str_equal);

	do {
		char *key, *value;
		DBusMessageIter subiter;

		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_DICT_ENTRY)
			goto error;
			/* With all due respect to Dijkstra,
			 * this goto is for exception
			 * handling, and it is ok because it
			 * avoids duplication of the code
			 * responsible for destroying the hash
			 * table.  Exceptional instructions
			 * for exceptional situations.
			 */

		dbus_message_iter_recurse(iter, &subiter);
		if (!gaim_dbus_message_iter_get_args(&subiter, error,
				DBUS_TYPE_STRING, &key,
				DBUS_TYPE_STRING, &value,
				DBUS_TYPE_INVALID))
			goto error; /* same here */

		g_hash_table_insert(hash, key, value);
	} while (dbus_message_iter_next(iter));

	return hash;

error:
	g_hash_table_destroy(hash);
	return NULL;
}

/**************************************************************/
/* DBus bindings ...                                          */
/**************************************************************/

static DBusConnection *gaim_dbus_connection;

DBusConnection *
gaim_dbus_get_connection(void)
{
	return gaim_dbus_connection;
}

#include "dbus-bindings.c"

static gboolean
gaim_dbus_dispatch_cb(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	const char *name;
	GaimDBusBinding *bindings;
	int i;

	bindings = (GaimDBusBinding*) user_data;

	if (!dbus_message_has_path(message, DBUS_PATH_GAIM))
		return FALSE;

	name = dbus_message_get_member(message);

	if (name == NULL)
		return FALSE;

	if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_METHOD_CALL)
		return FALSE;

	for (i = 0; bindings[i].name; i++)
		if (!strcmp(name, bindings[i].name))
		{
			DBusMessage *reply;
			DBusError error;

			dbus_error_init(&error);

			reply = bindings[i].handler(message, &error);

			if (reply == NULL && dbus_error_is_set(&error))
				reply = dbus_message_new_error (message,
						error.name, error.message);

			if (reply != NULL)
			{
				dbus_connection_send(connection, reply, NULL);
				dbus_message_unref(reply);
			}

			return TRUE; /* return reply! */
		}

	return FALSE;
}


static const char *
dbus_gettext(const char **ptr)
{
	const char *text = *ptr;
	*ptr += strlen(text) + 1;
	return text;
}

static void
gaim_dbus_introspect_cb(GList **bindings_list, void *bindings)
{
	*bindings_list = g_list_prepend(*bindings_list, bindings);
}

static DBusMessage *gaim_dbus_introspect(DBusMessage *message)
{
	DBusMessage *reply;
	GString *str;
	GList *bindings_list, *node;

	str = g_string_sized_new(0x1000); /* TODO: why this size? */

	g_string_append(str, "<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>\n");
	g_string_append_printf(str, "<node name='%s'>\n", DBUS_PATH_GAIM);
	g_string_append_printf(str, "<interface name='%s'>\n", DBUS_INTERFACE_GAIM);

	bindings_list = NULL;
	gaim_signal_emit(gaim_dbus_get_handle(), "dbus-introspect", &bindings_list);

	for (node = bindings_list; node; node = node->next)
	{
		GaimDBusBinding *bindings;
		int i;

		bindings = (GaimDBusBinding*)node->data;

		for (i = 0; bindings[i].name; i++)
		{
			const char *text;

			g_string_append_printf(str, "<method name='%s'>\n", bindings[i].name);

			text = bindings[i].parameters;
			while (*text)
			{
				const char *name, *direction, *type;

				direction = dbus_gettext(&text);
				type = dbus_gettext(&text);
				name = dbus_gettext(&text);

				g_string_append_printf(str,
						"<arg name='%s' type='%s' direction='%s'/>\n",
						name, type, direction);
			}
			g_string_append(str, "</method>\n");
		}
	}

	g_string_append(str, "</interface>\n</node>\n");

	reply = dbus_message_new_method_return(message);
	dbus_message_append_args(reply, DBUS_TYPE_STRING, &(str->str),
			DBUS_TYPE_INVALID);
	g_string_free(str, TRUE);
	g_list_free(bindings_list);

	return reply;
}

static DBusHandlerResult
gaim_dbus_dispatch(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	if (gaim_signal_emit_return_1(gaim_dbus_get_handle(),
			"dbus-method-called", connection, message))
		return DBUS_HANDLER_RESULT_HANDLED;

	if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_METHOD_CALL &&
			dbus_message_has_path(message, DBUS_PATH_GAIM) &&
			dbus_message_has_interface(message, DBUS_INTERFACE_INTROSPECTABLE) &&
			dbus_message_has_member(message, "Introspect"))
	{
		DBusMessage *reply;
		reply = gaim_dbus_introspect(message);
		dbus_connection_send (connection, reply, NULL);
		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void
gaim_dbus_register_bindings(void *handle, GaimDBusBinding *bindings)
{
	gaim_signal_connect(gaim_dbus_get_handle(), "dbus-method-called",
			handle,
			GAIM_CALLBACK(gaim_dbus_dispatch_cb),
			bindings);
	gaim_signal_connect(gaim_dbus_get_handle(), "dbus-introspect",
			handle,
			GAIM_CALLBACK(gaim_dbus_introspect_cb),
			bindings);
}

static void
gaim_dbus_dispatch_init(void)
{
	static DBusObjectPathVTable vtable = {NULL, &gaim_dbus_dispatch, NULL, NULL, NULL, NULL};
	DBusError error;
	int result;

	dbus_error_init(&error);
	gaim_dbus_connection = dbus_bus_get(DBUS_BUS_STARTER, &error);

	if (gaim_dbus_connection == NULL)
	{
		init_error = g_strdup_printf(N_("Failed to get connection: %s"), error.message);
		dbus_error_free(&error);
		return;
	}

	if (!dbus_connection_register_object_path(gaim_dbus_connection,
			DBUS_PATH_GAIM, &vtable, NULL))
	{
		init_error = g_strdup_printf(N_("Failed to get name: %s"), error.name);
		dbus_error_free(&error);
		return;
	}

	result = dbus_bus_request_name(gaim_dbus_connection,
			DBUS_SERVICE_GAIM, 0, &error);

	if (dbus_error_is_set(&error))
	{
		dbus_connection_unref(gaim_dbus_connection);
		dbus_error_free(&error);
		gaim_dbus_connection = NULL;
		init_error = g_strdup_printf(N_("Failed to get serv name: %s"), error.name);
		return;
	}

	dbus_connection_setup_with_g_main(gaim_dbus_connection, NULL);

	gaim_debug_misc("dbus", "okkk\n");

	gaim_signal_register(gaim_dbus_get_handle(), "dbus-method-called",
			 gaim_marshal_BOOLEAN__POINTER_POINTER,
			 gaim_value_new(GAIM_TYPE_BOOLEAN), 2,
			 gaim_value_new(GAIM_TYPE_POINTER),
			 gaim_value_new(GAIM_TYPE_POINTER));

	gaim_signal_register(gaim_dbus_get_handle(), "dbus-introspect",
			 gaim_marshal_VOID__POINTER, NULL, 1,
			 gaim_value_new_outgoing(GAIM_TYPE_POINTER));

	GAIM_DBUS_REGISTER_BINDINGS(gaim_dbus_get_handle());
}



/**************************************************************************/
/** @name Signals                                                         */
/**************************************************************************/



static char *
gaim_dbus_convert_signal_name(const char *gaim_name)
{
	int gaim_index, g_index;
	char *g_name = g_new(char, strlen(gaim_name) + 1);
	gboolean capitalize_next = TRUE;

	for (gaim_index = g_index = 0; gaim_name[gaim_index]; gaim_index++)
		if (gaim_name[gaim_index] != '-' && gaim_name[gaim_index] != '_')
		{
			if (capitalize_next)
				g_name[g_index++] = g_ascii_toupper(gaim_name[gaim_index]);
			else
				g_name[g_index++] = gaim_name[gaim_index];
			capitalize_next = FALSE;
		} else
			capitalize_next = TRUE;

	g_name[g_index] = 0;

	return g_name;
}

#define my_arg(type) (ptr != NULL ? * ((type *)ptr) : va_arg(data, type))

static void
gaim_dbus_message_append_gaim_values(DBusMessageIter *iter,
		int number, GaimValue **gaim_values, va_list data)
{
	int i;

	for (i = 0; i < number; i++)
	{
		const char *str;
		int id;
		gint xint;
		guint xuint;
		gboolean xboolean;
		gpointer ptr = NULL;

		if (gaim_value_is_outgoing(gaim_values[i]))
		{
			ptr = my_arg(gpointer);
			g_return_if_fail(ptr);
		}

		switch (gaim_values[i]->type)
		{
		case GAIM_TYPE_INT:
			xint = my_arg(gint);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &xint);
			break;
		case GAIM_TYPE_UINT:
			xuint = my_arg(guint);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &xuint);
			break;
		case GAIM_TYPE_BOOLEAN:
			xboolean = my_arg(gboolean);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &xboolean);
			break;
		case GAIM_TYPE_STRING:
			str = null_to_empty(my_arg(char*));
			dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &str);
			break;
		case GAIM_TYPE_SUBTYPE: /* registered pointers only! */
		case GAIM_TYPE_POINTER:
		case GAIM_TYPE_OBJECT:
		case GAIM_TYPE_BOXED:
			id = gaim_dbus_pointer_to_id(my_arg(gpointer));
			dbus_message_iter_append_basic(iter,
					(sizeof(void *) == 4) ? DBUS_TYPE_UINT32 : DBUS_TYPE_UINT64, &id);
			break;
		default: /* no conversion implemented */
			g_return_if_reached();
		}
	}
}

#undef my_arg

void
gaim_dbus_signal_emit_gaim(const char *name, int num_values,
		GaimValue **values, va_list vargs)
{
	DBusMessage *signal;
	DBusMessageIter iter;
	char *newname;

#if 0 /* this is noisy with no dbus connection */
	g_return_if_fail(gaim_dbus_connection);
#else
	if (gaim_dbus_connection == NULL)
		return;
#endif


	/*
	 * The test below is a hack that prevents our "dbus-method-called"
	 * signal from being propagated to dbus.  What we really need is a
	 * flag for each signal that states whether this signal is to be
	 * dbus-propagated or not.
	 */
	if (!strcmp(name, "dbus-method-called"))
		return;

	newname = gaim_dbus_convert_signal_name(name);
	signal = dbus_message_new_signal(DBUS_PATH_GAIM, DBUS_INTERFACE_GAIM, newname);
	dbus_message_iter_init_append(signal, &iter);

	gaim_dbus_message_append_gaim_values(&iter, num_values, values, vargs);

	dbus_connection_send(gaim_dbus_connection, signal, NULL);

	g_free(newname);
	dbus_message_unref(signal);
}

const char *
gaim_dbus_get_init_error(void)
{
	return init_error;
}

void *
gaim_dbus_get_handle(void)
{
	static int handle;

	return &handle;
}

void
gaim_dbus_init(void)
{
	gaim_dbus_init_ids();

	g_free(init_error);
	init_error = NULL;
	gaim_dbus_dispatch_init();
	if (init_error != NULL)
		gaim_debug_error("dbus", "%s\n", init_error);
}

void
gaim_dbus_uninit(void)
{
	/* Surely we must do SOME kind of uninitialization? */

	g_free(init_error);
	init_error = NULL;
}
