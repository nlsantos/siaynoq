/***
 * siaynoq -- a typically geeky shell replacement for Windows
 * Copyright (C) 2007 Neil Santos <neil_santos@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 ***/
#ifndef _TILING_H_ // include guard
#define _TILING_H_

#include <windows.h>

#define SIAYNOQ_MSG_APP_FOCUS_CHANGE "siaynoq_msg_app_focus_change"
#define SIAYNOQ_MSG_APP_LOSING_WINDOW "siaynoq_msg_app_losing_window"

// Yes, these are global variables.  No, I am not ashamed. :P
HWND siaynoq_next_maximized_wnd_handle;
HWND siaynoq_curr_maximized_wnd_handle;
HWND siaynoq_prev_maximized_wnd_handle;
HWND siaynoq_marked_for_destruction_wnd_handle;
UINT siaynoq_active_window_count;
BOOL siaynoq_is_in_tiled_mode;

BOOL siaynoq_tiling_init (HWND);

RECT siaynoq_calc_wnd_on_track_dimension ();
HWND siaynoq_set_wnd_handle_on_track (HWND, BOOL, LPCREATESTRUCT);

UINT siaynoq_tile_non_focused_wnd ();

/***
 * Set the handle of the window to be maximized before calling this
 */
BOOL siaynoq_is_new_wnd_tileable (HWND, DWORD, DWORD);
BOOL siaynoq_is_target_wnd_tileable (HWND);

#endif
