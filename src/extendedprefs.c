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

#include "notify.h"
#include "prefs.h"
#include "signals.h"
#include "version.h"

#include "gtkblist.h"
#include "gtkconv.h"
#include "gtkimhtml.h"
#include "gtkplugin.h"
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
static const char *pref_popup_size       = "/plugins/gtk/kstange/extendedprefs/popup_size";
static const char *pref_conv_size        = "/plugins/gtk/kstange/extendedprefs/conv_size";
static const char *pref_log_size         = "/plugins/gtk/kstange/extendedprefs/log_size";
static const char *pref_blist_size       = "/plugins/gtk/kstange/extendedprefs/blist_size";
static const char *pref_blist_allow_shrink	= "/plugins/gtk/kstange/extendedprefs/blist_allow_shrink";

static void
spin_pref_set_int(GtkWidget *w, void *data) {
	int value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(w));
	char *pref = (char *)data;
	gaim_prefs_set_int(pref, value);
}

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
size_prefs_update(GtkWidget *w, void *pref) {
	int value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(w));

	if (value < KSTANGE_EP_SIZE_MIN)
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), KSTANGE_EP_SIZE_MIN);
	else if (value > KSTANGE_EP_SIZE_MAX)
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), KSTANGE_EP_SIZE_MAX);
	else
		gaim_prefs_set_int((const char *)pref, value);

	if ((const char *)pref == pref_conv_size) {
		size_set("gaim_gtkconv_entry", value);
		size_set("gaim_gtkconv_imhtml", value);
		size_set("gaim_gtkprefs_font_imhtml", value);
	}
	else if ((const char *)pref == pref_popup_size) {
		size_set("gaim_gtkrequest_imhtml", value);
		size_set("gaim_gtknotify_imhtml", value);
	}
	else if ((const char *)pref == pref_log_size)
		size_set("gaim_gtklog_imhtml", value);
	else if ((const char *)pref == pref_blist_size)
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
conv_buttons_set_all(const char *pref, gboolean value) {
	GList *conv;

	for(conv = gaim_get_conversations(); conv != NULL; conv = conv->next)
		conv_buttons_set(conv->data, pref, value);
}

static void
conv_buttons_update(GtkWidget *w, void *data) {
	gboolean value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	const char *pref = (char *)data;

	gaim_prefs_set_bool(pref, value);
	conv_buttons_set_all(pref, value);
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
blist_taskbar_update() {
	GaimBuddyList *blist = gaim_get_blist();
	GaimGtkBuddyList *gtkblist;

	if (blist) {
		/* FIXME: In Win32, the taskbar entry won't come back till the window
		   is focused. */
	  	gtkblist = GAIM_GTK_BLIST(blist);

		if (!GTK_IS_WINDOW(gtkblist->window))
			return;

		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(gtkblist->window),
										 !gaim_prefs_get_bool(pref_blist_taskbar));
	}
}

