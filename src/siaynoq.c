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
#include <time.h>

#include "siaynoq.h"
#include "config.h"
#include "tiling.h"
#include "tools.h"
#include "hotkeys.h"

#if (_WIN32_WINNT >= 0x501)
#  include <wtsapi32.h>
#endif

#include <shellapi.h>

// Items needed from shlobj.h
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
// - Items needed from shlobj.h

const char* SIAYNOQ_WND_CLASSNAME = "SiaynoqWndClass";
const char* SIAYNOQ_WND_CLASSNAME_DESKTOP = "SiaynoqWndClassDesktop";
const char* SIAYNOQ_WND_TITLE = "[dpi] Siaynoq";
const char* SIAYNOQ_WND_TITLE_DESKTOP = "[dpi] Siaynoq (Desktop)";
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
  ATOM wnd_class_atom;

  retval = TRUE;

  retval &= adjust_process_priviledge ();

  siaynoq_statusbar_height = FONT_SIZE + 6;

  if (DEBUG && _DEBUG)
    {
      if ((wnd_class_atom = siaynoq_wnd_regclass_desktop (instance)))
        siaynoq_main_wnd_handle = siaynoq_wnd_create_desktop (instance, wnd_class_atom);
    }

  if ((wnd_class_atom = siaynoq_wnd_regclass_statusbar (instance)))
    siaynoq_main_wnd_handle = siaynoq_wnd_create_statusbar (instance, wnd_class_atom);

  if (!(wnd_class_atom))
    {
      debug_output ("!!! Window class registration failed; function returned 0.  Aborting.");
      return FALSE;
    }

  // Load hooks library here
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
  retval &= (0 != SY_WINDOWACTIVATED);

  if (!retval)
    debug_output ("!!! RegisterWindowMessage() returned 0 for SY_WINDOWACTIVATED");

  SY_WINDOWDESTROYED = RegisterWindowMessage (SIAYNOQ_MSG_APP_LOSING_WINDOW);
  retval &= (0 != SY_WINDOWDESTROYED);

  if (!retval)
    debug_output ("!!! RegisterWindowMessage() returned 0 for SY_WINDOWACTIVATED");

  if (DEBUG)
    assert (retval);

  // Tile foreground window immediately
  if (retval)
    put_focused_window_on_track ((SIAYNOQHOTKEYARG) { 0 });

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

  // Unload hooks library here
  retval &= hooks_dll_unload ();

  return retval;
}


