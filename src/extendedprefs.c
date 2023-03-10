/*
 * Pidgin Extended Preferences Plugin
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

#define PURPLE_PLUGINS

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
#include "debug.h"

#define KSTANGE_EP_PLUGIN_ID     "gtk-kstange-extendedprefs"

#define KSTANGE_EP_SIZE_MIN 4
#define KSTANGE_EP_SIZE_MAX 90
#define KSTANGE_EP_BLIST_TIP_MIN 0
#define KSTANGE_EP_BLIST_TIP_STD 500
#define KSTANGE_EP_BLIST_TIP_MAX 7000
#define KSTANGE_EP_BLIST_BLIST_VS_MIN 0
#define KSTANGE_EP_BLIST_BLIST_VS_MAX 5
#define KSTANGE_EP_BLIST_DEFAULT_EXPANDER 12

static const char *pref_conv_zoom        = "/plugins/gtk/kstange/extendedprefs/conv_zoom";

static const char *pref_blist_taskbar    = "/plugins/gtk/kstange/extendedprefs/blist_taskbar";

static const char *pref_tooltip_delay    = "/pidgin/blist/tooltip_delay";
static const char *pref_blist_visible    = "/pidgin/blist/list_visible";

static const char *pref_conv_show_joinpart = "/plugins/gtk/kstange/extendedprefs/conv_show_joinpart";
static const char *pref_popup_size       = "/plugins/gtk/kstange/extendedprefs/popup_size";
static const char *pref_conv_size        = "/plugins/gtk/kstange/extendedprefs/conv_size";
static const char *pref_log_size         = "/plugins/gtk/kstange/extendedprefs/log_size";
static const char *pref_blist_size       = "/plugins/gtk/kstange/extendedprefs/blist_size";
static const char *pref_blist_allow_shrink	= "/plugins/gtk/kstange/extendedprefs/blist_allow_shrink";
static const char *pref_blist_autohide   = "/plugins/gtk/kstange/extendedprefs/blist_autohide";
static const char *pref_blist_tooltip		= "/plugins/gtk/kstange/extendedprefs/blist_tooltip";
static const char *pref_blist_vspace		= "/plugins/gtk/kstange/extendedprefs/blist_vspace";
static const char *pref_blist_group_tooltip = "/plugins/gtk/kstange/extendedprefs/blist_group_tooltip";
static const char *pref_conv_typing_not	= "/plugins/gtk/kstange/extendedprefs/conv_typing_not";
static const char *pref_blist_expander		= "/plugins/gtk/kstange/extendedprefs/blist_expander";

static gdouble _point_sizes [] = { .69444444, .8333333, 1, 1.2, 1.44, 1.728, 2.0736};

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

	for(conv = purple_get_conversations(); conv != NULL; conv = conv->next) {
		PidginConversation *gtkconv = PIDGIN_CONVERSATION((PurpleConversation *)conv->data);
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

/* Creates a gtkrc style to use for set_style */
static GString*
create_style(const char* name, const char* stylename, int value, const char* widgetname) {
	GString *style = g_string_new("");
	g_string_append_printf(style, "style \"%s\" { %s=%d } widget \"%s\" style \"%s\"", name, stylename, value, widgetname, name);
	return style;
}

/* Sets the gtkrc style */
static void
set_style(GString *style) {
	gtk_rc_parse_string(style->str);

#if GTK_CHECK_VERSION(2,4,0)
   GtkSettings *setting = gtk_settings_get_default();
	gtk_rc_reset_styles(setting);
#endif

}


static void
size_prefs_init_all() {
	int value;

	value = purple_prefs_get_int(pref_conv_size);
	size_set("pidgin_conv_entry", value);
	size_set("pidgin_conv_imhtml", value);
	size_set("pidgin_prefs_font_imhtml", value);

	value = purple_prefs_get_int(pref_popup_size);
	size_set("pidgin_request_imhtml", value);
	size_set("pidgin_notify_imhtml", value);

	value = purple_prefs_get_int(pref_log_size);
	size_set("pidgin_log_imhtml", value);

	value = purple_prefs_get_int(pref_blist_size);
	size_set("pidgin_blist_treeview", value);
	reset_theme();
	resize_imhtml_fonts();
}

