/*
 * ct_main_win_vte.cc
 *
 * Copyright 2009-2022
 * Giuseppe Penone <giuspen@gmail.com>
 * Evgenii Gurianov <https://github.com/txe>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "ct_main_win.h"
#if defined(HAVE_VTE)
#include <vte/vte.h>
#endif // HAVE_VTE

#if defined(HAVE_VTE)
static void _vteTerminalSpawnAsyncCallback(VteTerminal*/*terminal*/,
                                           GPid pid,
                                           GError* error,
                                           gpointer/*user_data*/)
{
    if (-1 != pid) {
        spdlog::debug("+VTE");
    }
    else {
        spdlog::error("!! VTE");
    }
    if (NULL != error) {
        spdlog::error("{}", error->message);
        g_clear_error(&error);
    }
}
#endif // HAVE_VTE

void CtMainWin::show_hide_vte(bool visible)
{
#if defined(HAVE_VTE)
    if (not _pVte) {
        GtkWidget* pTermWidget = vte_terminal_new();
        _pVte = Gtk::manage(Glib::wrap(pTermWidget));
        vte_terminal_set_scrollback_lines(VTE_TERMINAL(pTermWidget), -1/*infinite*/);
        update_vte_settings();

        char* startterm[2] = {(char*)"/bin/sh", 0};
        vte_terminal_spawn_async(VTE_TERMINAL(pTermWidget),
                                 VTE_PTY_DEFAULT,
                                 NULL/*working_directory*/,
                                 startterm/*argv*/,
                                 NULL/*envv*/,
                                 G_SPAWN_DEFAULT/*spawn_flags_*/,
                                 NULL/*child_setup*/,
                                 NULL/*child_setup_data*/,
                                 NULL/*child_setup_data_destroy*/,
                                 -1/*timeout*/,
                                 NULL/*cancellable*/,
                                 &_vteTerminalSpawnAsyncCallback,
                                 NULL/*user_data*/);

        auto button_copy = Gtk::manage(new Gtk::Button{});
        button_copy->set_image(*new_managed_image_from_stock("ct_edit_copy", Gtk::ICON_SIZE_MENU));
        button_copy->set_tooltip_text(_("Copy Selection or All"));

        auto button_paste = Gtk::manage(new Gtk::Button{});
        button_paste->set_image(*new_managed_image_from_stock("ct_edit_paste", Gtk::ICON_SIZE_MENU));
        button_paste->set_tooltip_text(_("Paste"));

        auto button_clear = Gtk::manage(new Gtk::Button{});
        button_clear->set_image(*new_managed_image_from_stock("ct_clear", Gtk::ICON_SIZE_MENU));
        button_clear->set_tooltip_text(_("Clear All"));

        button_copy->signal_clicked().connect([pTermWidget](){
            if (not vte_terminal_get_has_selection(VTE_TERMINAL(pTermWidget))) {
                vte_terminal_select_all(VTE_TERMINAL(pTermWidget));
            }
            vte_terminal_copy_clipboard_format(VTE_TERMINAL(pTermWidget), VTE_FORMAT_TEXT);
        });
        button_paste->signal_clicked().connect([pTermWidget](){
            vte_terminal_paste_clipboard(VTE_TERMINAL(pTermWidget));
        });
        button_clear->signal_clicked().connect([pTermWidget](){
            vte_terminal_reset(VTE_TERMINAL(pTermWidget), true/*clear_tabstops*/, true/*clear_history*/);
            vte_terminal_feed_child(VTE_TERMINAL(pTermWidget), "\n", 1);
        });

        auto pButtonsBox = Gtk::manage(new Gtk::Box{Gtk::ORIENTATION_VERTICAL, 0/*spacing*/});
        pButtonsBox->pack_start(*button_copy, false, false);
        pButtonsBox->pack_start(*button_paste, false, false);
        pButtonsBox->pack_start(*button_clear, false, false);

        _hBoxVte.pack_start(*pButtonsBox, false, false);
        _hBoxVte.pack_start(*_pVte);
        if (GTK_IS_SCROLLABLE(pTermWidget)) {
            GtkWidget* pScrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL,
                    gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(pTermWidget)));
            Gtk::Widget* pGtkmmScrollbar = Gtk::manage(Glib::wrap(pScrollbar));
            _hBoxVte.pack_start(*pGtkmmScrollbar, false, false);
        }
        _hBoxVte.show_all();
    }
#endif // HAVE_VTE
    _hBoxVte.property_visible() = visible;
}

void CtMainWin::update_vte_settings()
{
#if defined(HAVE_VTE)
    vte_terminal_set_font(VTE_TERMINAL(_pVte->gobj()),
                          Pango::FontDescription{_pCtConfig->vteFont}.gobj());
#endif // HAVE_VTE
}

void CtMainWin::exec_in_vte(const std::string& shell_cmd)
{
    show_hide_vte(true/*visible*/);
#if defined(HAVE_VTE)
    vte_terminal_feed_child(VTE_TERMINAL(_pVte->gobj()), shell_cmd.c_str(), shell_cmd.size());
#else // !HAVE_VTE
    spdlog::warn("!! noVte {}", shell_cmd);
#endif // !HAVE_VTE
}
