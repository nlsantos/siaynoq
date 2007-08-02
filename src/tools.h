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
#ifndef _TOOLS_H_ /* include guard */
#define _TOOLS_H_

#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <share.h>

#ifndef _HOTKEYS_H_
typedef struct
{
  HWND curr_maximized_wnd_handle;
  HWND prev_maximized_wnd_handle;
  HWND marked_for_maximizing_wnd_handle;
  HWND marked_for_destruction_wnd_handle;
  UINT active_windows;
  BOOL is_in_tiled_mode;
} SIAYNOQSHAREDMEM;

BOOL adjust_process_privilege ();

LPVOID shared_mem_struct_init (HANDLE, LPCTSTR, const size_t);
BOOL shared_mem_struct_free (LPVOID, HANDLE);
#endif

void debug_output (LPCTSTR);
void debug_output_ex (LPCTSTR, ...);

LPBYTE reg_get_value (LPCTSTR, LPDWORD, const LPDWORD, HKEY, LPTSTR);
BOOL reg_set_value (LPTSTR, LPVOID, DWORD, DWORD, HKEY, LPTSTR, BOOL, BOOL);
DWORD reg_get_value_info (HKEY, LPDWORD, LPDWORD);

int system_spawn (LPTSTR, LPCTSTR, LPCTSTR, BOOL, LPCTSTR);

#ifndef _HOOKS_H_
HANDLE hooks_dll_load (HWND);
BOOL hooks_dll_unload ();
#endif

#endif /* _TOOLS_H */
