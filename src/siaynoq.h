/***
 * siaynoq -- a typically geeky shell replacement for Windows
 * Copyright (C) 2007 Neil Santos <neil_santos@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
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
#ifndef _SIAYNOQ_H_ /* include guard */
#define _SIAYNOQ_H_

#include <windows.h>

const char* SIAYNOQ_WND_CLASSNAME;
const char* SIAYNOQ_WND_TITLE;

BOOL siaynoq_init (HINSTANCE);
BOOL siaynoq_free ();

void siaynoq_run_reg_startup_items ();
void siaynoq_run_fs_startup_items ();
/* Intended to be called with CreateThread */
DWORD WINAPI siaynoq_run_startup_items (LPVOID);

/*** Main window class registration function
 *
 * For debug builds, responsible for registering the shell's desktop
 * window class with Windows.
 *
 * If compiled with debugging flags, returns a unique ID for the
 * registered class if successful; otherwise, returns zero.  If built
 * without said flags, this function will always return 1.
 *
 * See RegisterClassEx()
 */
ATOM siaynoq_wnd_regclass_statusbar (HINSTANCE);

/*** Main window creation function
 *
 * For debug builds, responsible for creating the shell's desktop
 * window.
 *
 * If compiled with debugging flags, returns the handle to the shell's
 * desktop window if successful; otherwise, or if built without said
 * flags, returns NULL.
 *
 * See CreateWindowEx()
 */
HWND siaynoq_wnd_create_statusbar (HINSTANCE, ATOM);

/*** Application-specific callback function for the main window
 *
 * Responsible for handling messages sent to the shell's desktop window.
 *
 * Returns result of the message processing which varies, depending on
 * the particular message handled.
 *
 * See WindowProc()
 */
LRESULT CALLBACK siaynoq_wnd_proc_statusbar (HWND, UINT, WPARAM, LPARAM);

HWND siaynoq_find_next_tiling_candidate(HWND[]);

/* Message-handling functions */
LRESULT siaynoq_msg_handler_sy_windowdestroyed (HWND, WPARAM, LPARAM);
LRESULT siaynoq_msg_handler_wm_paint (HWND, WPARAM, LPARAM);

#endif /* _SIAYNOQ_H_ */
