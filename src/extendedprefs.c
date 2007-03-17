/*
 * Gaim Extended Preferences Plugin
 *
 * Copyright 2004-05 Kevin Stange <extprefs@simguy.net>
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

#ifndef _WIN32
# ifdef HAVE_CONFIG_H
#  include "extprefs_config.h"
# endif
#endif

#define GAIM_PLUGINS

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "plugin.h"
#include "signals.h"
#include "version.h"

#include "gtkblist.h"
#include "gtkconv.h"
#include "gtkimhtml.h"
#include "gtkplugin.h"
#include "gtkprefs.h"
#include "gtkutils.h"

#define KSTANGE_EP_PLUGIN_ID     "gtk-kstange-extendedprefs"

#define KSTANGE_EP_SIZE_MIN 4
#define KSTANGE_EP_SIZE_MAX 90
#define KSTANGE_EP_BLIST_TIP_MIN 0
#define KSTANGE_EP_BLIST_TIP_MAX 7000

static const char *pref_conv_zoom        = "/plugins/gtk/kstange/extendedprefs/conv_zoom";

static const char *pref_blist_taskbar    = "/plugins/gtk/kstange/extendedprefs/blist_taskbar";

static const char *pref_tooltip_delay    = "/gaim/gtk/blist/tooltip_delay";

static const char *pref_conv_show_joinpart = "/plugins/gtk/kstange/extendedprefs/conv_show_joinpart";
static const char *pref_popup_size       = "/plugins/gtk/kstange/extendedprefs/popup_size";
static const char *pref_conv_size        = "/plugins/gtk/kstange/extendedprefs/conv_size";
static const char *pref_log_size         = "/plugins/gtk/kstange/extendedprefs/log_size";
static const char *pref_blist_size       = "/plugins/gtk/kstange/extendedprefs/blist_size";
static const char *pref_blist_allow_shrink	= "/plugins/gtk/kstange/extendedprefs/blist_allow_shrink";
static const char *pref_blist_autohide   = "/plugins/gtk/kstange/extendedprefs/blist_autohide";

static gdouble _point_sizes [] = { .69444444, .8333333, 1, 1.2, 1.44, 1.728, 2.0736};

static gboolean logging_in = FALSE;

static void
size_set(const char *widget, int value) {
	char *style;

	/* Define the style we're going to adhere to */
	if(value > 0)
		style = g_strdup_printf("style \"%s\" { font_name = \"%d\" }", widget, value);
	else
		style = g_strdup_printf("style \"%s\" { font_name = \"\" }", widget);
	gtk_rc_parse_string(style);
	g_free(style);

	/* Make sure the widget in question is tied to the style */
	style = g_strdup_printf("widget \"*%s\" style \"%s\"\n", widget, widget);
	gtk_rc_parse_string(style);
	g_free(style);
}

/* This should allow dynamic resizing of coversation text */
static void
recalculate_font_sizes(GtkTextTag *tag, gpointer imhtml)
{
	if (strncmp(tag->name, "FONT SIZE ", 10) == 0) {
		GtkTextAttributes *attr = gtk_text_view_get_default_attributes(GTK_TEXT_VIEW(imhtml));
		int size;
		size = strtol(tag->name + 10, NULL, 10);
		g_object_set(G_OBJECT(tag), "size",
					 (gint) (pango_font_description_get_size(attr->font) *
							 (double) _point_sizes[size-1]), NULL);
	}
}

static void
resize_imhtml_fonts()
{
	GList *conv;

	for(conv = gaim_get_conversations(); conv != NULL; conv = conv->next) {
		GaimGtkConversation *gtkconv = GAIM_GTK_CONVERSATION((GaimConversation *)conv->data);
		gtk_text_tag_table_foreach(gtk_text_buffer_get_tag_table(GTK_IMHTML(gtkconv->imhtml)->text_buffer),
								   recalculate_font_sizes, gtkconv->imhtml);
		gtk_text_tag_table_foreach(gtk_text_buffer_get_tag_table(GTK_IMHTML(gtkconv->entry)->text_buffer),
								   recalculate_font_sizes, gtkconv->entry);

	}
}