static void
size_prefs_update(const char *pref, PurplePrefType type, gpointer val,
				  gpointer user_data)
{
	gint value = GPOINTER_TO_INT(val);

	if (!strcmp(pref, pref_conv_size)) {
		size_set("pidgin_conv_entry", value);
		size_set("pidgin_conv_imhtml", value);
		size_set("pidgin_prefs_font_imhtml", value);
	}
	else if (!strcmp(pref, pref_popup_size)) {
		size_set("pidgin_request_imhtml", value);
		size_set("pidgin_notify_imhtml", value);
	}
	else if (!strcmp(pref, pref_log_size))
		size_set("pidgin_log_imhtml", value);
	else if (!strcmp(pref, pref_blist_size))
		size_set("pidgin_blist_treeview", value);

	reset_theme();
	resize_imhtml_fonts();
}

static void
size_prefs_clear_all() {
	size_set("pidgin_conv_entry", 0);
	size_set("pidgin_conv_imhtml", 0);
	size_set("pidgin_request_imhtml", 0);
	size_set("pidgin_notify_imhtml", 0);
	size_set("pidgin_log_imhtml", 0);
	size_set("pidgin_prefs_font_imhtml", 0);
	size_set("pidgin_blist_treeview", 0);

	reset_theme();
	resize_imhtml_fonts();
}


static void
blist_taskbar_update(const char *pref, PurplePrefType type, gpointer value,
					 gpointer user_data)
{
	PurpleBuddyList *blist = purple_get_blist();
	PidginBuddyList *gtkblist;

	if (blist) {
		/* FIXME: In Win32, the taskbar entry won't come back till the window
		   is focused. This is probably a GTK bug. */
	  	gtkblist = PIDGIN_BLIST(blist);

		if (!GTK_IS_WINDOW(gtkblist->window))
			return;
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(gtkblist->window),
										 !GPOINTER_TO_INT(value));

	}
}

static void
blist_shrink_update(const char *pref, PurplePrefType type, gpointer value,
					gpointer user_data)
{
	PurpleBuddyList *blist = purple_get_blist();

	if (blist) {
		PidginBuddyList *gtkblist = PIDGIN_BLIST(blist);

		if (!GTK_IS_WINDOW(gtkblist->window))
			return;

		GTK_WINDOW(gtkblist->window)->allow_shrink = GPOINTER_TO_INT(value);

	}
}

/* Set up the style for buddy vertical spacing */
static GString*
blist_treeview_style(int value) {

	GString *style = g_string_new("");
	g_string_append(style, "style \"blist-vspace-style\" {\n");
	g_string_append_printf(style, "GtkTreeView::vertical-separator=%d\n", value);
	g_string_append(style, "\n} widget \"*pidgin_blist_treeview\" style \"blist-vspace-style\""); 
	return style;
}

/* Update buddy vertical spacing
 * 
 * Using code ideas for style and applying style to 
 * blist from Etan Reisner <deryni@eden.rutgers.edu> GTK+ Theme Control Plugin
*/
static void
blist_vspace_update(const char *name, PurplePrefType type, gconstpointer value, gpointer data) {
	const GString *style;
	GtkSettings *setting = NULL;
	int spin_value;

	if (data != NULL) {
		GtkWidget *hbox = data;
		GtkSpinButton *spin = g_list_last(gtk_container_get_children(GTK_CONTAINER(hbox)))->data;		
		spin_value = (int)gtk_spin_button_get_value(spin);
	}
	else spin_value = GPOINTER_TO_INT(value);
	style = blist_treeview_style(spin_value);
	gtk_rc_parse_string(style->str);

#if GTK_CHECK_VERSION(2,4,0)
   setting = gtk_settings_get_default();
	gtk_rc_reset_styles(setting);
#endif

	 purple_prefs_set_int(pref_blist_vspace, spin_value);	

}