void
siaynoq_resize_work_area ()
{
  RECT work_area;
  UINT screen_width, screen_height;

  screen_width = GetSystemMetrics (SM_CXSCREEN);
  screen_height = GetSystemMetrics (SM_CYSCREEN);

  work_area.left = 0;
  work_area.top = siaynoq_statusbar_height;
  work_area.right = screen_width;
  work_area.bottom = screen_height;

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
  HKEY target_keys[] = { HKEY_LOCAL_MACHINE,
                         HKEY_CURRENT_USER };
  UINT target_key_count = 2;

  LPTSTR target_subkeys[] = { "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx",
                              "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                              "Software\\Microsoft\\Windows\\CurrentVersion\\Run" };
  UINT target_subkey_count = 3;

  HKEY reg_key;
  UINT key_idx, subkey_idx;

  debug_output ("~~~ Searching registry for startup items");
  for (key_idx = 0; key_idx < target_key_count; key_idx++)
    for (subkey_idx = 0; subkey_idx < target_subkey_count; subkey_idx++)
      {
        if (ERROR_SUCCESS == (RegOpenKeyEx (target_keys[key_idx],
                                            target_subkeys[subkey_idx],
                                            0,
                                            KEY_ALL_ACCESS,
                                            &reg_key)))
          {
            DWORD num_values, max_name_len, max_value_len;

            if (ERROR_SUCCESS == (RegQueryInfoKey (reg_key,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   &num_values,
                                                   &max_name_len,
                                                   &max_value_len,
                                                   NULL,
                                                   NULL)))
              {
                if (0 >= num_values)
                  debug_output ("~~~ No startup items found");
                else
                  {
                    max_name_len++;
                    max_value_len++;

                    UINT idx;
                    LPTSTR item_name, item_value;
                    DWORD item_name_len, item_value_len;
                    STARTUPINFO startup_info;

                    GetStartupInfo (&startup_info);

                    for (idx = 0; idx < num_values; idx++)
                      {
                        item_name_len = sizeof (TCHAR) * (max_name_len);
                        item_value_len = sizeof (TCHAR) * (max_value_len);

                        item_name = malloc (item_name_len);
                        item_value = malloc (item_value_len);

                        if (NULL == item_name)
                          debug_output ("!!! Can't malloc() memory for item name");
                        else if (NULL == item_value)
                          debug_output ("!!! Can't malloc() memory for item value");
                        else
                          {
                            if (ERROR_SUCCESS == (RegEnumValue (reg_key,
                                                                idx,
                                                                item_name,
                                                                &item_name_len,
                                                                NULL,
                                                                NULL,
                                                                item_value,
                                                                &item_value_len)))
                              {
                                debug_output ("~~~ Startup item value");
                                debug_output (item_value);

                                PROCESS_INFORMATION proc_info;

                                if (!(CreateProcess (NULL,
                                                     item_value,
                                                     NULL,
                                                     NULL,
                                                     FALSE,
                                                     DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                                                     NULL,
                                                     NULL,
                                                     &startup_info,
                                                     &proc_info)))
                                  debug_output ("!!! CreateProcess() returned zero");
                                else
                                  {
                                    CloseHandle (proc_info.hProcess);
                                    CloseHandle (proc_info.hThread);
                                  }
                              }
                            else
                              debug_output ("!!! RegEnumValue() failed");
                          }

                        free (item_value);
                        free (item_name);
                      }
                  }

                RegCloseKey (reg_key);
              }
            else
              debug_output ("!!! Failed to create/open registry key");
          }
        else
          debug_output ("!!! Failed to create/open registry key");
      }

  debug_output ("~~~ Searching file system for startup items");
  int target_csidl[] = { CSIDL_COMMON_STARTUP,
                         CSIDL_COMMON_ALTSTARTUP,
                         CSIDL_STARTUP,
                         CSIDL_ALTSTARTUP };
  UINT target_csidl_count = 4;

  UINT csidl_idx;

  for (csidl_idx = 0; csidl_idx < target_csidl_count; csidl_idx++)
    {
      LPTSTR startup_path = malloc (sizeof (TCHAR) * (MAX_PATH + 2));
      if (NULL == startup_path)
        debug_output ("!!! Can't malloc() memory for startup path");
      else if (!(SHGetSpecialFolderPath (NULL, startup_path, target_csidl[csidl_idx], FALSE)))
        debug_output ("!!! SHGetSpecialFolderPath() returned FALSE");
      else
        {
          debug_output ("~~~ Startup target directory");
          debug_output (startup_path);

          WIN32_FIND_DATA find_data;
          HANDLE find_handle;

          find_handle = FindFirstFile (startup_path, &find_data);
          if (INVALID_HANDLE_VALUE == find_handle)
            debug_output ("!!! FindFirstFile() returned INVALID_HANDLE_VALUE");
          else if (("." != find_data.cFileName) && (".." != find_data.cFileName))
            {
              do
                if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
                  {
                    if (32 >= ((int) ShellExecute (NULL,
                                                   NULL,
                                                   find_data.cFileName,
                                                   NULL,
                                                   startup_path,
                                                   SW_SHOWDEFAULT)))
                      debug_output ("!!! ShellExecute() returned an error value");
                    else
                      {
                        debug_output ("~~~ Startup item value");
                        debug_output (find_data.cFileName);
                      }
                  }
              while (0 != FindNextFile (find_handle, &find_data));
              FindClose (find_handle);
            }
        }
      free (startup_path);
    }

  return TRUE;
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

      siaynoq_resize_work_area ();

      // Register timer for time/date display updates
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

      // Request notification for session messages
      if (!(WTSRegisterSessionNotification (wnd_handle, NOTIFY_FOR_THIS_SESSION)))
        debug_output ("!!! Request for session messages failed");
    }

  return wnd_handle;
}