static void
reset_theme() {
  	/* This crazy piece of code is derived from GTK-Wimp's method of being
	   GTK 2.4 compatible without being GTK 2.4 dependent. */

	if (!gtk_check_version(2, 4, 0)) {
		GModule *module = g_module_open(NULL, 0);
		gpointer prc_reset_function = NULL;
		void (*rc_reset_function) (GtkSettings * settings) = NULL;

		if(module) {
			g_module_symbol (module, "gtk_rc_reset_styles", &prc_reset_function);
			rc_reset_function = prc_reset_function;
			(*rc_reset_function)(gtk_settings_get_default());
			g_module_close(module);
		}
	}
}

static void
size_prefs_init_all() {
	int value;

	value = gaim_prefs_get_int(pref_conv_size);
	size_set("gaim_gtkconv_entry", value);
	size_set("gaim_gtkconv_imhtml", value);
	size_set("gaim_gtkprefs_font_imhtml", value);

	value = gaim_prefs_get_int(pref_popup_size);
	size_set("gaim_gtkrequest_imhtml", value);
	size_set("gaim_gtknotify_imhtml", value);

	value = gaim_prefs_get_int(pref_log_size);
	size_set("gaim_gtklog_imhtml", value);

	value = gaim_prefs_get_int(pref_blist_size);
	size_set("gaim_gtkblist_treeview", value);
	reset_theme();
	resize_imhtml_fonts();
}

static void
size_prefs_update(const char *pref, GaimPrefType type, gpointer val,
				  gpointer user_data)
{
	gint value = GPOINTER_TO_INT(val);

	if (!strcmp(pref, pref_conv_size)) {
		size_set("gaim_gtkconv_entry", value);
		size_set("gaim_gtkconv_imhtml", value);
		size_set("gaim_gtkprefs_font_imhtml", value);
	}
	else if (!strcmp(pref, pref_popup_size)) {
		size_set("gaim_gtkrequest_imhtml", value);
		size_set("gaim_gtknotify_imhtml", value);
	}
	else if (!strcmp(pref, pref_log_size))
		size_set("gaim_gtklog_imhtml", value);
	else if (!strcmp(pref, pref_blist_size))
		size_set("gaim_gtkblist_treeview", value);

	reset_theme();
	resize_imhtml_fonts();
}

static void
size_prefs_clear_all() {
	size_set("gaim_gtkconv_entry", 0);
	size_set("gaim_gtkconv_imhtml", 0);
	size_set("gaim_gtkrequest_imhtml", 0);
	size_set("gaim_gtknotify_imhtml", 0);
	size_set("gaim_gtklog_imhtml", 0);
	size_set("gaim_gtkprefs_font_imhtml", 0);
	size_set("gaim_gtkblist_treeview", 0);

	reset_theme();
	resize_imhtml_fonts();
}


static void
blist_taskbar_update(const char *pref, GaimPrefType type, gpointer value,
					 gpointer user_data)
{
	GaimBuddyList *blist = gaim_get_blist();
	GaimGtkBuddyList *gtkblist;

	if (blist) {
		/* FIXME: In Win32, the taskbar entry won't come back till the window
		   is focused. This is probably a GTK bug. */
	  	gtkblist = GAIM_GTK_BLIST(blist);

		if (!GTK_IS_WINDOW(gtkblist->window))
			return;

		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(gtkblist->window),
										 !GPOINTER_TO_INT(value));
	}
}

static void
blist_shrink_update(const char *pref, GaimPrefType type, gpointer value,
					gpointer user_data)
{
	GaimBuddyList *blist = gaim_get_blist();

	if (blist) {
		GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(blist);

		if (!GTK_IS_WINDOW(gtkblist->window))
			return;

		GTK_WINDOW(gtkblist->window)->allow_shrink = GPOINTER_TO_INT(value);

	}
}

static void
blist_show_cb(GtkWidget *blist, void *nothing)
{
	if(gaim_prefs_get_bool(pref_blist_autohide) && logging_in == TRUE)
		gtk_widget_hide(blist);
}