/* Update the expander size in the buddy list */
static void
blist_expander_update(const char *name, PurplePrefType type, gconstpointer value, gpointer data) {
	GString *style = g_string_new("");
	GtkSettings *setting = NULL;
	int spin_value;

	spin_value = purple_prefs_get_int(pref_blist_expander);
	style = create_style("blist-expander-style", "GtkTreeView::expander-size", spin_value, "*pidgin_blist_treeview");
	gtk_rc_parse_string(style->str);

#if GTK_CHECK_VERSION(2,4,0)
   setting = gtk_settings_get_default();
	gtk_rc_reset_styles(setting);
#endif

	
}

static void
blist_created_cb(PurpleBuddyList *blist, void *data) {
	blist_taskbar_update(NULL, 0, GINT_TO_POINTER(purple_prefs_get_bool(pref_blist_taskbar)), NULL);
	blist_shrink_update(NULL, 0, GINT_TO_POINTER(purple_prefs_get_bool(pref_blist_allow_shrink)), NULL);
	blist_vspace_update(NULL, 0, GINT_TO_POINTER(purple_prefs_get_int(pref_blist_vspace)), NULL);
}

static gboolean
chat_join_part_cb(PurpleConversation *conv, const gchar *name, PurpleConvChatBuddyFlags flags, void *data) {
	return !purple_prefs_get_bool(pref_conv_show_joinpart);
}

static void
connect_callback(PurplePlugin *plugin, const char *pref, PurplePrefCallback function) {
	purple_prefs_connect_callback(plugin, pref, function, NULL);
}

/* Callback showing the tooltip and tooltip reveal time */
static void
blist_tooltip_update(const char *name, PurplePrefType type, gconstpointer value, gpointer data) {
	GtkWidget *hbox = data;
	GtkSpinButton *spin = g_list_last(gtk_container_get_children(GTK_CONTAINER(hbox)))->data;

	if (purple_prefs_get_bool(name)) {
		gtk_widget_set_sensitive(hbox, TRUE);
		gtk_spin_button_set_value(spin, KSTANGE_EP_BLIST_TIP_STD);
	}
	else {
		gtk_widget_set_sensitive(hbox, FALSE);
		gtk_spin_button_set_value(spin, KSTANGE_EP_BLIST_TIP_MIN);
	}
}


/* Hides the converstaion window buddy typing notification */
static void
conv_typing_not_cb (const char *pref, PurplePrefType type, gpointer val, gpointer user_data) {
	GList *conv;

#define RESERVE_LINE	"";

	 for (conv = purple_get_ims(); conv != NULL; conv = conv->next) {
		if (conv != NULL) {
			PidginConversation *gtkconv = PIDGIN_CONVERSATION((PurpleConversation *)conv->data);

			if (gtkconv == NULL) return;

			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkconv->imhtml));
			GtkTextTagTable *tagtable = gtk_text_buffer_get_tag_table(buffer);

			GtkTextTag *tag = gtk_text_tag_table_lookup(tagtable, "TYPING-NOTIFICATION");
			if (purple_prefs_get_bool(pref_conv_typing_not)) {
				g_object_set(G_OBJECT(tag), "scale", 0.01, NULL);
				g_object_set(G_OBJECT(tag), "invisible", TRUE, NULL);
			}
			else {
				g_object_set(G_OBJECT(tag), "scale", 1.0, NULL);
				g_object_set(G_OBJECT(tag), "invisible", FALSE, NULL);
			}

		}
	}

}

/* Hides/destroys the group or buddy tooltip */
static void
tooltip_cb (PurpleBlistNode *node, GString *text, gboolean full) {
	purple_debug_info("Extended Prefs", "tooltip_cb: Drawing tool\n");
	if (purple_prefs_get_bool(pref_blist_group_tooltip) && PURPLE_BLIST_NODE_IS_GROUP(node)) {
		pidgin_blist_tooltip_destroy();
	}
	if (!purple_prefs_get_bool(pref_blist_tooltip) && PURPLE_BLIST_NODE_IS_BUDDY(node)) {
		pidgin_blist_tooltip_destroy();
	}
}



}

static void
delete_prefs(GtkWidget *widget, void *data) {
	/* Unregister callbacks. */
	purple_prefs_disconnect_by_handle(widget);
}

