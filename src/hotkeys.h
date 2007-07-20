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
#ifndef _HOTKEYS_H_ /* include guard */
#define _HOTKEYS_H_

#include <windows.h>

typedef union
{
  LPTSTR cmd;
  UINT i;
} SIAYNOQHOTKEYARG, *SIAYNOQHOTKEYARG_PTR;

typedef void(*SIAYNOQHOTKEYFUNCTION)(SIAYNOQHOTKEYARG);

typedef struct
{
  UINT mod;
  UINT keycode;
  SIAYNOQHOTKEYFUNCTION func_ptr;
  SIAYNOQHOTKEYARG arg;
} SIAYNOQHOTKEY;

/* Valid hotkey function names */
void exit_shell (SIAYNOQHOTKEYARG);
void switch_users (SIAYNOQHOTKEYARG);
void logoff_user (SIAYNOQHOTKEYARG);
void put_focused_window_on_track (SIAYNOQHOTKEYARG);
void zoom_focused_window (SIAYNOQHOTKEYARG);

/* Valid hotkey functions, but only if DEBUG is defined on compile */
void load_hooks_dll (SIAYNOQHOTKEYARG);
void unload_hooks_dll (SIAYNOQHOTKEYARG);

BOOL siaynoq_hotkeys_init (HWND, HANDLE);
BOOL siaynoq_hotkeys_free ();

SIAYNOQHOTKEY* siaynoq_hotkey_by_func_ptr (SIAYNOQHOTKEYFUNCTION);
SIAYNOQHOTKEY* siaynoq_hotkey_by_name (LPTSTR);

#include "tools.h"

#endif /* _HOTKEYS_H_ */
