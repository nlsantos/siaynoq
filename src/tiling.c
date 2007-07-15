#include "tiling.h"
#include "tools.h"
#include "config.h"

HWND siaynoq_main_wnd_handle = NULL;
UINT siaynoq_tileable_wnd_count = 0;
FLOAT siaynoq_non_focused_height_divisor = 1.0;
BOOL siaynoq_wnd_on_track_uses_max_space = TRUE;
BOOL siaynoq_tiled_inactive_uses_max_height = FALSE;

BOOL
siaynoq_tiling_init (HWND main_wnd_handle)
{
  siaynoq_main_wnd_handle = main_wnd_handle;
  assert (NULL != siaynoq_main_wnd_handle);
  return TRUE;
}


BOOL
siaynoq_is_new_wnd_tileable (HWND target_wnd,
                             DWORD style_create,
                             DWORD exstyle_create)
{
  assert (NULL != target_wnd);

  if ((target_wnd == siaynoq_main_wnd_handle)
      || (target_wnd == siaynoq_curr_maximized_wnd_handle)
      || (target_wnd == siaynoq_marked_for_destruction_wnd_handle))
    return FALSE;

  // Discard non-top level and/or non-app windows
  if (((style_create & WS_POPUP)
       || (style_create & WS_POPUPWINDOW))
      && (!(((style_create & WS_MAXIMIZEBOX)
             || (style_create & WS_TILEDWINDOW)
             || (style_create & WS_OVERLAPPEDWINDOW)
             || (exstyle_create & WS_EX_APPWINDOW))
            && (style_create & WS_VISIBLE)))) // This is *NOT* the same as IsWindowVisible()!
    {
      debug_output ("~~~ not tileable: doesn't have title bar or is not tagged as an appwindow");
      return FALSE;
    }

  // The following block checks for window classes that are off-limits
  LPTSTR wnd_class_name;
  BOOL retval;

  retval = TRUE;
  wnd_class_name = malloc (sizeof (TCHAR) * 100); // Will this always be long enough?

  if (NULL == wnd_class_name)
    {
      debug_output ("!!! couldn't malloc() memory for the window class name");
      retval = FALSE;
    }
  else
    {
      UINT ret_len = RealGetWindowClass (target_wnd, wnd_class_name, 100);

      if (ret_len)
        {
          debug_output ("~~~ Class name");
          debug_output (wnd_class_name);

          if (0 == strcmpi (wnd_class_name, "ConsoleWindowClass"))
            { // Don't fuck with Windows' command shell; class name accurate for XP
              retval = FALSE;
              debug_output ("!!! command shell detected; electing to not mess with it");
            }
          else if (0 == strcmpi (wnd_class_name, "#32770"))
            { // Don't fuck with Windows' task manager; class name accurate for XP
              retval = FALSE;
              debug_output ("!!! windows task manager detected; electing to not mess with it");
            }
          else if (0 == strcmpi (wnd_class_name, "Shell_TrayWnd"))
            { // Don't fuck with Windows' systray; class name accurate for W95 until XP
              retval = FALSE;
              debug_output ("!!! windows systray detected; electing to not mess with it");
            }
        }

      if (!retval)
        debug_output ("~~~ not tileable: class name appears in blacklist");
    }
  free (wnd_class_name);

  if (DEBUG)
    {
      LPTSTR wnd_module_filename;
      wnd_module_filename = malloc (sizeof (TCHAR) * (MAX_PATH + 1));
      if (NULL != wnd_module_filename)
        {
          UINT ret_len = GetWindowModuleFileName (target_wnd, wnd_module_filename, MAX_PATH);

          if (ret_len)
            {
              debug_output ("~~~ Module path");
              debug_output (wnd_module_filename);
            }
        }
      else
        debug_output ("!!! Couldn't malloc() memory for module's path");
      free (wnd_module_filename);
    }

  return retval;
}


BOOL
siaynoq_is_target_wnd_tileable (HWND target_wnd)
{
  // Discard invisible windows
  if (!(IsWindowVisible (target_wnd)))
    return FALSE;

  WINDOWINFO wnd_info;
  wnd_info.cbSize = sizeof (WINDOWINFO);
  if (!(GetWindowInfo (target_wnd, &wnd_info)))
    {
      debug_output ("~~~ not tileable: GetWindowInfo() returned FALSE");
      return FALSE;
    }

  return siaynoq_is_new_wnd_tileable (target_wnd, wnd_info.dwStyle, wnd_info.dwExStyle);
}