static gboolean
plugin_load(PurplePlugin *plugin) {
	PidginBuddyList *gtkblist = PIDGIN_BLIST(purple_get_blist());

	/* Set the buddy list pref to invisible.  If Pidgin is loading, the
	   buddy list will still get hidden.  Otherwise, we don't want to do
	   anything anyway. */
	if (purple_prefs_get_bool(pref_blist_autohide)) {
		purple_prefs_set_bool(pref_blist_visible, FALSE);
	}

	/* If the blist already exists, activate the prefs for it. */
	if (gtkblist != NULL && GTK_IS_WINDOW(gtkblist->window)) {
		blist_created_cb(purple_get_blist(), NULL);
	}

	/* For when the blist is created, restore prefs for it. */
	purple_signal_connect(pidgin_blist_get_handle(), "gtkblist-created", plugin, PURPLE_CALLBACK(blist_created_cb), NULL);

	/* Handle the join and part chat signals to hide the messages if needed */
	purple_signal_connect(purple_conversations_get_handle(), "chat-buddy-joining", plugin, PURPLE_CALLBACK(chat_join_part_cb), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "chat-buddy-leaving", plugin, PURPLE_CALLBACK(chat_join_part_cb), NULL);

	purple_signal_connect(purple_conversations_get_handle(), "buddy-typing", plugin, PURPLE_CALLBACK(conv_typing_not_cb), NULL);
	/* Callback for group and buddy tooltips */
	purple_signal_connect(pidgin_blist_get_handle(), "drawing-tooltip", plugin, PURPLE_CALLBACK(tooltip_cb), NULL);
	/* Callback for dim_idle buddies */
	purple_signal_connect(pidgin_blist_get_handle(), "buddy-idle-changed", plugin, PURPLE_CALLBACK(dim_idle_buddies_cb), NULL);
	/* Callback for hiding status icons */
	purple_signal_connect(pidgin_blist_get_handle(), "buddy-status-changed", plugin, PURPLE_CALLBACK(buddy_status_changed_cb), NULL);
	
	/* Initialize all font size prefs. */
	size_prefs_init_all();

	/* Connect the preference callbacks we want to use. */
	connect_callback(plugin, pref_conv_size,  (PurplePrefCallback)size_prefs_update);
	connect_callback(plugin, pref_log_size,   (PurplePrefCallback)size_prefs_update);
	connect_callback(plugin, pref_popup_size, (PurplePrefCallback)size_prefs_update);
	connect_callback(plugin, pref_blist_size, (PurplePrefCallback)size_prefs_update);

	connect_callback(plugin, pref_blist_taskbar,      (PurplePrefCallback)blist_taskbar_update);
	connect_callback(plugin, pref_blist_allow_shrink, (PurplePrefCallback)blist_shrink_update);

	/* Make sure our "enabled" pref is in sync with the tooltip's delay. */
	purple_prefs_set_bool(pref_blist_tooltip,
		(purple_prefs_get_int(pref_tooltip_delay) > 0));

	/* If enabled, make sure we set up the callback to handle it */
	if (purple_prefs_get_bool(pref_conv_typing_not)) {
		purple_signal_connect(purple_conversations_get_handle(), "buddy-typing", plugin, PURPLE_CALLBACK(conv_typing_not_cb), NULL);
	}

	/* Set the gtkrc settings upon loading */
	int value = purple_prefs_get_int(pref_blist_expander);
	GString* style = create_style("blist-expander-style", "GtkTreeView::expander-size", value, "*pidgin_blist_treeview");
	set_style(style);


	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	PidginBuddyList *gtkblist = PIDGIN_BLIST(purple_get_blist());

	/* Disable buddy list modifications we set */
	if (gtkblist != NULL && GTK_IS_WINDOW(gtkblist->window)) {
		/* FIXME: In Win32, the taskbar entry won't come back till the window
		   is focused. */
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(gtkblist->window), FALSE);

		GTK_WINDOW(gtkblist->window)->allow_shrink = FALSE;
	}

	/* Reset the buddy list's visible preference to its actual state */
	if (purple_prefs_get_bool(pref_blist_autohide)) {
		gboolean visible;
		if (gtkblist != NULL && GTK_IS_WINDOW(gtkblist->window)) {
			g_object_get(G_OBJECT(gtkblist->window), "visible", &visible, NULL);
		} else {
			visible = FALSE;
		}
		purple_prefs_set_bool(pref_blist_visible, visible);
	}

	/* Reset GtkTreeView vertical-separator back to default value */
	GString *style = blist_treeview_style(2);
	set_style(style);
	
	/* Reset expander size back to default size (12) */
	style = create_style("blist-expander-style", 
		"GtkTreeView::expander-size", 
		KSTANGE_EP_BLIST_DEFAULT_EXPANDER, 
		"*pidgin_blist_treeview");
	set_style(style);

	/* Reset all fonts back to standard sizes. */
	size_prefs_clear_all();

	/* Disconnect all the prefs callbacks */
	purple_prefs_disconnect_by_handle(plugin);

	return TRUE;
}

