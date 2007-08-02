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
#include <time.h>

#include "siaynoq.h"
#include "config.h"
#include "tiling.h"
#include "tools.h"
#include "hotkeys.h"

#if (_WIN32_WINNT >= 0x0501)
#  include <wtsapi32.h>
#endif

#include <shellapi.h>

/* Items needed from shlobj.h */
#define CSIDL_COMMON_STARTUP	24
#define CSIDL_COMMON_ALTSTARTUP	30
#define CSIDL_STARTUP		7
#define CSIDL_ALTSTARTUP		29

BOOL WINAPI SHGetSpecialFolderPathA (HWND,LPSTR,int,BOOL);
BOOL WINAPI SHGetSpecialFolderPathW (HWND,LPWSTR,int,BOOL);

#ifdef UNICODE
#  define SHGetSpecialFolderPath SHGetSpecialFolderPathW
#else
#  define SHGetSpecialFolderPath SHGetSpecialFolderPathA
#endif
/* - Items needed from shlobj.h */

const char* SIAYNOQ_WND_CLASSNAME = "SiaynoqWndClass";
const char* SIAYNOQ_WND_TITLE = "[dpi] Siaynoq";
const UINT_PTR SIAYNOQ_TIME_DATE_DISPLAY_TIMER_EVENT_ID = WM_USER + 1;
const UINT_PTR SIAYNOQ_WINDOW_ACTIVATED_TIMER_EVENT_ID = WM_USER + 2;

static HWND siaynoq_main_wnd_handle = NULL;
static HANDLE siaynoq_lib_handle_hook = NULL;
static UINT_PTR siaynoq_time_date_display_timer = 0;
static UINT_PTR siaynoq_window_activated_timer = 0;
static UINT siaynoq_statusbar_height =  0;

static UINT SY_WINDOWACTIVATED;
static UINT SY_WINDOWDESTROYED;

BOOL
siaynoq_init (HINSTANCE instance)
{
  BOOL retval;
  BOOL last_stat;
  ATOM wnd_class_atom;

  retval = TRUE;

  retval &= adjust_process_privilege ();

  siaynoq_statusbar_height = FONT_SIZE + 6;

  if ((wnd_class_atom = siaynoq_wnd_regclass_statusbar (instance)))
    siaynoq_main_wnd_handle = siaynoq_wnd_create_statusbar (instance, wnd_class_atom);

  if (!(wnd_class_atom))
    {
      debug_output ("!!! Window class registration failed; function returned 0.  Aborting.");
      return FALSE;
    }

  /* Load hooks library here */
  siaynoq_lib_handle_hook = hooks_dll_load (siaynoq_main_wnd_handle);
  retval &= (NULL != siaynoq_lib_handle_hook);

  if (!(siaynoq_hotkeys_init (siaynoq_main_wnd_handle, siaynoq_lib_handle_hook)))
    {
      debug_output ("!!! Failed to register hotkeys");
      retval &= FALSE;
    }

  if (!(siaynoq_tiling_init (siaynoq_main_wnd_handle)))
    {
      debug_output ("!!! Failed to init tiling mechanisms");
      retval &= FALSE;
    }

  SY_WINDOWACTIVATED = RegisterWindowMessage (SIAYNOQ_MSG_APP_FOCUS_CHANGE);
  last_stat = (0 != SY_WINDOWACTIVATED);
  retval &= last_stat;

  if (!last_stat)
    debug_output ("!!! RegisterWindowMessage() returned 0 for SY_WINDOWACTIVATED");

  SY_WINDOWDESTROYED = RegisterWindowMessage (SIAYNOQ_MSG_APP_LOSING_WINDOW);
  last_stat = (0 != SY_WINDOWDESTROYED);
  retval &= last_stat;

  if (!last_stat)
    debug_output ("!!! RegisterWindowMessage() returned 0 for SY_WINDOWACTIVATED");

  if (DEBUG)
    assert (retval);

  /* Tile foreground window immediately; if foreground window isn't
     tileable, find one that is. */
  if (retval)
    put_focused_window_on_track (SIAYNOQ_IGNORED_HOTKEY_ARG);

  return retval;
}


BOOL
siaynoq_free ()
{
  BOOL retval;

  retval = TRUE;

  if (!(siaynoq_hotkeys_free (siaynoq_main_wnd_handle)))
    {
      debug_output ("!!! Failed to unregister hotkeys");
      retval &= FALSE;
    }

  /* Unload hooks library here */
  retval &= hooks_dll_unload ();

  return retval;
}


