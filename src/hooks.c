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
#include "hooks.h"
#include "tools.h"
#include "config.h"
#include "tiling.h"

typedef struct
{
  HHOOK shellproc;
} SIAYNOQSHAREDMEMHH;

static HWND *siaynoq_shared_main_wnd_handle = NULL;
static HANDLE siaynoq_file_mapping_obj = NULL;

static SIAYNOQSHAREDMEMHH *siaynoq_shared_mem_hh = NULL;
static HANDLE siaynoq_file_mapping_obj_hh = NULL;

static UINT SY_WINDOWACTIVATED;
static UINT SY_WINDOWDESTROYED;


BOOL WINAPI
DllMain (HINSTANCE instance, DWORD reason, LPVOID reserved)
{
  BOOL retval;

  retval = TRUE;

  switch (reason)
    {
    case DLL_PROCESS_ATTACH:
      siaynoq_shared_main_wnd_handle = (HWND*) shared_mem_struct_init (siaynoq_file_mapping_obj,
                                                                       "SIAYNOQ_SHARED_MAIN_WND_HANDLE",
                                                                       sizeof (HWND));
      if (NULL == siaynoq_shared_main_wnd_handle)
        {
          debug_output ("!!! Failed to initialize shared main window handle");
          return FALSE;
        }

      SY_WINDOWACTIVATED = RegisterWindowMessage (SIAYNOQ_MSG_APP_FOCUS_CHANGE);
      retval &= (0 != SY_WINDOWACTIVATED);

      if (!retval)
        debug_output ("!!! RegisterWindowMessage() returned 0 for SY_WINDOWACTIVATED");

      SY_WINDOWDESTROYED = RegisterWindowMessage (SIAYNOQ_MSG_APP_LOSING_WINDOW);
      retval &= (0 != SY_WINDOWDESTROYED);

      if (!retval)
        debug_output ("!!! RegisterWindowMessage() returned 0 for SY_WINDOWACTIVATED");

      if (DEBUG)
        assert (retval);
      break;

    case DLL_PROCESS_DETACH:
      shared_mem_struct_free ((LPVOID) siaynoq_shared_main_wnd_handle,
                              siaynoq_file_mapping_obj);
      break;
    }

  return retval;
}


LRESULT CALLBACK
siaynoq_hook_shellproc (int code, WPARAM wParam, LPARAM lParam)
{
  HWND target_wnd_handle;
  BOOL is_target_wnd_handle_fullscreen;

  if (0 <= code)
    {
      debug_output ("@@@ Here we go...");
      debug_output_ex ("--- %u shellproc code\n", code);
      debug_output_ex ("--- %u HSHELL_WINDOWACTIVATED\n", HSHELL_WINDOWACTIVATED);
      debug_output_ex ("--- %u HSHELL_WINDOWDESTROYED\n", HSHELL_WINDOWDESTROYED);

      if (HSHELL_WINDOWACTIVATED == code)
        {
          debug_output ("@@@ HSHELL_WINDOWACTIVATED");

          target_wnd_handle = (HWND) wParam;
          is_target_wnd_handle_fullscreen = (BOOL) lParam;

          if ((NULL != target_wnd_handle)
              && (!is_target_wnd_handle_fullscreen)
              && (HIBYTE (GetAsyncKeyState (MAIN_MOD_KEY)) != 128) /* Specified mod key musn't be pressed */
              && (*siaynoq_shared_main_wnd_handle != target_wnd_handle))
            {
              debug_output ("!!! Focus changed; mod key isn't pressed; window isn't full screen");
              PostMessage (*siaynoq_shared_main_wnd_handle, SY_WINDOWACTIVATED, wParam, (LPARAM) NULL);
            }
          else
            debug_output ("!!! Focus changed; mod key is pressed or window is full screen");
        } /* HSHELL_WINDOWACTIVATED */
      else if (HSHELL_WINDOWDESTROYED == code)
        {
          debug_output ("@@@ HSHELL_WINDOWDESTROYED");
          PostMessage (*siaynoq_shared_main_wnd_handle, SY_WINDOWDESTROYED, wParam, (LPARAM) NULL);
        } /* HSHELL_WINDOWDESTROYED */

      return 0;
    }

  return CallNextHookEx (NULL, code, wParam, lParam);
}


BOOL DLL_CONVENTION
siaynoq_hooks_init (HWND shell_wnd_handle)
{
  BOOL retval;

  retval = TRUE;

  if (*siaynoq_shared_main_wnd_handle != shell_wnd_handle)
    *siaynoq_shared_main_wnd_handle = shell_wnd_handle;

  siaynoq_shared_mem_hh = shared_mem_struct_init (siaynoq_file_mapping_obj_hh,
                                                  "SIAYNOQ_SHARED_MEM_HH",
                                                  sizeof (SIAYNOQSHAREDMEMHH));
  if (NULL == siaynoq_shared_mem_hh)
    {
      debug_output ("!!! Failed in mapping view of shared memory.");
      retval &= FALSE;
    }
  else
    {
      siaynoq_shared_mem_hh->shellproc = SetWindowsHookEx (WH_SHELL,
                                                           siaynoq_hook_shellproc,
                                                           GetModuleHandle ("wafer.dll"),
                                                           0);
      retval &= (NULL != siaynoq_shared_mem_hh->shellproc);

      if (NULL != siaynoq_shared_mem_hh->shellproc)
        debug_output ("!!! Shell hook installed");
      else
        debug_output ("!!! Failed in installing shell hook.");
    }

  return retval;
}


BOOL DLL_CONVENTION
siaynoq_hooks_free ()
{
  BOOL retval;

  retval = TRUE;

  if (NULL != siaynoq_shared_mem_hh->shellproc)
    retval &= UnhookWindowsHookEx (siaynoq_shared_mem_hh->shellproc);

  if (!(shared_mem_struct_free (siaynoq_shared_mem_hh, siaynoq_file_mapping_obj_hh)))
    debug_output ("!!! Failed in freeing hook handles shared mem struct");

  return retval;
}