static GtkWidget* get_config_frame(PurplePlugin *plugin) {
	GtkWidget *ret, *nb, *hbox, *spin, *page, *cb;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkSizeGroup *sg;

	ret = gtk_vbox_new(FALSE, 6);

	nb = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(ret), nb, FALSE, FALSE, 0);

	/* If this frame is destroyed, clean up after it. */
        g_signal_connect(G_OBJECT(ret), "destroy",
                                         G_CALLBACK(delete_prefs), NULL);

	/* Create a notebook tab for the General prefs */
	page = gtk_vbox_new(FALSE, 18);
	label = gtk_label_new("General");
	gtk_notebook_append_page(GTK_NOTEBOOK(nb), page, label);
	gtk_container_set_border_width (GTK_CONTAINER(page), 12);

	vbox = pidgin_make_frame (page, "Interface Font Sizes (points)");

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	/* Conversations */
	pidgin_prefs_labeled_spin_button(vbox, "_Conversations:",
									   pref_conv_size,
									   KSTANGE_EP_SIZE_MIN,
									   KSTANGE_EP_SIZE_MAX,
									   sg);

	/* Log Viewer Size */
	pidgin_prefs_labeled_spin_button(vbox, "Log _Viewer:",
									   pref_log_size,
									   KSTANGE_EP_SIZE_MIN,
									   KSTANGE_EP_SIZE_MAX,
									   sg);

	/* Popup Dialogs */
	pidgin_prefs_labeled_spin_button(vbox, "Information _Dialogs:",
									   pref_popup_size,
									   KSTANGE_EP_SIZE_MIN,
									   KSTANGE_EP_SIZE_MAX,
									   sg);

	/* Buddy List Size */
	pidgin_prefs_labeled_spin_button(vbox, "Budd_y List:",
									   pref_blist_size,
									   KSTANGE_EP_SIZE_MIN,
									   KSTANGE_EP_SIZE_MAX,
									   sg);

	if (gtk_check_version(2, 4, 0)) {
		label = gtk_label_new_with_mnemonic("You must close and reopen any affected windows\nfor font changes to take effect.");
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	}

	vbox = pidgin_make_frame (page, "Conversations");

	pidgin_prefs_checkbox("Show _join and part messages in chats",
							pref_conv_show_joinpart, vbox);

	/* Hide conversation window typing notification */
	cb = pidgin_prefs_checkbox("Hide conversation window typing notification",
	 						pref_conv_typing_not, vbox);
 	purple_prefs_connect_callback(ret, pref_conv_typing_not,
									 (PurplePrefCallback)conv_typing_not_cb, cb);
	vbox = pidgin_make_frame (page, "Buddy List");

	/* Tooltip Delay */
	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	pidgin_prefs_checkbox("Show buddy _tooltips:", pref_blist_tooltip, hbox);
	spin = pidgin_prefs_labeled_spin_button(hbox, "Reveal delay (ms):",
										pref_tooltip_delay,
										KSTANGE_EP_BLIST_TIP_MIN,
										KSTANGE_EP_BLIST_TIP_MAX,
										NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(spin), purple_prefs_get_bool(pref_blist_tooltip));
	purple_prefs_connect_callback(ret, pref_blist_tooltip,
									blist_tooltip_update, spin);

	/* Window Widget Tweaking Prefs */
	pidgin_prefs_checkbox("Show buddy _list entry in taskbar",
							pref_blist_taskbar, vbox);

	pidgin_prefs_checkbox("Hide buddy list at _startup",
							pref_blist_autohide, vbox);

	pidgin_prefs_checkbox("Allow buddy list to s_hrink below normal size constraints",
							pref_blist_allow_shrink, vbox);
	
	/* Buddy vertical spacing in the buddy list */
	spin = pidgin_prefs_labeled_spin_button(vbox, "Buddy vertical spacing:",
							pref_blist_vspace, 
							KSTANGE_EP_BLIST_BLIST_VS_MIN, 
							KSTANGE_EP_BLIST_BLIST_VS_MAX, 
							NULL);
	purple_prefs_connect_callback(ret, pref_blist_vspace,
							blist_vspace_update, spin);

	spin = pidgin_prefs_labeled_spin_button(vbox, "Expander size:",
							pref_blist_expander,
							KSTANGE_EP_BLIST_BLIST_VS_MIN,
							KSTANGE_EP_BLIST_BLIST_VS_MAX+10,
							NULL);
	purple_prefs_connect_callback(ret, pref_blist_expander,
							blist_expander_update, spin);

	/* Group tooltip */
	cb = pidgin_prefs_checkbox("Hide group tooltips",
							pref_blist_group_tooltip, vbox);