static void
blist_taskbar_set(GtkWidget *w) {
	gaim_prefs_set_bool(pref_blist_taskbar,
						gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
	blist_taskbar_update();
}

static void
blist_shrink_update() {
	GaimBuddyList *blist = gaim_get_blist();

	if (blist) {
		GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(blist);

		if (!GTK_IS_WINDOW(gtkblist->window))
			return;

		GTK_WINDOW(gtkblist->window)->allow_shrink =
			gaim_prefs_get_bool(pref_blist_allow_shrink);

	}
}

static void
blist_shrink_set(GtkWidget *w) {
	gaim_prefs_set_bool(pref_blist_allow_shrink,
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
	blist_shrink_update();
}

static void
blist_created_cb(GaimBuddyList *blist, void *data) {
	blist_taskbar_update();
	blist_shrink_update();
}

gboolean plugin_load(GaimPlugin *plugin) {
	GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(gaim_get_blist());

	gaim_signal_connect((void*)gaim_conversations_get_handle(),
						"conversation-created", plugin,
						GAIM_CALLBACK(conv_prefs_init), NULL);

	if (gtkblist != NULL && GTK_IS_WINDOW(gtkblist->window)) {
		blist_created_cb(gaim_get_blist(), NULL);
	}

	gaim_signal_connect(gaim_gtk_blist_get_handle(), "gtkblist-created", plugin, GAIM_CALLBACK(blist_created_cb), NULL);

	conv_prefs_init_all();
	size_prefs_init_all();

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

	conv_prefs_clear_all();
	size_prefs_clear_all();

	return TRUE;
}

static GtkWidget* get_config_frame(GaimPlugin *plugin) {
	GtkWidget *ret;
	GtkWidget *spinner, *button;
	GtkWidget *vbox, *hbox;
	GtkWidget *label;
	GtkAdjustment *adj;
	GtkSizeGroup *sg;

	ret = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width (GTK_CONTAINER (ret), 12);

	vbox = gaim_gtk_make_frame (ret, _("Interface Font Sizes (points)"));

	/* Conversations */
	hbox = gtk_hbox_new(FALSE, 5);

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	label = gtk_label_new_with_mnemonic(_("_Conversations:"));
	gtk_size_group_add_widget(sg, label);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);

	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	adj = (GtkAdjustment *)gtk_adjustment_new(gaim_prefs_get_int(pref_conv_size), KSTANGE_EP_SIZE_MIN, KSTANGE_EP_SIZE_MAX, 1, 1, 0);
	spinner = gtk_spin_button_new(adj, 0, 0);
	gtk_signal_connect(GTK_OBJECT(spinner), "value-changed", GTK_SIGNAL_FUNC(size_prefs_update), (void *)pref_conv_size);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), spinner);

	gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* Log Viewer Size */
	hbox = gtk_hbox_new(FALSE, 5);

	label = gtk_label_new_with_mnemonic(_("Log _Viewer:"));
	gtk_size_group_add_widget(sg, label);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);

	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	adj = (GtkAdjustment *)gtk_adjustment_new(gaim_prefs_get_int(pref_log_size), KSTANGE_EP_SIZE_MIN, KSTANGE_EP_SIZE_MAX, 1, 1, 0);
	spinner = gtk_spin_button_new(adj, 0, 0);
	gtk_signal_connect(GTK_OBJECT(spinner), "value-changed", GTK_SIGNAL_FUNC(size_prefs_update), (void *)pref_log_size);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), spinner);

	gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* Popup Dialogs */
	hbox = gtk_hbox_new(FALSE, 5);

	label = gtk_label_new_with_mnemonic(_("Information _Dialogs:"));
	gtk_size_group_add_widget(sg, label);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);

	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	adj = (GtkAdjustment *)gtk_adjustment_new(gaim_prefs_get_int(pref_popup_size), KSTANGE_EP_SIZE_MIN, KSTANGE_EP_SIZE_MAX, 1, 1, 0);
	spinner = gtk_spin_button_new(adj, 0, 0);
	gtk_signal_connect(GTK_OBJECT(spinner), "value-changed", GTK_SIGNAL_FUNC(size_prefs_update), (void *)pref_popup_size);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), spinner);

	gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* Buddy List Size */
	hbox = gtk_hbox_new(FALSE, 5);

	label = gtk_label_new_with_mnemonic(_("Bu_ddy List:"));
	gtk_size_group_add_widget(sg, label);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);

	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	adj = (GtkAdjustment *)gtk_adjustment_new(gaim_prefs_get_int(pref_blist_size), KSTANGE_EP_SIZE_MIN, KSTANGE_EP_SIZE_MAX, 1, 1, 0);
	spinner = gtk_spin_button_new(adj, 0, 0);
	gtk_signal_connect(GTK_OBJECT(spinner), "value-changed", GTK_SIGNAL_FUNC(size_prefs_update), (void *)pref_blist_size);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), spinner);

	gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	if (!gtk_check_version(2, 4, 0)) {
		label = gtk_label_new_with_mnemonic(_("You must close and reopen any affected windows\nbesides the buddy list for font changes to take effect."));
	} else {
		label = gtk_label_new_with_mnemonic(_("You must close and reopen any affected windows\nfor font changes to take effect."));
	}
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	vbox = gaim_gtk_make_frame (ret, _("Conversations"));

	button = gtk_check_button_new_with_mnemonic("Show _Add/Remove button in IMs and chats");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), gaim_prefs_get_bool(pref_conv_show_add));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(conv_buttons_update), (void *)pref_conv_show_add);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_check_button_new_with_mnemonic("Show _Warn button in IMs");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), gaim_prefs_get_bool(pref_conv_show_warn));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(conv_buttons_update), (void *)pref_conv_show_warn);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_check_button_new_with_mnemonic("Show _Block button in IMs");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), gaim_prefs_get_bool(pref_conv_show_block));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(conv_buttons_update), (void *)pref_conv_show_block);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_check_button_new_with_mnemonic("Show _Send File button in IMs");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), gaim_prefs_get_bool(pref_conv_show_file));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(conv_buttons_update), (void *)pref_conv_show_file);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_check_button_new_with_mnemonic("Show I_nfo button in IMs");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), gaim_prefs_get_bool(pref_conv_show_info));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(conv_buttons_update), (void *)pref_conv_show_info);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_check_button_new_with_mnemonic("Show _Invite button in chats");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), gaim_prefs_get_bool(pref_conv_show_invite));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(conv_buttons_update), (void *)pref_conv_show_invite);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	vbox = gaim_gtk_make_frame (ret, _("Buddy List"));

	/* Tooltip Delay */
	hbox = gtk_hbox_new(FALSE, 5);

	label = gtk_label_new_with_mnemonic(_("_Tooltip reveal delay (ms):"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	adj = (GtkAdjustment *)gtk_adjustment_new(gaim_prefs_get_int(pref_tooltip_delay), KSTANGE_EP_BLIST_TIP_MIN, KSTANGE_EP_BLIST_TIP_MAX, 100, 100, 0);
	spinner = gtk_spin_button_new(adj, 0, 0);
	gtk_signal_connect(GTK_OBJECT(spinner), "value-changed", GTK_SIGNAL_FUNC(spin_pref_set_int), (void *)pref_tooltip_delay);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), spinner);

	gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	button = gtk_check_button_new_with_mnemonic("Show buddy _list entry in taskbar");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), gaim_prefs_get_bool(pref_blist_taskbar));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(blist_taskbar_set), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_check_button_new_with_mnemonic("Allow buddy list to s_hrink");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), gaim_prefs_get_bool(pref_blist_allow_shrink));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(blist_shrink_set), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

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
	gaim_prefs_add_bool(pref_blist_taskbar, TRUE);
	gaim_prefs_add_none("/plugins/gtk/kstange/extendedprefs/conv_buttons");
	gaim_prefs_add_bool(pref_conv_show_warn, TRUE);
	gaim_prefs_add_bool(pref_conv_show_block, TRUE);
	gaim_prefs_add_bool(pref_conv_show_file, TRUE);
	gaim_prefs_add_bool(pref_conv_show_add, TRUE);
	gaim_prefs_add_bool(pref_conv_show_info, TRUE);
	gaim_prefs_add_bool(pref_conv_show_invite, TRUE);
	gaim_prefs_add_int(pref_conv_size, 8);
	gaim_prefs_add_int(pref_popup_size, 8);
	gaim_prefs_add_int(pref_log_size, 8);
	gaim_prefs_add_int(pref_blist_size, 8);
	gaim_prefs_add_bool(pref_blist_allow_shrink, FALSE);

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