HWND
siaynoq_set_wnd_handle_on_track (HWND target_wnd,
                                 BOOL target_is_new,
                                 LPCREATESTRUCT reserved)
{
  if (target_wnd == siaynoq_curr_maximized_wnd_handle)
    return siaynoq_curr_maximized_wnd_handle;

  if (target_is_new)
    {
      if (!(siaynoq_is_new_wnd_tileable (target_wnd, reserved->style, reserved->dwExStyle)))
        return siaynoq_prev_maximized_wnd_handle;
    }
  else if (!(siaynoq_is_target_wnd_tileable (target_wnd)))
    return siaynoq_prev_maximized_wnd_handle;

  HWND retval;

  retval = siaynoq_curr_maximized_wnd_handle;

  siaynoq_curr_maximized_wnd_handle = target_wnd;

  UINT num_tiled_windows = siaynoq_tile_non_focused_wnd ();

  if (DEBUG)
    {
      FILE *fp;
      fp = fopen ("d:\\siaynoq.log", "a");
      fprintf (fp, "--- %u tileable windows found\n", num_tiled_windows);
      fflush (fp);
      fclose (fp);
    }

  siaynoq_tiled_inactive_uses_max_height = (num_tiled_windows <= 1);
  siaynoq_wnd_on_track_uses_max_space = (0 == num_tiled_windows);

  RECT dimensions = siaynoq_calc_wnd_on_track_dimension ();

  if (target_is_new)
    {
      reserved->x = dimensions.left;
      reserved->y = dimensions.top;
      reserved->cx = dimensions.right;
      reserved->cy = dimensions.bottom;
    }
  else
    MoveWindow (target_wnd,
                dimensions.left,
                dimensions.top,
                dimensions.right,
                dimensions.bottom,
                TRUE);

/*   SetFocus (target_wnd); */
  InvalidateRect (siaynoq_main_wnd_handle, NULL, FALSE);

  return retval;
}


RECT
siaynoq_calc_wnd_on_track_dimension ()
{
  RECT work_area;
  RECT retval;

  SystemParametersInfo (SPI_GETWORKAREA, 0, &work_area, 0);
  work_area.bottom -= work_area.top;
  retval = work_area;

  if (!siaynoq_wnd_on_track_uses_max_space)
    {
      debug_output ("::: using max_track_width");
      retval.right *= (MAX_TRACK_WIDTH / 100.0);
    }

  return retval;
}


BOOL CALLBACK
siaynoq_filter_desktop_active_wnd (HWND current_wnd, LPARAM ignore)
{
  if (!(siaynoq_is_target_wnd_tileable (current_wnd)))
    return TRUE;

  siaynoq_tileable_wnd_count++;

  if (DEBUG)
    {
      LPTSTR wnd_title;
      int len_title;

      len_title = GetWindowTextLength (current_wnd) + 1;
      wnd_title = malloc (len_title * sizeof (TCHAR));

      if (NULL != wnd_title)
        {
          GetWindowText (current_wnd, wnd_title, len_title);
          debug_output (wnd_title);
        }
      free (wnd_title);
    }

  return TRUE;
}


BOOL CALLBACK
siaynoq_tile_non_focused_wnd_walker (HWND current_wnd, LPARAM scr_info)
{
  if (!(siaynoq_is_target_wnd_tileable (current_wnd)))
    return TRUE;

  WORD x_start_coord = HIWORD (scr_info);
  WORD y_tile_inc = LOWORD (scr_info);

  if (DEBUG)
    {
      LPTSTR wnd_title;
      int len_title;

      len_title = GetWindowTextLength (current_wnd) + 1;
      wnd_title = malloc (len_title * sizeof (TCHAR));

      if (NULL != wnd_title)
        {
          GetWindowText (current_wnd, wnd_title, len_title);

          FILE *fp;
          fp = fopen ("d:\\siaynoq.log", "a");
          fprintf (fp, "+++ tiling: %s at_x: %u\n", wnd_title, (siaynoq_tileable_wnd_count * y_tile_inc));
          fflush (fp);
          fclose (fp);

          debug_output (wnd_title);
        }
      free (wnd_title);
    }

  RECT work_area;

  SystemParametersInfo (SPI_GETWORKAREA, 0, &work_area, 0);

  UINT wnd_width;
  UINT wnd_height;

  wnd_width = work_area.right - x_start_coord;
  wnd_height = work_area.bottom - work_area.top;

  if (!siaynoq_tiled_inactive_uses_max_height)
    wnd_height /= siaynoq_non_focused_height_divisor;

  siaynoq_tileable_wnd_count--;
  MoveWindow (current_wnd,
              x_start_coord,
              work_area.top + (siaynoq_tileable_wnd_count * y_tile_inc),
              wnd_width,
              wnd_height,
              TRUE);

  return (siaynoq_tileable_wnd_count > 0);
}


UINT
siaynoq_tile_non_focused_wnd ()
{
  UINT retval;

  debug_output ("::: filtering active windows");
  siaynoq_tileable_wnd_count = 0;
  EnumWindows (siaynoq_filter_desktop_active_wnd, (LPARAM) NULL);
  debug_output ("::: done filtering active windows");

  retval = siaynoq_tileable_wnd_count;

  WORD y_tile_inc;
  WORD max_wnd_width;
  RECT work_area;

  SystemParametersInfo (SPI_GETWORKAREA, 0, &work_area, 0);

  y_tile_inc = ((work_area.bottom - work_area.top) / (siaynoq_tileable_wnd_count * 1.0));
  max_wnd_width = (work_area.right * (MAX_TRACK_WIDTH / 100.0));

  assert (10 <= MAX_TRACK_WIDTH);
  assert (0 < max_wnd_width);

  siaynoq_active_window_count = 0;

  debug_output ("::: tiling active but unfocused windows");
  siaynoq_non_focused_height_divisor = (siaynoq_tileable_wnd_count * 1.0);
  EnumWindows (siaynoq_tile_non_focused_wnd_walker, MAKELPARAM (y_tile_inc, max_wnd_width));
  debug_output ("::: done tiling active but unfocused windows");

  return retval;
}