#if 0
	/* Create a notebook tab for the Accels editor */
	page = gtk_vbox_new(FALSE, 18);
	label = gtk_label_new("Accels");
	gtk_notebook_append_page(GTK_NOTEBOOK(nb), page, label);
	gtk_container_set_border_width (GTK_CONTAINER(page), 12);
#endif

	gtk_widget_show_all(ret);
	return ret;
}

static PidginPluginUiInfo ui_info =
{
	get_config_frame,   /* UI config frame */
	0                   /* page_num */
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	PIDGIN_PLUGIN_TYPE,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	KSTANGE_EP_PLUGIN_ID,
	"Extended Preferences",
	EP_VERSION,
	"Provides a number of additional preferences to Pidgin users.",
	"This plugin provides a number of different preferences that were "
	"either rejected or \"slashed\" from Pidgin.  It was originally "
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
init_plugin(PurplePlugin *plugin)
{
	purple_prefs_add_none("/plugins/gtk/kstange");
	purple_prefs_add_none("/plugins/gtk/kstange/extendedprefs");
	purple_prefs_add_bool(pref_conv_show_joinpart, TRUE);
	purple_prefs_add_int(pref_conv_size, 8);
	purple_prefs_add_int(pref_popup_size, 8);
	purple_prefs_add_int(pref_log_size, 8);
	purple_prefs_add_int(pref_blist_size, 8);
	purple_prefs_add_bool(pref_blist_taskbar, TRUE);
	purple_prefs_add_bool(pref_blist_allow_shrink, FALSE);
	purple_prefs_add_bool(pref_blist_autohide, FALSE);
	purple_prefs_add_bool(pref_blist_tooltip, TRUE);
	purple_prefs_add_int(pref_blist_vspace, 2);
	purple_prefs_add_bool(pref_blist_group_tooltip, FALSE);
	purple_prefs_add_bool(pref_conv_typing_not, FALSE);
	purple_prefs_add_int(pref_blist_expander, KSTANGE_EP_BLIST_DEFAULT_EXPANDER);
	purple_prefs_add_bool(pref_hide_status_icon, FALSE);
	purple_prefs_add_int(pref_status_box_size, 8);
	purple_prefs_add_bool(pref_dim_idle_buddies, TRUE);

	if (purple_prefs_exists(pref_conv_zoom)) {
		double zoom = 8 * 0.01 * purple_prefs_get_int(pref_conv_zoom);
		purple_prefs_set_int(pref_conv_size,  floor(zoom));
		purple_prefs_set_int(pref_popup_size, floor(zoom));
		purple_prefs_set_int(pref_log_size,   floor(zoom));
		purple_prefs_set_int(pref_blist_size, floor(zoom));
		purple_prefs_remove(pref_conv_zoom);
	}
}

PURPLE_INIT_PLUGIN(extendedprefs, init_plugin, info)