LRESULT CALLBACK
siaynoq_wnd_proc_statusbar (HWND wnd_handle, UINT message, WPARAM wParam, LPARAM lParam)
{
  if ((WM_DISPLAYCHANGE == message)
      || ((WM_WTSSESSION_CHANGE == message) && (WTS_SESSION_UNLOCK == wParam)))
    {
      debug_output ("@@@ System-wide status change");
      debug_output ("@@@ Resizing work area");
      siaynoq_resize_work_area ();
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
    {
      debug_output ("@@@ SY_WINDOWDESTROYED");
      HWND target_wnd_handle = (HWND) wParam;

      // TODO: Add code to make sure that, for n tileable windows, one is on the track if n > 0
      if (target_wnd_handle != siaynoq_main_wnd_handle)
        {
          siaynoq_marked_for_destruction_wnd_handle = target_wnd_handle;

          if (DEBUG)
            {
              LPTSTR wnd_title;
              int len_title;

              len_title = GetWindowTextLength (target_wnd_handle) + 1;
              wnd_title = malloc (len_title * sizeof (TCHAR));

              if (NULL != wnd_title)
                {
                  GetWindowText (target_wnd_handle, wnd_title, len_title);
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
            } // DEBUG

          if (NULL == siaynoq_prev_maximized_wnd_handle)
            {
              debug_output ("::: No window tagged as next-in-line");
              HWND next_wnd_handle;

              next_wnd_handle = GetWindow (target_wnd_handle, GW_HWNDNEXT);
              if (NULL != next_wnd_handle)
                {
                  while ((NULL != next_wnd_handle) && (!(siaynoq_is_target_wnd_tileable (next_wnd_handle))))
                    {
                      debug_output ("!!! GetWindow() returned a non-tileable window handle; trying for another one");
                      next_wnd_handle = GetWindow (next_wnd_handle, GW_HWNDNEXT);
                    }

                  if (NULL != next_wnd_handle)
                    siaynoq_prev_maximized_wnd_handle = next_wnd_handle;
                  else
                    debug_output ("!!! Done with GetWindow() but didn't get a tileable window; shell left with inconsistent internal state");
                }
              else
                debug_output ("!!! GetWindow() returned NULL on first try");
            }

          if (target_wnd_handle != siaynoq_curr_maximized_wnd_handle)
            siaynoq_tile_non_focused_wnd ();
          else if (IsWindow (siaynoq_prev_maximized_wnd_handle))
            {
              siaynoq_set_wnd_handle_on_track (siaynoq_prev_maximized_wnd_handle, FALSE, NULL);
              siaynoq_prev_maximized_wnd_handle = NULL;
            }
        } // target_wnd_handle != siaynoq_main_wnd_handle
    } // SY_WINDOWDESTROYED == message
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

          if (DEBUG)
            {
              FILE *fp;
              fp = fopen ("d:\\siaynoq.log", "a");
              fprintf (fp, "--- %u active windows found\n", siaynoq_active_window_count);
              fflush (fp);
              fclose (fp);
            }
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

          SIAYNOQHOTKEY *hotkey = siaynoq_hotkey_by_name (func_name);

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
    {
      RECT sbar_dimensions;
      GetClientRect (wnd_handle, &sbar_dimensions);

      RECT layout_coords;
      RECT time_date_coords;
      RECT title_coords;

      SIZE text_size;
      LPTSTR layout_symbol = LAYOUT_TILE;

      HFONT font_handle = CreateFont (FONT_SIZE, 0, 0, 0,
                                      FW_NORMAL,
                                      FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET,
                                      OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS,
                                      DEFAULT_QUALITY,
                                      DEFAULT_PITCH,
                                      FONT_NAME);

      if (NULL == font_handle)
        {
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
            {
              debug_output ("!!! Gah!  Fail-safe font name failed!");
              return 1;
            }
        }

      HBRUSH brush_bg_norm = CreateSolidBrush (COLOR_BG_NORM);
      HBRUSH brush_bg_label = CreateSolidBrush (COLOR_BG_LABEL);

      PAINTSTRUCT paint;
      HDC main_dc = BeginPaint (wnd_handle, &paint);

      HDC buf_dc = CreateCompatibleDC (NULL);
      HBITMAP buf_bmp = CreateCompatibleBitmap (main_dc, sbar_dimensions.right, sbar_dimensions.right);
      HBITMAP old_bmp = (HBITMAP) SelectObject (buf_dc, buf_bmp);

      // Draw tile layout indicator
      HDC layout_dc = CreateCompatibleDC (NULL);
      HFONT layout_old_font = (HFONT) SelectObject (layout_dc, font_handle);
      SetBkMode (layout_dc, TRANSPARENT);

      HBITMAP layout_bitmap = CreateCompatibleBitmap (main_dc, sbar_dimensions.right, sbar_dimensions.bottom);
      HBITMAP layout_old_bitmap = (HBITMAP) SelectObject (layout_dc, layout_bitmap);

      SetTextColor (layout_dc, COLOR_FG_LABEL);
      GetTextExtentPoint32 (layout_dc, layout_symbol, strlen (layout_symbol) + 1, &text_size);

      layout_coords.left = 0;
      layout_coords.top = 0;
      layout_coords.right = text_size.cx;
      layout_coords.bottom = sbar_dimensions.bottom;

      FillRect (layout_dc, &layout_coords, brush_bg_label);
      DrawText (layout_dc, layout_symbol, -1, &layout_coords, DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE);
      BitBlt (buf_dc, 0, 0, layout_coords.right, layout_coords.bottom, layout_dc, 0, 0, SRCCOPY);

      DeleteObject (layout_old_font);
      DeleteObject (layout_bitmap);
      DeleteObject (layout_old_bitmap);
      DeleteDC (layout_dc);
      // Drew tile layout indicator

      // Draw time/date
      HDC time_date_dc = CreateCompatibleDC (NULL);
      HFONT time_date_old_font = (HFONT) SelectObject (time_date_dc, font_handle);
      SetBkMode (time_date_dc, TRANSPARENT);

      HBITMAP time_date_bitmap = CreateCompatibleBitmap (time_date_dc, sbar_dimensions.right, sbar_dimensions.bottom);
      HBITMAP time_date_old_bitmap = (HBITMAP) SelectObject (time_date_dc, time_date_bitmap);

      LPTSTR curr_time_date = malloc (sizeof (TCHAR) * 100);
      BOOL malloc_failed = (NULL == curr_time_date);

      if (malloc_failed)
        {
          debug_output ("!!! couldn't malloc() memory for current time/date display");
          curr_time_date = "siaynoq !!! couldn't malloc() memory for current time/date display";
        }
      else
        {
          time_t time_val;
          time (&time_val);

          struct tm *time_struct = localtime (&time_val);

          if (NULL == time_struct)
            debug_output ("!!! localtime_s() returned non-zero");
          else
            if (0 == strftime (curr_time_date,
                               (sizeof (TCHAR) * 255),
                               TIME_DATE_FORMAT,
                               time_struct))
              {
                debug_output ("!!! strftime() returned 0; maybe the buffer's not long enough?");
                curr_time_date = "siaynoq !!! strftime() returned 0; maybe the buffer's not long enough?";
              }
        } // !malloc_failed

      GetTextExtentPoint32 (time_date_dc, curr_time_date, strlen (curr_time_date) + 1, &text_size);
      time_date_coords.left = 0;
      time_date_coords.top = 0;
      time_date_coords.right = text_size.cx;
      time_date_coords.bottom = sbar_dimensions.bottom;

      SetTextColor (time_date_dc, COLOR_FG_LABEL);
      FillRect (time_date_dc, &time_date_coords, brush_bg_label);

      DrawText (time_date_dc, curr_time_date, -1, &time_date_coords, DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE);

      if (!malloc_failed)
        free (curr_time_date);

      BitBlt (buf_dc, sbar_dimensions.right - time_date_coords.right, 0, sbar_dimensions.right, time_date_coords.bottom, time_date_dc, 0, 0, SRCCOPY);

      DeleteObject (time_date_old_font);
      DeleteObject (time_date_bitmap);
      DeleteObject (time_date_old_bitmap);
      DeleteDC (time_date_dc);
      // Drew time/date

      // Draw focused window's title
      HWND target_wnd_handle;
      UINT target_wnd_handle_title_len;

      target_wnd_handle = GetForegroundWindow ();

      if ((target_wnd_handle == siaynoq_main_wnd_handle)
          && (NULL != siaynoq_curr_maximized_wnd_handle))
        target_wnd_handle = siaynoq_curr_maximized_wnd_handle;

      HDC title_dc = CreateCompatibleDC (NULL);
      HFONT title_old_font = (HFONT) SelectObject (title_dc, font_handle);
      SetBkMode (title_dc, TRANSPARENT);

      HBITMAP title_bitmap = CreateCompatibleBitmap (main_dc, sbar_dimensions.right, sbar_dimensions.bottom);
      HBITMAP title_old_bitmap = (HBITMAP) SelectObject (title_dc, title_bitmap);

      title_coords.left = 0;
      title_coords.top = 0;
      title_coords.right = sbar_dimensions.right - time_date_coords.right - layout_coords.right;
      title_coords.bottom = sbar_dimensions.bottom;

      SetTextColor (title_dc, COLOR_FG_NORM);
      FillRect (title_dc, &title_coords, brush_bg_norm);

      if (0 < (target_wnd_handle_title_len = GetWindowTextLength (target_wnd_handle)))
        {
          LPTSTR active_wnd_title = malloc (sizeof (TCHAR) * target_wnd_handle_title_len);
          if (NULL == active_wnd_title)
            debug_output ("!!! couldn't malloc() memory for active window title");
          else
            {
              if (0 != GetWindowText (target_wnd_handle, active_wnd_title, 255))
                {
                  title_coords.left = 3;
                  DrawText (title_dc, active_wnd_title, -1, &title_coords, DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);

                  if (_DEBUG)
                    {
                      debug_output ("^^^ currently focused");
                      debug_output (active_wnd_title);
                    }
                }
              else
                debug_output ("!!! GetWindowText() returned 0");
            }
          free (active_wnd_title);
        }

      BitBlt (buf_dc, layout_coords.right, 0, title_coords.right, title_coords.bottom, title_dc, 0, 0, SRCCOPY);

      DeleteObject (title_bitmap);
      DeleteObject (title_old_font);
      DeleteObject (title_old_bitmap);
      DeleteDC (title_dc);
      // Drew focused window's title

      // Copy buf over to the main DC
      BitBlt (main_dc, 0, 0, sbar_dimensions.right, sbar_dimensions.bottom, buf_dc, 0, 0, SRCCOPY);

      DeleteObject (brush_bg_label);
      DeleteObject (brush_bg_norm);

      DeleteObject (buf_bmp);
      DeleteObject (old_bmp);
      DeleteDC (buf_dc);
      DeleteObject (font_handle);

      DrawEdge (main_dc, &sbar_dimensions, EDGE_RAISED, BF_BOTTOM);
      EndPaint (wnd_handle, &paint);
    }
  else
    return DefWindowProc (wnd_handle, message, wParam, lParam);

  return 0;
}


/** Window-class registration function */
ATOM
siaynoq_wnd_regclass_desktop (HINSTANCE instance)
{
  if (DEBUG)
    {
      WNDCLASSEX wnd_class;

      memset (&wnd_class, 0, sizeof (WNDCLASSEX));

      wnd_class.cbSize = sizeof (WNDCLASSEX);
      wnd_class.style = CS_DBLCLKS;
      wnd_class.lpfnWndProc = siaynoq_wnd_proc_desktop;
      wnd_class.hInstance = instance;
      wnd_class.hIcon = LoadIcon (0, IDI_APPLICATION);
      wnd_class.hCursor = LoadCursor (0, IDC_ARROW);
      wnd_class.hbrBackground = (HBRUSH) COLOR_WINDOW;
      wnd_class.lpszClassName = SIAYNOQ_WND_CLASSNAME_DESKTOP;

      return RegisterClassEx (&wnd_class);
    }

  return 1;
}


/** Window creation function */
HWND
siaynoq_wnd_create_desktop (HINSTANCE instance, ATOM class_atom)
{
  if (DEBUG)
    {
      HWND wnd_handle;
      RECT work_area;
      UINT screen_width, screen_height;

      wnd_handle = NULL;
      screen_width = GetSystemMetrics (SM_CXSCREEN);
      screen_height = GetSystemMetrics (SM_CYSCREEN);

      wnd_handle = CreateWindowEx (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                   MAKEINTATOM (class_atom),
                                   SIAYNOQ_WND_TITLE_DESKTOP,
                                   WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP,
                                   0, 0,
                                   screen_width, screen_height,
                                   (HWND) NULL, (HMENU) NULL,
                                   instance,
                                   NULL);
      if (NULL == wnd_handle)
        debug_output ("Window creation failed; function returned null handle.");
      else
        {
          SetWindowPos (wnd_handle, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
          ShowWindow (wnd_handle, SW_SHOWNOACTIVATE);
        }

      if (NULL != wnd_handle)
        {
          // Use whole screen as work area
          work_area.left = 0;
          work_area.top = 0;
          work_area.right = screen_width;
          work_area.bottom = screen_height;

          SystemParametersInfo (SPI_SETWORKAREA, 0, &work_area, 0);
        }

      return wnd_handle;
    }

  return NULL;
}


LRESULT CALLBACK
siaynoq_wnd_proc_desktop (HWND wnd_handle, UINT message, WPARAM wParam, LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT paint;

  switch (message)
    {
    case WM_PAINT:
      hdc = BeginPaint (wnd_handle, &paint);
      PaintDesktop (hdc);
      EndPaint (wnd_handle, &paint);
      break;

    case WM_ERASEBKGND:
      PaintDesktop ((HDC) wParam);
      return TRUE;

    case WM_NCPAINT:
    case WM_WINDOWPOSCHANGING:
      if (wParam)
        return 0;

      SetWindowPos (wnd_handle,
                    HWND_BOTTOM,
                    -1, -1, -1, -1,
                    SWP_NOSENDCHANGING | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
      break;

    case WM_CLOSE:
      siaynoq_free ();
      PostQuitMessage (0);
      break;

    case WM_HOTKEY:
      debug_output ("WM_HOTKEY");
      if ((IDHOT_SNAPDESKTOP != (int) wParam)
          && (IDHOT_SNAPWINDOW != (int) wParam))
        {
          int hotkey_id = (int) wParam;
          LPTSTR func_name;
          UINT func_name_maxlen = 255;

          func_name = malloc (func_name_maxlen * sizeof (TCHAR));
          if (NULL != func_name)
            {
              GlobalGetAtomName ((ATOM) hotkey_id, func_name, func_name_maxlen);
              debug_output (func_name);

              SIAYNOQHOTKEY *hotkey = siaynoq_hotkey_by_name (func_name);

              if (NULL != hotkey)
                (hotkey->func_ptr) (hotkey->arg);
              else
                debug_output ("!!! got a NULL instead of a hotkey");
            }
          else
            debug_output ("!!! couldn't malloc() memory for the hotkey func name");

          free (func_name);
        }
      break;

    default:
      return DefWindowProc (wnd_handle, message, wParam, lParam);
    }

  return 0;
}