static void
blist_created_cb(GaimBuddyList *blist, void *data) {
	GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(blist);

	blist_taskbar_update(NULL, 0, (gpointer)gaim_prefs_get_bool(pref_blist_taskbar), NULL);
	blist_shrink_update(NULL, 0, (gpointer)gaim_prefs_get_bool(pref_blist_allow_shrink), NULL);

	g_signal_connect(G_OBJECT(gtkblist->window), "show",
					 G_CALLBACK(blist_show_cb), NULL);

	if (gaim_prefs_get_bool(pref_blist_autohide) && (gboolean)data == TRUE) {
		gtk_widget_hide(gtkblist->window);
		logging_in = TRUE;
	}
}

static void
blist_signon_check_cb(GaimConnection *gc, void *data)
{
	if (gaim_connections_get_connecting() == NULL)
		logging_in = FALSE;
}

static gboolean
chat_join_part_cb(GaimConversation *conv, const gchar *name, GaimConvChatBuddyFlags flags, void *data) {
	return !gaim_prefs_get_bool(pref_conv_show_joinpart);
}

static void
connect_callback(GaimPlugin *plugin, const char *pref, GaimPrefCallback function) {
	gaim_prefs_connect_callback(plugin, pref, function, NULL);
}

static gboolean
plugin_load(GaimPlugin *plugin) {
	GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(gaim_get_blist());

	if (gtkblist != NULL && GTK_IS_WINDOW(gtkblist->window)) {
		blist_created_cb(gaim_get_blist(), (gpointer)FALSE);
	}

	gaim_signal_connect(gaim_gtk_blist_get_handle(), "gtkblist-created", plugin, GAIM_CALLBACK(blist_created_cb), (gpointer)TRUE);
	gaim_signal_connect(gaim_connections_get_handle(), "signed-on", plugin, GAIM_CALLBACK(blist_signon_check_cb), NULL);

	gaim_signal_connect(gaim_conversations_get_handle(), "chat-buddy-joining", plugin, GAIM_CALLBACK(chat_join_part_cb), NULL);
	gaim_signal_connect(gaim_conversations_get_handle(), "chat-buddy-leaving", plugin, GAIM_CALLBACK(chat_join_part_cb), NULL);

	size_prefs_init_all();

	/* Connect the preference callbacks we want to use. */
	connect_callback(plugin, pref_conv_size,  (GaimPrefCallback)size_prefs_update);
	connect_callback(plugin, pref_log_size,   (GaimPrefCallback)size_prefs_update);
	connect_callback(plugin, pref_popup_size, (GaimPrefCallback)size_prefs_update);
	connect_callback(plugin, pref_blist_size, (GaimPrefCallback)size_prefs_update);

	connect_callback(plugin, pref_blist_taskbar,      (GaimPrefCallback)blist_taskbar_update);
	connect_callback(plugin, pref_blist_allow_shrink, (GaimPrefCallback)blist_shrink_update);

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin) {
	GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(gaim_get_blist());

	if (gtkblist != NULL && GTK_IS_WINDOW(gtkblist->window)) {
		/* FIXME: In Win32, the taskbar entry won't come back till the window
		   is focused. */
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(gtkblist->window), FALSE);

		GTK_WINDOW(gtkblist->window)->allow_shrink = FALSE;
	}

	size_prefs_clear_all();

	return TRUE;
}

