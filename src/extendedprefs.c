/*
 * Gaim Extended Preferences Plugin
 *
 * Copyright 2004 Kevin Stange <extprefs@simguy.net>
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

static const char *pref_conv_show_warn   = "/plugins/gtk/kstange/extendedprefs/conv_buttons/warn";
static const char *pref_conv_show_block  = "/plugins/gtk/kstange/extendedprefs/conv_buttons/block";
static const char *pref_conv_show_file   = "/plugins/gtk/kstange/extendedprefs/conv_buttons/file";
static const char *pref_conv_show_add    = "/plugins/gtk/kstange/extendedprefs/conv_buttons/add";
static const char *pref_conv_show_info   = "/plugins/gtk/kstange/extendedprefs/conv_buttons/info";
static const char *pref_conv_show_invite = "/plugins/gtk/kstange/extendedprefs/conv_buttons/invite";
static const char *pref_conv_show_joinpart = "/plugins/gtk/kstange/extendedprefs/conv_show_joinpart";
static const char *pref_popup_size       = "/plugins/gtk/kstange/extendedprefs/popup_size";
static const char *pref_conv_size        = "/plugins/gtk/kstange/extendedprefs/conv_size";
static const char *pref_log_size         = "/plugins/gtk/kstange/extendedprefs/log_size";
static const char *pref_blist_size       = "/plugins/gtk/kstange/extendedprefs/blist_size";
static const char *pref_blist_allow_shrink	= "/plugins/gtk/kstange/extendedprefs/blist_allow_shrink";
static const char *pref_blist_autohide   = "/plugins/gtk/kstange/extendedprefs/blist_autohide";
static const char *pref_blist_hidden     = "/plugins/gtk/kstange/extendedprefs/blist_hidden";

static GList *pref_callbacks;

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

static void
reset_theme() {
  	/* This crazy piece of code is derived from GTK-Wimp's method of being
	   GTK 2.4 compatible without being GTK 2.4 dependent. */

	if (!gtk_check_version(2, 4, 0)) {
		GModule *module = g_module_open(NULL, 0);
		void (*rc_reset_function) (GtkSettings * settings) = NULL;

		if(module) {
			g_module_symbol (module, "gtk_rc_reset_styles",
							 (gpointer *)(&rc_reset_function));
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
}

static void
conv_buttons_set(GaimConversation *c, const char *pref, gboolean value) {
	GaimGtkConversation *gtkconv;
	GtkWidget *target = NULL;

	GaimConversationType type = gaim_conversation_get_type(c);
	gtkconv = GAIM_GTK_CONVERSATION(c);

	if (gtkconv != NULL) {
		if (!strcmp(pref, pref_conv_show_warn) && type == GAIM_CONV_IM)
			target = gtkconv->u.im->warn;
		else if (!strcmp(pref, pref_conv_show_block) && type == GAIM_CONV_IM)
			target = gtkconv->u.im->block;
		else if (!strcmp(pref, pref_conv_show_file) && type == GAIM_CONV_IM)
			target = gtkconv->u.im->send_file;
		else if (!strcmp(pref, pref_conv_show_add))
			if (gaim_find_buddy(gaim_conversation_get_account(c),
								gaim_conversation_get_name(c)) != NULL ||
			    gaim_blist_find_chat(gaim_conversation_get_account(c),
									 gaim_conversation_get_name(c)) != NULL)
				target = gtkconv->remove;
			else
				target = gtkconv->add;

		else if (!strcmp(pref, pref_conv_show_info) && type == GAIM_CONV_IM)
			target = gtkconv->info;
		else if (!strcmp(pref, pref_conv_show_invite) && type == GAIM_CONV_CHAT)
			target = gtkconv->u.chat->invite;

		if (target == NULL || !GTK_IS_WIDGET(target))
			return;

		if (value == TRUE)
			gtk_widget_show(target);
		else if (value == FALSE)
			gtk_widget_hide(target);
	}
}

static void
conv_buttons_set_all(const char *pref, GaimPrefType type, gpointer value,
					 gpointer user_data)
{
	GList *conv;

	for(conv = gaim_get_conversations(); conv != NULL; conv = conv->next)
		conv_buttons_set(conv->data, pref, GPOINTER_TO_INT(value));
}

static void
conv_buttons_init(GaimConversation *c) {
	conv_buttons_set(c, pref_conv_show_warn, gaim_prefs_get_bool(pref_conv_show_warn));
	conv_buttons_set(c, pref_conv_show_block, gaim_prefs_get_bool(pref_conv_show_block));
	conv_buttons_set(c, pref_conv_show_file, gaim_prefs_get_bool(pref_conv_show_file));
	conv_buttons_set(c, pref_conv_show_add, gaim_prefs_get_bool(pref_conv_show_add));
	conv_buttons_set(c, pref_conv_show_info, gaim_prefs_get_bool(pref_conv_show_info));
	conv_buttons_set(c, pref_conv_show_invite, gaim_prefs_get_bool(pref_conv_show_invite));
}

static void
conv_button_change(GtkWidget *widget, void *data)
{
	conv_buttons_init((GaimConversation *)data);

}

static void
conv_connect_signals(GtkWidget *ignored, GaimConversation *c)
{
	GaimConversationType type = gaim_conversation_get_type(c);
	GaimGtkConversation *gtkconv = GAIM_GTK_CONVERSATION(c);

	if (gtkconv != NULL) {
		if (type == GAIM_CONV_IM && gtkconv->u.im != NULL) {

			g_signal_connect(G_OBJECT(gtkconv->u.im->warn), "show",
						 G_CALLBACK(conv_button_change), c);

			g_signal_connect(G_OBJECT(gtkconv->u.im->block), "show",
						 G_CALLBACK(conv_button_change), c);

			g_signal_connect(G_OBJECT(gtkconv->u.im->send_file), "show",
						 G_CALLBACK(conv_button_change), c);

			g_signal_connect(G_OBJECT(gtkconv->info), "show",
						 G_CALLBACK(conv_button_change), c);
		}
		if (type == GAIM_CONV_IM && gtkconv->u.chat != NULL) {

			g_signal_connect(G_OBJECT(gtkconv->u.chat->invite), "show",
						 G_CALLBACK(conv_button_change), c);
		}

		g_signal_connect(G_OBJECT(gtkconv->add), "show",
					 G_CALLBACK(conv_button_change), c);

		g_signal_connect(G_OBJECT(gtkconv->remove), "show",
					 G_CALLBACK(conv_button_change), c);

	}

	conv_buttons_init(c);
}


static void
conv_prefs_init(GaimConversation *c) {
	conv_connect_signals(NULL, c);
}

static void
conv_prefs_init_all() {
	GList *conv;

	for(conv = gaim_get_conversations(); conv != NULL; conv = conv->next)
		conv_prefs_init(conv->data);
}

static void
conv_prefs_clear_all() {
	GList *conv;

	for(conv = gaim_get_conversations(); conv != NULL; conv = conv->next) {
		conv_buttons_set(conv->data, pref_conv_show_warn, TRUE);
		conv_buttons_set(conv->data, pref_conv_show_block, TRUE);
		conv_buttons_set(conv->data, pref_conv_show_file, TRUE);
		conv_buttons_set(conv->data, pref_conv_show_add, TRUE);
		conv_buttons_set(conv->data, pref_conv_show_info, TRUE);
		conv_buttons_set(conv->data, pref_conv_show_invite, TRUE);
	}
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
	if(logging_in == TRUE)
		gtk_widget_hide(blist);
}

static void
blist_created_cb(GaimBuddyList *blist, void *data) {
	GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(blist);

	blist_taskbar_update(NULL, 0, NULL, NULL);
	blist_shrink_update(NULL, 0, NULL, NULL);

	if (gaim_prefs_get_bool(pref_blist_autohide) && (gboolean)data == TRUE) {
		gtk_widget_hide(gtkblist->window);
		logging_in = TRUE;

		g_signal_connect(G_OBJECT(gtkblist->window), "show",
						 G_CALLBACK(blist_show_cb), NULL);
	}
}

static void
blist_signon_check_cb(GaimConnection *gc, void *data)
{
	GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(gaim_get_blist());

	if (gaim_connections_get_connecting() == NULL)
		logging_in = FALSE;
}

static gboolean
chat_join_part_cb(GaimConversation *conv, const gchar *name, GaimConvChatBuddyFlags flags, void *data) {
	return !gaim_prefs_get_bool(pref_conv_show_joinpart);
}

static void
connect_callback(const char *pref, GaimPrefCallback function) {
	guint callback = gaim_prefs_connect_callback(pref, function, NULL);
	pref_callbacks = g_list_append(pref_callbacks, &callback);
}

gboolean plugin_load(GaimPlugin *plugin) {
	GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(gaim_get_blist());

	gaim_signal_connect((void*)gaim_conversations_get_handle(),
						"conversation-created", plugin,
						GAIM_CALLBACK(conv_prefs_init), NULL);

	if (gtkblist != NULL && GTK_IS_WINDOW(gtkblist->window)) {
		blist_created_cb(gaim_get_blist(), (gpointer)FALSE);
	}

	gaim_signal_connect(gaim_gtk_blist_get_handle(), "gtkblist-created", plugin, GAIM_CALLBACK(blist_created_cb), (gpointer)TRUE);
	gaim_signal_connect(gaim_connections_get_handle(), "signed-on", plugin, GAIM_CALLBACK(blist_signon_check_cb), NULL);

	gaim_signal_connect(gaim_conversations_get_handle(), "chat-buddy-joining", plugin, GAIM_CALLBACK(chat_join_part_cb), NULL);
	gaim_signal_connect(gaim_conversations_get_handle(), "chat-buddy-leaving", plugin, GAIM_CALLBACK(chat_join_part_cb), NULL);

	conv_prefs_init_all();
	size_prefs_init_all();

	/* Connect the preference callbacks we want to use. */
	connect_callback(pref_conv_size,  size_prefs_update);
	connect_callback(pref_log_size,   size_prefs_update);
	connect_callback(pref_popup_size, size_prefs_update);
	connect_callback(pref_blist_size, size_prefs_update);

	connect_callback(pref_conv_show_add,    conv_buttons_set_all);
	connect_callback(pref_conv_show_warn,   conv_buttons_set_all);
	connect_callback(pref_conv_show_block,  conv_buttons_set_all);
	connect_callback(pref_conv_show_file,   conv_buttons_set_all);
	connect_callback(pref_conv_show_info,   conv_buttons_set_all);
	connect_callback(pref_conv_show_invite, conv_buttons_set_all);

	connect_callback(pref_blist_taskbar,      blist_taskbar_update);
	connect_callback(pref_blist_allow_shrink, blist_shrink_update);

	return TRUE;
}

gboolean plugin_unload(GaimPlugin *plugin) {
	GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(gaim_get_blist());

	if (gtkblist != NULL && GTK_IS_WINDOW(gtkblist->window)) {
		/* FIXME: In Win32, the taskbar entry won't come back till the window
		   is focused. */
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(gtkblist->window), FALSE);

		GTK_WINDOW(gtkblist->window)->allow_shrink = FALSE;
	}

	g_list_foreach(pref_callbacks, (GFunc)gaim_prefs_disconnect_callback, NULL);

	conv_prefs_clear_all();
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

	vbox = gaim_gtk_make_frame (ret, _("Interface Font Sizes (points)"));

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

	if (!gtk_check_version(2, 4, 0)) {
		label = gtk_label_new_with_mnemonic(_("You must close and reopen any affected windows\nbesides the buddy list for font changes to take effect."));
	} else {
		label = gtk_label_new_with_mnemonic(_("You must close and reopen any affected windows\nfor font changes to take effect."));
	}
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	vbox = gaim_gtk_make_frame (ret, _("Conversations"));

	gaim_gtk_prefs_checkbox("Show _Add/Remove button in IMs and chats",
							pref_conv_show_add, vbox);

	gaim_gtk_prefs_checkbox("Show _Warn button in IMs",
							pref_conv_show_warn, vbox);

	gaim_gtk_prefs_checkbox("Show _Block button in IMs",
							pref_conv_show_block, vbox);

	gaim_gtk_prefs_checkbox("Show Send _File button in IMs",
							pref_conv_show_file, vbox);

	gaim_gtk_prefs_checkbox("Show I_nfo button in IMs",
							pref_conv_show_info, vbox);

	gaim_gtk_prefs_checkbox("Show _Invite button in chats",
							pref_conv_show_invite, vbox);

	gaim_gtk_prefs_checkbox("Show _join and part messages in chats",
							pref_conv_show_joinpart, vbox);

	vbox = gaim_gtk_make_frame (ret, _("Buddy List"));

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
	get_config_frame
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
	N_("Extended Preferences"),
	EP_VERSION,
	N_("Extended Gaim preferences not officially supported by Gaim."),
	N_("Provides several extended preferences for Gaim users.  These were omitted from Gaim for various reasons, but were popular requests from Gaim users.\n\nNote: This plugin is not supported by the official Gaim project.  Do not bug the developers about this plugin!"),
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
	gaim_prefs_add_bool(pref_conv_show_warn, TRUE);
	gaim_prefs_add_bool(pref_conv_show_block, TRUE);
	gaim_prefs_add_bool(pref_conv_show_file, TRUE);
	gaim_prefs_add_bool(pref_conv_show_add, TRUE);
	gaim_prefs_add_bool(pref_conv_show_info, TRUE);
	gaim_prefs_add_bool(pref_conv_show_invite, TRUE);
	gaim_prefs_add_bool(pref_conv_show_joinpart, TRUE);
	gaim_prefs_add_int(pref_conv_size, 8);
	gaim_prefs_add_int(pref_popup_size, 8);
	gaim_prefs_add_int(pref_log_size, 8);
	gaim_prefs_add_int(pref_blist_size, 8);
	gaim_prefs_add_bool(pref_blist_taskbar, TRUE);
	gaim_prefs_add_bool(pref_blist_allow_shrink, FALSE);
	gaim_prefs_add_bool(pref_blist_autohide, FALSE);
	gaim_prefs_add_bool(pref_blist_hidden, FALSE);

	if (gaim_prefs_exists(pref_conv_zoom)) {
		double zoom = 8 * 0.01 * gaim_prefs_get_int(pref_conv_zoom);
		gaim_prefs_set_int(pref_conv_size,  floor(zoom));
		gaim_prefs_set_int(pref_popup_size, floor(zoom));
		gaim_prefs_set_int(pref_log_size,   floor(zoom));
		gaim_prefs_set_int(pref_blist_size, floor(zoom));
		gaim_prefs_remove(pref_conv_zoom);
	}
}

GAIM_INIT_PLUGIN(convoptions, init_plugin, info)