void
siaynoq_adjust_work_area ()
{
  RECT work_area;
  MINIMIZEDMETRICS min_metrics;

  /* Hide title bars of minimized windows */
  min_metrics.cbSize = sizeof (MINIMIZEDMETRICS);
  SystemParametersInfo (SPI_GETMINIMIZEDMETRICS, min_metrics.cbSize,
                        &min_metrics, 0);
  min_metrics.iArrange |= ARW_HIDE;
  SystemParametersInfo (SPI_SETMINIMIZEDMETRICS, min_metrics.cbSize,
                        &min_metrics, 0);

  work_area.left = 0;
  work_area.top = siaynoq_statusbar_height;
  work_area.right = GetSystemMetrics (SM_CXSCREEN);
  work_area.bottom = GetSystemMetrics (SM_CYSCREEN);

  SystemParametersInfo (SPI_SETWORKAREA, 0, &work_area, 0);
}


ATOM
siaynoq_wnd_regclass_statusbar (HINSTANCE instance)
{
  WNDCLASSEX wnd_class;

  memset (&wnd_class, 0, sizeof (WNDCLASSEX));

  wnd_class.cbSize = sizeof (WNDCLASSEX);
  wnd_class.style = CS_DBLCLKS;
  wnd_class.lpfnWndProc = siaynoq_wnd_proc_statusbar;
  wnd_class.hInstance = instance;
  wnd_class.hIcon = LoadIcon (0, IDI_APPLICATION);
  wnd_class.hCursor = LoadCursor (0, IDC_ARROW);
  wnd_class.hbrBackground = (HBRUSH) CreateSolidBrush (COLOR_BG_NORM);
  wnd_class.lpszClassName = SIAYNOQ_WND_CLASSNAME;

  return RegisterClassEx (&wnd_class);
}


DWORD WINAPI
siaynoq_run_startup_items (LPVOID ignore)
{
  siaynoq_run_reg_startup_items ();
  siaynoq_run_fs_startup_items ();

  return TRUE;
}


void
siaynoq_run_reg_startup_items ()
{
  const UINT num_target_hives = 2;
  const HKEY target_hives[] = {
    HKEY_LOCAL_MACHINE,
    HKEY_CURRENT_USER,
  };

  const UINT num_target_keys = 3;
  const LPTSTR target_keys[] = {
    "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx",
    "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
    "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
  };

  HKEY open_reg_key;

  /* For iterating over the different hives & keys */
  UINT hive_idx, key_idx;
  DWORD num_key_values, maxlen_value_name, maxlen_value_data;

  /* For iterating over the values (if any) retrieved from keys */
  UINT value_idx;
  LPTSTR value_name, value_data;
  DWORD len_value_name, len_value_data;

  debug_output ("~~~ Searching registry for startup items");
  for (hive_idx = 0; hive_idx < num_target_hives; hive_idx++)
    for (key_idx = 0; key_idx < num_target_keys; key_idx++)
      {
        if (ERROR_SUCCESS != (RegOpenKeyEx (target_hives[hive_idx],
                                            target_keys[key_idx],
                                            0, KEY_READ, &open_reg_key)))
          {
            debug_output ("!!! Failed to open registry key");
            continue;
          }

        num_key_values = reg_get_value_info (open_reg_key,
                                             &maxlen_value_name,
                                             &maxlen_value_data);
        if ((NULL == &num_key_values) || (0 == num_key_values))
          {
            debug_output ("~~~ No startup items found");
            continue;
          }

        /* Factor in the \0 terminator */
        len_value_name = sizeof (TCHAR) * maxlen_value_name++;
        len_value_data = sizeof (TCHAR) * maxlen_value_data++;

        value_name = malloc (len_value_name);
        value_data = malloc (len_value_data);

        if ((NULL == value_name) || (NULL == value_data))
          debug_output ("!!! Can't malloc() memory for item name/value");
        else
          {
            for (value_idx = 0; value_idx < num_key_values; value_idx++)
              {
                if (ERROR_SUCCESS != (RegEnumValue (open_reg_key, value_idx,
                                                    value_name, &len_value_name,
                                                    NULL, NULL,
                                                    (LPBYTE) value_data,
                                                    &len_value_data)))
                  {
                    debug_output ("!!! RegEnumValue() failed");
                    continue;
                  }

                debug_output ("~~~ Startup item value");
                debug_output (value_data);
                system_spawn (value_data, NULL, NULL, FALSE, NULL);
              }

            free (value_data);
            free (value_name);
          }

        RegCloseKey (open_reg_key);
      }

  debug_output ("~~~ Done searching registry for startup items");
}