static GtkWidget* get_config_frame(GaimPlugin *plugin) {
	GtkWidget *ret;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkSizeGroup *sg;

	ret = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width (GTK_CONTAINER (ret), 12);

	vbox = gaim_gtk_make_frame (ret, "Interface Font Sizes (points)");

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	/* Conversations */
	gaim_gtk_prefs_labeled_spin_button(vbox, "_Conversations:",
									   pref_conv_size,
									   KSTANGE_EP_SIZE_MIN,
									   KSTANGE_EP_SIZE_MAX,
									   sg);

	/* Log Viewer Size */
	gaim_gtk_prefs_labeled_spin_button(vbox, "Log _Viewer:",
									   pref_log_size,
									   KSTANGE_EP_SIZE_MIN,
									   KSTANGE_EP_SIZE_MAX,
									   sg);

	/* Popup Dialogs */
	gaim_gtk_prefs_labeled_spin_button(vbox, "Information _Dialogs:",
									   pref_popup_size,
									   KSTANGE_EP_SIZE_MIN,
									   KSTANGE_EP_SIZE_MAX,
									   sg);

	/* Buddy List Size */
	gaim_gtk_prefs_labeled_spin_button(vbox, "Budd_y List:",
									   pref_blist_size,
									   KSTANGE_EP_SIZE_MIN,
									   KSTANGE_EP_SIZE_MAX,
									   sg);

	if (gtk_check_version(2, 4, 0)) {
		label = gtk_label_new_with_mnemonic("You must close and reopen any affected windows\nfor font changes to take effect.");
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	}

	vbox = gaim_gtk_make_frame (ret, "Conversations");

	gaim_gtk_prefs_checkbox("Show _join and part messages in chats",
							pref_conv_show_joinpart, vbox);

	vbox = gaim_gtk_make_frame (ret, "Buddy List");

	/* Tooltip Delay */
	gaim_gtk_prefs_labeled_spin_button(vbox, "_Tooltip reveal delay (ms):",
									   pref_tooltip_delay,
									   KSTANGE_EP_BLIST_TIP_MIN,
									   KSTANGE_EP_BLIST_TIP_MAX,
									   NULL);

	/* Window Widget Tweaking Prefs */
	gaim_gtk_prefs_checkbox("Show buddy _list entry in taskbar",
							pref_blist_taskbar, vbox);

	gaim_gtk_prefs_checkbox("Hide buddy list at _signon",
							pref_blist_autohide, vbox);

	gaim_gtk_prefs_checkbox("Allow buddy list to s_hrink below normal size constraints",
							pref_blist_allow_shrink, vbox);

	gtk_widget_show_all(ret);
	return ret;
}

static GaimGtkPluginUiInfo ui_info =
{
	get_config_frame,   /* UI config frame */
	0                   /* page_num */
};

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,
	GAIM_GTK_PLUGIN_TYPE,
	0,
	NULL,
	GAIM_PRIORITY_DEFAULT,
	KSTANGE_EP_PLUGIN_ID,
	"Extended Preferences",
	EP_VERSION,
	"Provides a number of additional preferences to Gaim users.",
	"This plugin provides a number of different preferences that were "
	"either rejected or \"slashed\" from Gaim.  It was originally "
	"created to provide Windows users with a way to make the text "
	"inside their conversation windows larger, but now performs a "
	"number of other functions.",
	"Kevin Stange <extprefs@simguy.net>",
	"http://gaim-extprefs.sf.net/",
	plugin_load,
	plugin_unload,
	NULL,
	&ui_info,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(GaimPlugin *plugin)
{
	gaim_prefs_add_none("/plugins/gtk/kstange");
	gaim_prefs_add_none("/plugins/gtk/kstange/extendedprefs");
	gaim_prefs_add_none("/plugins/gtk/kstange/extendedprefs/conv_buttons");
	gaim_prefs_add_bool(pref_conv_show_joinpart, TRUE);
	gaim_prefs_add_int(pref_conv_size, 8);
	gaim_prefs_add_int(pref_popup_size, 8);
	gaim_prefs_add_int(pref_log_size, 8);
	gaim_prefs_add_int(pref_blist_size, 8);
	gaim_prefs_add_bool(pref_blist_taskbar, TRUE);
	gaim_prefs_add_bool(pref_blist_allow_shrink, FALSE);
	gaim_prefs_add_bool(pref_blist_autohide, FALSE);

	if (gaim_prefs_exists(pref_conv_zoom)) {
		double zoom = 8 * 0.01 * gaim_prefs_get_int(pref_conv_zoom);
		gaim_prefs_set_int(pref_conv_size,  floor(zoom));
		gaim_prefs_set_int(pref_popup_size, floor(zoom));
		gaim_prefs_set_int(pref_log_size,   floor(zoom));
		gaim_prefs_set_int(pref_blist_size, floor(zoom));
		gaim_prefs_remove(pref_conv_zoom);
	}
}

GAIM_INIT_PLUGIN(extendedprefs, init_plugin, info)
