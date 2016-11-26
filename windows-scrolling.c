/**
 *   Pidgin Win32 Smooth Scrolling
 *   Allows fine-grain control of scrolling in Pidgin, eg when using a trackpad
 *   Copyright (C) 2016  Eion Robb
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define PURPLE_PLUGINS

#include "pidgin.h"

#include "gtkconv.h"
#include "gtkplugin.h"
#include "gtkimhtml.h"

#include "purple.h"

#include <windows.h>
#include <gdk/gdkwin32.h>


#ifndef WM_MOUSEWHEEL
#	define WM_MOUSEWHEEL 0x020A
#endif

static HHOOK hhHook = NULL;

static LRESULT CALLBACK 
win32_scroll_event_handler(int nCode, WPARAM wparam, LPARAM lparam)
{
	if(nCode == HC_ACTION) {
		if (wparam == WM_MOUSEWHEEL) {
			MSLLHOOKSTRUCT *pdata = (MSLLHOOKSTRUCT *) lparam;
			short scroll_delta = (short) HIWORD(pdata->mouseData);
			HWND hwnd;

			if ((hwnd = WindowFromPoint(pdata->pt)) != NULL)
			{
				GdkWindow *new_window = gdk_win32_handle_table_lookup((GdkNativeWindow) hwnd);
				gpointer userdata = NULL;
				GtkWidget *widget = NULL;
				GtkAdjustment *adj = NULL;
				
				if (new_window != NULL) {
					gdk_window_get_user_data(new_window, &userdata);
					if (userdata != NULL) {
						widget = userdata;
						
						if (GTK_IS_TEXT_VIEW(widget)) {
							GtkTextView *text_view = GTK_TEXT_VIEW(widget);
							adj = text_view->vadjustment;
						} else if (GTK_IS_SCROLLED_WINDOW(widget)) {
							GtkScrolledWindow *scrolled_window = GTK_SCROLLED_WINDOW(widget);
							adj = gtk_scrolled_window_get_vadjustment(scrolled_window);
						} else if (GTK_IS_TREE_VIEW(widget)) {
							GtkTreeView *tree_view = GTK_TREE_VIEW(widget);
							adj = gtk_tree_view_get_vadjustment(tree_view);
						} else {
							g_object_get(widget, "vadjustment", &adj);
							g_object_unref(adj);
						}
					}
				}
				if (adj != NULL) {
					
					// Try to compensate for the scrolling on the widget
					gdouble diff = adj->step_increment * ((((double)ABS(scroll_delta)) - 120.0) / 120.0);
					
					// Don't use gtk_adjustment_set_value() as this causes a redraw and makes things look messy
					adj->value = adj->value - diff * (scroll_delta < 0 ? -1 : 1);
					return 0;
				}
			}
		}
	}
		
	return CallNextHookEx( hhHook, nCode, wparam, lparam );
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	WNDCLASSEXW wcex;
	DWORD lasterror;
	GList *list;
	
	hhHook = SetWindowsHookEx(WH_MOUSE_LL, win32_scroll_event_handler, winpidgin_exe_hinstance(), 0);
	lasterror = GetLastError();
	if (lasterror)
		purple_debug_error("win7", "SetWindowsHookEx error %d\n", (int) lasterror);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	UnhookWindowsHookEx(hhHook);

	return TRUE;
}

#ifndef N_
#define N_(a) (a)
#endif

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,                           /**< major version */
	PURPLE_MINOR_VERSION,                           /**< minor version */
	PURPLE_PLUGIN_STANDARD,                         /**< type */
	PIDGIN_PLUGIN_TYPE,                             /**< ui_requirement */
	0,                                              /**< flags */
	NULL,                                           /**< dependencies */
	PURPLE_PRIORITY_DEFAULT,                        /**< priority */

	"gtk-win32-scrolling",                                /**< id */
	N_("Windows Smooth Scrolling"),                              /**< name */
	"0.1",                                /**< version */
	N_("Windows Smooth Scrolling."),         /**< summary */
	N_("Allows smooth scrolling with trackpads on Windows."),    /**< description */
	"",              /**< author */
	"",                                 /**< homepage */
	plugin_load,                                    /**< load */
	plugin_unload,                                  /**< unload */
	NULL,                                           /**< destroy */
	NULL,                                           /**< ui_info */
	NULL,                                           /**< extra_info */
	NULL,                                           /**< prefs_info */
	NULL,                                           /**< actions */

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
}

PURPLE_INIT_PLUGIN(win32scroll, init_plugin, info)