void
siaynoq_run_fs_startup_items ()
{
  UINT num_target_csidls = 4;
  const int target_csidls[] = {
    CSIDL_COMMON_STARTUP,
    CSIDL_COMMON_ALTSTARTUP,
    CSIDL_STARTUP,
    CSIDL_ALTSTARTUP,
  };

  /* For iterating over CSISDLs */
  UINT csidl_idx;

  /* Working directory for a startup entry */
  LPTSTR startup_path = malloc (sizeof (TCHAR) * (MAX_PATH + 2));

  /* For iterating over a startup folder's contents */
  WIN32_FIND_DATA find_data;
  HANDLE find_handle;

  if (NULL == startup_path)
    {
      debug_output ("!!! Can't malloc() memory for startup path");
      return; /* Not sure if this should be considered broken... */
    }

  debug_output ("~~~ Searching file system for startup items");
  for (csidl_idx = 0; csidl_idx < num_target_csidls; csidl_idx++)
    {
      if (!(SHGetSpecialFolderPath (NULL, startup_path,
                                    target_csidls[csidl_idx], FALSE)))
        {
          debug_output ("!!! SHGetSpecialFolderPath() returned FALSE");
          continue;
        }

      debug_output ("~~~ Startup target directory");
      debug_output (startup_path);

      find_handle = FindFirstFile (startup_path, &find_data);
      if (INVALID_HANDLE_VALUE == find_handle)
        {
          debug_output ("!!! FindFirstFile() returned INVALID_HANDLE_VALUE");
          continue;
        }

      if (("." != find_data.cFileName) && (".." != find_data.cFileName))
        do
          { /* I'm not sure if Explorer makes these discriminations,
               too.  Seems useful, though. */
            if (!((find_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
                  || (find_data.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)
                  || (find_data.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY)))
              system_spawn (find_data.cFileName, NULL, startup_path, TRUE, NULL);
          }
        while (0 != FindNextFile (find_handle, &find_data));

      FindClose (find_handle);
    }
  free (startup_path);
  debug_output ("~~~ Done searching file system for startup items");
}


HWND
siaynoq_wnd_create_statusbar (HINSTANCE instance, ATOM class_atom)
{
  HWND wnd_handle = CreateWindowEx (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                    MAKEINTATOM (class_atom),
                                    SIAYNOQ_WND_TITLE,
                                    WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP,
                                    0, 0,
                                    GetSystemMetrics (SM_CXSCREEN),
                                    siaynoq_statusbar_height,
                                    (HWND) NULL, (HMENU) NULL,
                                    instance,
                                    NULL);
  if (NULL == wnd_handle)
    debug_output ("Window creation failed; function returned null handle.");
  else
    {
      SetWindowPos (wnd_handle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
      ShowWindow (wnd_handle, SW_SHOWNOACTIVATE);

      siaynoq_adjust_work_area ();

      /* Register timer for time/date display updates */
      if (0 == siaynoq_time_date_display_timer)
        {
          siaynoq_time_date_display_timer = SetTimer (wnd_handle,
                                                      SIAYNOQ_TIME_DATE_DISPLAY_TIMER_EVENT_ID,
                                                      100,
                                                      NULL);
          if (0 == siaynoq_time_date_display_timer)
            debug_output ("!!! Failed to set time/date update timer");
          else
            debug_output ("~~~ Time/date update timer successfully installed");
        }

      /* Request notification for session messages */
      if (!(WTSRegisterSessionNotification (wnd_handle, NOTIFY_FOR_THIS_SESSION)))
        debug_output ("!!! Request for session messages failed");
    }

  return wnd_handle;
}


LRESULT CALLBACK
siaynoq_wnd_proc_statusbar (HWND wnd_handle, UINT message, WPARAM wParam, LPARAM lParam)
{
  SIAYNOQHOTKEY *hotkey;

  /* At some point, check if we're actually on XP or above */
  if ((WM_DISPLAYCHANGE == message)
      || ((WM_WTSSESSION_CHANGE == message) && (WTS_SESSION_UNLOCK == wParam)))
    {
      debug_output ("@@@ System-wide status change");
      debug_output ("@@@ Resizing work area");
      siaynoq_adjust_work_area ();
      debug_output ("@@@ Retiling windows");
      siaynoq_set_wnd_handle_on_track (siaynoq_curr_maximized_wnd_handle,
                                       FALSE,
                                       (LPCREATESTRUCT) NULL);
    }

  if (SY_WINDOWACTIVATED == message)
    {
      debug_output ("@@@ SY_WINDOWACTIVATED received; setting safety buffer");
      siaynoq_window_activated_timer = SetTimer (wnd_handle,
                                                 SIAYNOQ_WINDOW_ACTIVATED_TIMER_EVENT_ID,
                                                 TIMEOUT_WINDOW_ACTIVATE,
                                                 NULL);
      if (DEBUG)
        assert (0 != siaynoq_window_activated_timer);

      if (0 == siaynoq_window_activated_timer)
        debug_output ("!!! Failed to set window activation timer");
      else
        {
          debug_output ("~~~ Window activation timer successfully installed");
          siaynoq_next_maximized_wnd_handle = (HWND) wParam;
        }
    }
  else if (SY_WINDOWDESTROYED == message)
    return siaynoq_msg_handler_sy_windowdestroyed (wnd_handle, wParam, lParam);
  else if (WM_TIMER == message)
    {
      if (SIAYNOQ_TIME_DATE_DISPLAY_TIMER_EVENT_ID == (UINT_PTR) wParam)
        {
          InvalidateRect (siaynoq_main_wnd_handle, NULL, FALSE);
          return 0;
        }
      else if (SIAYNOQ_WINDOW_ACTIVATED_TIMER_EVENT_ID == (UINT_PTR) wParam)
        {
          debug_output ("~~~ Window activation timer triggered");
          if (!(KillTimer (siaynoq_main_wnd_handle, SIAYNOQ_WINDOW_ACTIVATED_TIMER_EVENT_ID)))
            debug_output ("!!! Failed to kill window activation timer");

          if (siaynoq_curr_maximized_wnd_handle == siaynoq_next_maximized_wnd_handle)
            debug_output ("~~~ Current and next maximized window handles identical; skipping tiling");
          else
            {
              HWND tmp_wnd_handle = siaynoq_set_wnd_handle_on_track (siaynoq_next_maximized_wnd_handle,
                                                                     FALSE,
                                                                     NULL);

              if (siaynoq_curr_maximized_wnd_handle != siaynoq_next_maximized_wnd_handle)
                debug_output ("!!! siaynoq_set_wnd_handle_on_track() failed; refusing to touch state");
              else
                {
                  siaynoq_next_maximized_wnd_handle = NULL;
                  siaynoq_prev_maximized_wnd_handle = tmp_wnd_handle;
                }
            }
        }
    }
  else if (WM_WTSSESSION_CHANGE == message)
    {
      if (WTS_SESSION_LOCK == wParam)
        {
          debug_output ("@@@ Locking session");
          if (!((siaynoq_active_window_count)
                && (reg_set_value ("ProgramCount",
                                   &siaynoq_active_window_count,
                                   sizeof (UINT),
                                   REG_DWORD,
                                   NULL,
                                   "SessionInformation",
                                   TRUE,
                                   TRUE))))
            debug_output ("!!! Failed to write program count to registry");
          debug_output_ex ("--- %u active windows found\n", siaynoq_active_window_count);
        }
    }
  else if (WM_CLOSE == message)
    {
      if (!(WTSUnRegisterSessionNotification (wnd_handle)))
        debug_output ("!!! Request to stop receiving session messages failed");

      siaynoq_free ();
      PostQuitMessage (0);
    }
  else if (WM_QUIT == message)
    {
      if (0 != siaynoq_time_date_display_timer)
        if (!(KillTimer (siaynoq_main_wnd_handle, SIAYNOQ_TIME_DATE_DISPLAY_TIMER_EVENT_ID)))
          debug_output ("!!! Failed to kill time/date update timer");

      if (0 != siaynoq_window_activated_timer)
        if (!(KillTimer (siaynoq_main_wnd_handle, SIAYNOQ_TIME_DATE_DISPLAY_TIMER_EVENT_ID)))
          debug_output ("!!! Failed to kill window activation timer");
    }
  else if ((WM_HOTKEY == message)
      && ((IDHOT_SNAPDESKTOP != (int) wParam)
          && (IDHOT_SNAPWINDOW != (int) wParam)))
    {
      int hotkey_id = (int) wParam;
      LPTSTR func_name;
      UINT func_name_maxlen = 50;

      func_name = malloc (func_name_maxlen * sizeof (TCHAR));
      if (NULL != func_name)
        {
          GlobalGetAtomName ((ATOM) hotkey_id, func_name, func_name_maxlen);
          debug_output (func_name);

          hotkey = siaynoq_hotkey_by_name (func_name);
          if (NULL != hotkey)
            (hotkey->func_ptr) (hotkey->arg);
          else
            debug_output ("!!! got a NULL instead of a hotkey");
        }
      else
        debug_output ("!!! couldn't malloc() memory for the hotkey func name");

      free (func_name);
    }
  else if (WM_PAINT == message)
    return siaynoq_msg_handler_wm_paint (wnd_handle, wParam, lParam);
  else
    return DefWindowProc (wnd_handle, message, wParam, lParam);

  return 0;
}


HWND
siaynoq_find_next_tiling_candidate(HWND target_hwnds[])
{
  HWND retval = NULL;
  UINT target_hwnds_num;
  UINT target_hwnd_idx = 0;

  if (NULL == target_hwnds)
    {
      target_hwnds[0] = siaynoq_next_maximized_wnd_handle;
      target_hwnds[1] = siaynoq_prev_maximized_wnd_handle;
    }
  target_hwnds_num = sizeof (target_hwnds) / sizeof (HWND);

  for (; target_hwnd_idx < target_hwnds_num; target_hwnd_idx++)
    {
      if ((NULL == target_hwnds[target_hwnd_idx])
          ||  !(IsWindow (target_hwnds[target_hwnd_idx])))
        continue;
      retval = target_hwnds[target_hwnd_idx];
    }

  return retval;
}


LRESULT
siaynoq_msg_handler_sy_windowdestroyed (HWND wnd_handle, WPARAM wParam,
                                        LPARAM lParam)
{ /* TODO: Add code to make sure that, for n tileable windows, one is on
     the track if n > 0 */

  HWND closing_wnd;
  HWND possible_main_wnd; /* for iterating over windows */

  debug_output ("@@@ SY_WINDOWDESTROYED");

  closing_wnd = (HWND) wParam;
  if (closing_wnd == siaynoq_main_wnd_handle)
    return 0;

  siaynoq_marked_for_destruction_wnd_handle = closing_wnd;

  if (DEBUG)
    {
      LPTSTR wnd_title;
      int len_title;

      len_title = GetWindowTextLength (closing_wnd) + 1;
      wnd_title = malloc (len_title * sizeof (TCHAR));

      if (NULL != wnd_title)
        {
          GetWindowText (closing_wnd, wnd_title, len_title);
          debug_output ("^^^ about to be destroyed");
          debug_output (wnd_title);
          free (wnd_title);
        }

      len_title = GetWindowTextLength (siaynoq_curr_maximized_wnd_handle) + 1;
      wnd_title = malloc (len_title * sizeof (TCHAR));

      if (NULL != wnd_title)
        {
          GetWindowText (siaynoq_curr_maximized_wnd_handle, wnd_title, len_title);
          debug_output ("^^^ curr maximized");
          debug_output (wnd_title);
        }
      free (wnd_title);

      len_title = GetWindowTextLength (siaynoq_prev_maximized_wnd_handle) + 1;
      wnd_title = malloc (len_title * sizeof (TCHAR));

      if (NULL != wnd_title)
        {
          GetWindowText (siaynoq_prev_maximized_wnd_handle, wnd_title, len_title);
          debug_output ("^^^ prev maximized");
          debug_output (wnd_title);
        }
      free (wnd_title);
    } /* DEBUG */

  /* The tracked window is closing; need to find a replacement */
  if (closing_wnd == siaynoq_main_wnd_handle)
    {
      /* Priority is the next-in-line */
      if ((NULL != siaynoq_next_maximized_wnd_handle)
          && IsWindow (siaynoq_next_maximized_wnd_handle))
        {
          possible_main_wnd = siaynoq_next_maximized_wnd_handle;
          /* NULL it since after the next-in-line has been maximized, it
             should no longer be a candidate */
          siaynoq_next_maximized_wnd_handle = NULL;
        }
      else /* No next-in-line */
        {
          /* Try the last maximized window */
          if ((NULL != siaynoq_prev_maximized_wnd_handle)
              && IsWindow (siaynoq_prev_maximized_wnd_handle))
            {
              possible_main_wnd = siaynoq_prev_maximized_wnd_handle;
              /* NULL it since after the last maximized window has been
                 maximized, it should no longer be a candidate */
              siaynoq_prev_maximized_wnd_handle = NULL;
            }

          /* Normally, we'd be done by now.  The only times
             siaynoq_prev_maximized_wnd_handle should be NULL is if the
             main window just closed, or there's one or no visible,
             tileable window. */

          /* WTF?!  Still nothing? */
          else
            {
              debug_output ("::: No window tagged as next-in-line");

              /* Got to find a suitable candidate */
              possible_main_wnd = GetWindow (closing_wnd, GW_HWNDNEXT);
              if (NULL != possible_main_wnd)
                {
                  while ((NULL != possible_main_wnd) &&
                         (!(siaynoq_is_target_wnd_tileable (possible_main_wnd))))
                    {
                      debug_output ("!!! GetWindow() returned a non-tileable window handle; trying for another one");
                      possible_main_wnd = GetWindow (possible_main_wnd, GW_HWNDNEXT);
                    }

                  if (NULL != possible_main_wnd)
                    siaynoq_prev_maximized_wnd_handle = possible_main_wnd;
                  else
                    debug_output ("!!! Done with GetWindow() but didn't get a tileable window; shell left with inconsistent internal state");
                }
              else
                debug_output ("!!! GetWindow() returned NULL on first try");
            }
        }
        
      /* Bingo. */
      if ((NULL != possible_main_wnd)
          && IsWindow (possible_main_wnd))
        siaynoq_set_wnd_handle_on_track (possible_main_wnd, FALSE, NULL);
    }
  else /* Re-tile the whole bunch */
    siaynoq_tile_non_focused_wnd ();

  /*
  if (closing_wnd != siaynoq_curr_maximized_wnd_handle)
    siaynoq_tile_non_focused_wnd ();
  else if (IsWindow (siaynoq_prev_maximized_wnd_handle))
    {
      siaynoq_set_wnd_handle_on_track (siaynoq_prev_maximized_wnd_handle, FALSE, NULL);
      siaynoq_prev_maximized_wnd_handle = NULL;
    }
  */

  return 0;
}


LRESULT
siaynoq_msg_handler_wm_paint (HWND wnd_handle, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT paint; /* The all important PAINTSTRUCT */

  RECT coords_bar; /* The status bar coordinates */
  /* RECTs needed for laying out the various components of the bar */
  RECT coords_layout;
  RECT coords_time_date;
  RECT coords_title;

  SIZE text_size; /* Used with GetTextExtentPoint32() */
  LPTSTR layout_symbol = LAYOUT_TILE; /* symbol for current layout mode */

  /* Brush handles for painting in color */
  HBRUSH brush_bg_norm = CreateSolidBrush (COLOR_BG_NORM);
  HBRUSH brush_bg_label = CreateSolidBrush (COLOR_BG_LABEL);

  /* Device contexts */
  HDC main_dc;
  HDC buf_dc; /* Buffer DC for painting */
  HDC layout_dc; /* layout symbol display */
  HDC time_date_dc; /* time/date display */
  HDC title_dc; /* active window title display */

  /* Buffer bitmap handles */
  HBITMAP buf_bmp; /* main buffer DC */
  HBITMAP layout_bmp; /* layout symbol */
  HBITMAP time_date_bmp; /* time/date display */
  HBITMAP title_bmp; /* active window's title */

  /* Old bitmap handles assigned to created DCs */
  HBITMAP old_bmp; /* main buffer DC */
  HBITMAP old_layout_bmp; /* layout symbol */
  HBITMAP old_time_date_bmp; /* time/date display */
  HBITMAP old_title_bmp; /* active window's title */

  /* Font handles */
  HFONT font_handle;
  HFONT old_layout_font; /* The old font assigned to the layout DC */
  HFONT old_time_date_font; /* Old font assigned to time/date DC */
  HFONT old_title_font; /* Old font assigned to titel DC */

  /* For time/date component */
  LPTSTR curr_time_date_str;
  time_t curr_sys_time_value;
  struct tm *time_struct;
  /* TRUE if curr_time_date_str was successfully malloc()'d */
  BOOL ctd_str_is_dynamic;

  /* For window title display */
  HWND handle_active_wnd;
  UINT len_active_wnd_title;
  /* the string that'll be painted on the status bar */
  LPTSTR wnd_title_display;

  /* All set; let's rock! */
  main_dc = BeginPaint (wnd_handle, &paint);
  GetClientRect (wnd_handle, &coords_bar);

  font_handle = CreateFont (FONT_SIZE, 0, 0, 0,
                            FW_NORMAL,
                            FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS,
                            CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY,
                            DEFAULT_PITCH,
                            FONT_NAME);

  if (NULL == font_handle)
    { /* Can't find the font specified; need to fallback.  */
      debug_output ("!!! Couldn't find specified font");
      font_handle = CreateFont (FONT_SIZE, 0, 0, 0,
                                FW_NORMAL,
                                FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY,
                                DEFAULT_PITCH,
                                "Times New Roman");
      if (NULL == font_handle)
        { /* OMFG...  We're seriously screwed */
          debug_output ("!!! Gah!  Fail-safe font name failed!");
          return 1;
        }
    }

  /* Set up main DCs and all the attendant crap */
  buf_dc = CreateCompatibleDC (NULL);
  buf_bmp = CreateCompatibleBitmap (main_dc, coords_bar.right,
                                    coords_bar.bottom);
  old_bmp = (HBITMAP) SelectObject (buf_dc, buf_bmp);


  /* Draw tile layout indicator component */
  layout_dc = CreateCompatibleDC (NULL);
  old_layout_font = (HFONT) SelectObject (layout_dc, font_handle);
  SetBkMode (layout_dc, TRANSPARENT);

  layout_bmp = CreateCompatibleBitmap (main_dc, coords_bar.right,
                                       coords_bar.bottom);
  old_layout_bmp = (HBITMAP) SelectObject (layout_dc, layout_bmp);

  SetTextColor (layout_dc, COLOR_FG_LABEL);
  GetTextExtentPoint32 (layout_dc, layout_symbol,
                        (int) (strlen (layout_symbol) + 1), &text_size);

  coords_layout.left = 0;
  coords_layout.top = 0;
  coords_layout.right = text_size.cx;
  coords_layout.bottom = coords_bar.bottom;

  /* Draw actual tile layout indicator to buffer... */
  FillRect (layout_dc, &coords_layout, brush_bg_label);
  DrawText (layout_dc, layout_symbol, -1, &coords_layout,
            DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE);
  /* ... and then we transfer it to the status bar */
  BitBlt (buf_dc, 0, 0, coords_layout.right, coords_layout.bottom,
          layout_dc, 0, 0, SRCCOPY);

  /* Clean up */
  DeleteObject (old_layout_font);
  DeleteObject (layout_bmp);
  DeleteObject (old_layout_bmp);
  DeleteDC (layout_dc);
  /* Done drawing tile layout indicator component */


  /* Draw time/date component */
  time_date_dc = CreateCompatibleDC (NULL);
  old_time_date_font = (HFONT) SelectObject (time_date_dc, font_handle);
  SetBkMode (time_date_dc, TRANSPARENT);

  time_date_bmp = CreateCompatibleBitmap (time_date_dc, coords_bar.right,
                                          coords_bar.bottom);
  old_time_date_bmp = (HBITMAP) SelectObject (time_date_dc, time_date_bmp);

  SetTextColor (time_date_dc, COLOR_FG_LABEL);

  /* Try to get some time/date string for display */
  curr_time_date_str = malloc (sizeof (TCHAR) * 255); /* Is 255 chars enough? */
  ctd_str_is_dynamic = (NULL != curr_time_date_str);

  if (ctd_str_is_dynamic)
    {
      time (&curr_sys_time_value);
#pragma warning(push)
#pragma warning(disable:4996) /* MinGW doesn't support localtime_s() yet */
      time_struct = localtime (&curr_sys_time_value);
#pragma warning(pop)

      if (NULL == time_struct)
        debug_output ("!!! localtime() returned non-zero");
      else
        if (0 == strftime (curr_time_date_str, (sizeof (TCHAR) * 255),
                           TIME_DATE_FORMAT, time_struct))
          {
            debug_output ("!!! strftime() returned 0; maybe the buffer's not long enough?");
            free (curr_time_date_str);

            ctd_str_is_dynamic = FALSE;
            curr_time_date_str = "siaynoq !!! try adjusting the buffer for the time/date";
          }
    }
  else
    {
      debug_output ("!!! couldn't malloc() memory for current time/date display");
      curr_time_date_str = "siaynoq !!! time/date string malloc() failed";
    } /* if (ctd_str_is_dynamic) */

  GetTextExtentPoint32 (time_date_dc, curr_time_date_str,
                        (int) strlen (curr_time_date_str) + 1, &text_size);
  coords_time_date.left = 0;
  coords_time_date.top = 0;
  coords_time_date.right = text_size.cx;
  coords_time_date.bottom = coords_bar.bottom;

  /* Draw actual tile layout indicator to buffer... */
  FillRect (time_date_dc, &coords_time_date, brush_bg_label);
  DrawText (time_date_dc, curr_time_date_str, -1, &coords_time_date,
            DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE);
  /* ... do a bit of clean up ... */
  if (ctd_str_is_dynamic)
    free (curr_time_date_str);
  /* ... and then we transfer it to the status bar */
  BitBlt (buf_dc, (coords_bar.right - coords_time_date.right), 0,
          coords_bar.right, coords_time_date.bottom,
          time_date_dc, 0, 0, SRCCOPY);

  /* Further clean up */
  DeleteObject (old_time_date_font);
  DeleteObject (time_date_bmp);
  DeleteObject (old_time_date_bmp);
  DeleteDC (time_date_dc);
  /* Done drawing time/date component */


  /* Draw the piece de resistance: the focused window's title */
  handle_active_wnd = GetForegroundWindow ();

  /* If we have a max'd window, but the cursor is on top of a visible
     siaynoq component, make sure we used the max'd window's title,
     instead. */
  if ((handle_active_wnd == siaynoq_main_wnd_handle)
      && (NULL != siaynoq_curr_maximized_wnd_handle))
    handle_active_wnd = siaynoq_curr_maximized_wnd_handle;

  title_dc = CreateCompatibleDC (NULL);
  old_title_font = (HFONT) SelectObject (title_dc, font_handle);
  SetBkMode (title_dc, TRANSPARENT);

  title_bmp = CreateCompatibleBitmap (main_dc, coords_bar.right,
                                      coords_bar.bottom);
  old_title_bmp = (HBITMAP) SelectObject (title_dc, title_bmp);
  SetTextColor (title_dc, COLOR_FG_NORM);

  coords_title.left = 0;
  coords_title.top = 0;
  coords_title.right = coords_bar.right - coords_time_date.right - coords_layout.right;
  coords_title.bottom = coords_bar.bottom;

  /* Paint background color in anticipation */
  FillRect (title_dc, &coords_title, brush_bg_norm);

  /* In case there are no active windows, don't bother painting any
     title */
  len_active_wnd_title = GetWindowTextLength (handle_active_wnd);
  if ((handle_active_wnd != siaynoq_main_wnd_handle)
      && (0 != len_active_wnd_title))
    {
      len_active_wnd_title = sizeof (TCHAR) * (len_active_wnd_title + 1);
      wnd_title_display = malloc (len_active_wnd_title);

      if (NULL == wnd_title_display)
        debug_output ("!!! couldn't malloc() memory for active window title");
      else if (GetWindowText (handle_active_wnd, wnd_title_display,
                              len_active_wnd_title))
        {
          coords_title.left = 3; /* totally arbitrary */
          /* Paint the window title in! */
          DrawText (title_dc, wnd_title_display, -1, &coords_title,
                    DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
          if (_DEBUG)
            {
              debug_output ("^^^ currently focused");
              debug_output (wnd_title_display);
            }
          free (wnd_title_display);
        }
    }
  /* Transfer title to status bar */
  BitBlt (buf_dc, coords_layout.right, 0, coords_title.right, coords_title.bottom,
          title_dc, 0, 0, SRCCOPY);

  /* Clean up */
  DeleteObject (title_bmp);
  DeleteObject (old_title_font);
  DeleteObject (old_title_bmp);
  DeleteDC (title_dc);
  /* Done drawing focused window's title */

  /* Copy buf over to the main DC */
  BitBlt (main_dc, 0, 0, coords_bar.right, coords_bar.bottom,
          buf_dc, 0, 0, SRCCOPY);

  /* Final clean up */
  DeleteObject (brush_bg_label);
  DeleteObject (brush_bg_norm);

  DeleteObject (buf_bmp);
  DeleteObject (old_bmp);
  DeleteDC (buf_dc);
  DeleteObject (font_handle);

  /* Draw border */
  DrawEdge (main_dc, &coords_bar, EDGE_RAISED, BF_BOTTOM);
  EndPaint (wnd_handle, &paint);

  return 0;
}
