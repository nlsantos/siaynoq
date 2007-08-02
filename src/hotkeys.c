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
#include "hotkeys.h"
#include "config.h"
#include "tiling.h"

typedef struct
{
  SIAYNOQHOTKEYFUNCTION func;
  LPTSTR name;
} SIAYNOQHOTKEYFUNCNAMEPAIR;

const UINT SIAYNOQ_HOTKEY_FUNCTION_NAME_PAIR_COUNT = 7;
const SIAYNOQHOTKEYFUNCNAMEPAIR siaynoq_hotkey_function_name_pairs[] = {
  { load_hooks_dll, "siaynoq_load_hooks_dll" },
  { unload_hooks_dll, "siaynoq_unload_hooks_dll" },
  { exit_shell, "siaynoq_hotkeys_exit_shell" },
  { switch_users, "siaynoq_hotkeys_switch_users" },
  { logoff_user, "siaynoq_hotkeys_logoff_user" },
  { put_focused_window_on_track, "siaynoq_hotkeys_put_focused_window_on_track" },
  { zoom_focused_window, "siaynoq_hotkeys_zoom_focused_window" },
};

static HANDLE siaynoq_lib_handle_hook = NULL;
static HWND siaynoq_main_wnd_handle = NULL;

LPTSTR
siaynoq_hotkeys_name_by_func_ptr (SIAYNOQHOTKEYFUNCTION func_ptr)
{
  UINT idx;

  for (idx = 0; idx < SIAYNOQ_HOTKEY_FUNCTION_NAME_PAIR_COUNT; idx++)
    {
      SIAYNOQHOTKEYFUNCNAMEPAIR func_name_pair = siaynoq_hotkey_function_name_pairs[idx];

      if (func_name_pair.func == func_ptr)
        return func_name_pair.name;
    }

  return NULL;
}

SIAYNOQHOTKEYFUNCTION
siaynoq_hotkeys_func_ptr_by_name (LPTSTR func_name)
{
  UINT idx;

  for (idx = 0; idx < SIAYNOQ_HOTKEY_FUNCTION_NAME_PAIR_COUNT; idx++)
    {
      SIAYNOQHOTKEYFUNCNAMEPAIR func_name_pair = siaynoq_hotkey_function_name_pairs[idx];

      if (0 == lstrcmpi (func_name_pair.name, func_name))
        return func_name_pair.func;
    }

  return NULL;
}

BOOL
siaynoq_hotkeys_init (HWND main_wnd_handle, HANDLE hook_lib_handle)
{
  BOOL retval;

  assert (NULL != main_wnd_handle);
  assert (NULL != hook_lib_handle);

  retval = TRUE;

  if (hotkeys != NULL)
    {
      UINT hotkey_count = sizeof (hotkeys) / sizeof (hotkeys[0]);
      UINT idx;

      siaynoq_lib_handle_hook = hook_lib_handle;
      siaynoq_main_wnd_handle = main_wnd_handle;

      for (idx = 0; idx < hotkey_count; idx++)
        {
          SIAYNOQHOTKEY hotkey = hotkeys[idx];
          LPTSTR func_name = siaynoq_hotkeys_name_by_func_ptr (hotkey.func_ptr);

          if (NULL != func_name)
            {
              ATOM hotkey_atom = GlobalAddAtom (func_name);

              if (0 == hotkey_atom)
                {
                  retval &= FALSE;
                  debug_output ("GlobalAddAtom() failed");
                }
              else if (!(RegisterHotKey (main_wnd_handle, hotkey_atom, hotkey.mod, hotkey.keycode)))
                {
                  retval &= FALSE;
                  debug_output_ex ("RegisterHotKey() failed: %s :: %u", func_name, hotkey.keycode);
                }
            }
          else
            debug_output ("siaynoq_hotkeys_func_ptr_by_name returned NULL");
        }
    }

  return retval;
}

BOOL
siaynoq_hotkeys_free ()
{
  BOOL retval;

  retval = TRUE;
  if (hotkeys != NULL)
    {
      UINT hotkey_count = sizeof (hotkeys) / sizeof (hotkeys[0]);
      UINT idx;

      for (idx = 0; idx < hotkey_count; idx++)
        {
          SIAYNOQHOTKEY hotkey = hotkeys[idx];
          ATOM hotkey_atom = GlobalFindAtom (siaynoq_hotkeys_name_by_func_ptr (hotkey.func_ptr));

          if (0 == hotkey_atom)
            {
              retval &= FALSE;
              debug_output ("GlobalFindAtom() failed");
            }
          else if (!(UnregisterHotKey (siaynoq_main_wnd_handle, hotkey_atom)))
            {
              retval &= FALSE;
              debug_output ("UnregisterHotKey() failed");
            }
        }
    }
  return retval;
}

SIAYNOQHOTKEY*
siaynoq_hotkey_by_func_ptr (SIAYNOQHOTKEYFUNCTION func_ptr)
{
  UINT hotkey_count = sizeof (hotkeys) / sizeof (hotkeys[0]);
  UINT idx;

  assert (NULL != func_ptr);

  for (idx = 0; idx < hotkey_count; idx++)
    {
      SIAYNOQHOTKEY hotkey = hotkeys[idx];

      if (func_ptr == hotkey.func_ptr)
        return &(hotkeys[idx]);
    }

  return NULL;
}

SIAYNOQHOTKEY*
siaynoq_hotkey_by_name (LPTSTR func_name)
{
  SIAYNOQHOTKEYFUNCTION func_ptr = siaynoq_hotkeys_func_ptr_by_name (func_name);

  assert (NULL != func_name);

  if (NULL == func_ptr)
    debug_output ("got a NULL func ptr from a func name");
  else
    return siaynoq_hotkey_by_func_ptr (func_ptr);

  return NULL;
}

void
load_hooks_dll (SIAYNOQHOTKEYARG ignore)
{
  if (DEBUG)
    {
      assert (NULL != siaynoq_main_wnd_handle);
      hooks_dll_load (siaynoq_main_wnd_handle);
    }
}

void
unload_hooks_dll (SIAYNOQHOTKEYARG ignore)
{
  if (DEBUG)
    hooks_dll_unload ();
}

void
exit_shell (SIAYNOQHOTKEYARG ignore)
{
  PostMessage (siaynoq_main_wnd_handle, WM_CLOSE, 0, 0);
}

void
switch_users (SIAYNOQHOTKEYARG ignore)
{
  /* At some point, I've got to include checks for the OS version before
     triggering this... */
  LockWorkStation ();
}

void
logoff_user (SIAYNOQHOTKEYARG ignore)
{
  ExitWindowsEx (EWX_LOGOFF, 0);
}

void
put_focused_window_on_track (SIAYNOQHOTKEYARG ignore)
{
  siaynoq_set_wnd_handle_on_track (GetForegroundWindow (), FALSE, NULL);
}

void
zoom_focused_window (SIAYNOQHOTKEYARG ignore)
{
}
