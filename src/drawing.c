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
#include "drawing.h"
#include "config.h"
#include "tools.h"
#include "hotkeys.h"

RECT
siaynoq_draw_component_time_date (HDC bar_dc, RECT coords_bar, HFONT font_hnd)
{
  RECT coords_component;
  SIZE text_size; /* Used with GetTextExtentPoint32() */

  HDC component_dc;
  HBITMAP component_bmp;
  HBITMAP old_component_bmp;
  HFONT old_component_font;

  LPTSTR curr_time_date_str;
  time_t curr_sys_time_value;
  struct tm *time_struct;
  /* TRUE if curr_time_date_str was successfully malloc()'d */
  BOOL ctd_str_is_dynamic;

  HBRUSH brush_bg_label = CreateSolidBrush (COLOR_BG_LABEL);

  /* Set up DCs and all attendant crap */
  component_dc = CreateCompatibleDC (NULL);
  old_component_font = (HFONT) SelectObject (component_dc, font_hnd);
  SetBkMode (component_dc, TRANSPARENT);

  component_bmp = CreateCompatibleBitmap (component_dc, coords_bar.right,
                                          coords_bar.bottom);
  old_component_bmp = (HBITMAP) SelectObject (component_dc, component_bmp);
  SetTextColor (component_dc, COLOR_FG_LABEL);

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

  GetTextExtentPoint32 (component_dc, curr_time_date_str,
                        (int) strlen (curr_time_date_str) + 1, &text_size);
  coords_component.left = 0;
  coords_component.top = 0;
  coords_component.right = text_size.cx;
  coords_component.bottom = coords_bar.bottom;

  /* Draw actual time/date string  to buffer... */
  FillRect (component_dc, &coords_component, brush_bg_label);
  DrawText (component_dc, curr_time_date_str, -1, &coords_component,
            DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE);
  /* ... do a bit of clean up ... */
  if (ctd_str_is_dynamic)
    free (curr_time_date_str);
  /* ... and then we transfer it to the status bar */
  BitBlt (bar_dc, (coords_bar.right - coords_component.right), 0,
          coords_bar.right, coords_component.bottom,
          component_dc, 0, 0, SRCCOPY);

  /* Further clean up */
  DeleteObject (brush_bg_label);
  DeleteObject (old_component_font);
  DeleteObject (component_bmp);
  DeleteObject (old_component_bmp);
  DeleteDC (component_dc);

  return coords_component;
}


RECT
siaynoq_draw_component_wnd_title (HDC bar_dc, RECT coords_draw, HFONT font_hnd,
                                  HWND main_wnd_handle, HWND curr_maximized_wnd_handle)
{
  RECT coords_component;

  HDC component_dc;
  HBITMAP component_bmp;
  HBITMAP old_component_bmp;
  HFONT old_component_font;

  HWND handle_active_wnd;
  UINT len_active_wnd_title;
  /* the string that'll be painted on the status bar */
  LPTSTR wnd_title_display;

  HBRUSH brush_bg_norm = CreateSolidBrush (COLOR_BG_NORM);

  handle_active_wnd = GetForegroundWindow ();

  /* If we have a max'd window, but the cursor is on top of a visible
     siaynoq component, make sure we used the max'd window's title,
     instead. */
  if ((handle_active_wnd == main_wnd_handle)
      && (NULL != curr_maximized_wnd_handle))
    handle_active_wnd = curr_maximized_wnd_handle;

  component_dc = CreateCompatibleDC (NULL);
  old_component_font = (HFONT) SelectObject (component_dc, font_hnd);
  SetBkMode (component_dc, TRANSPARENT);

  component_bmp = CreateCompatibleBitmap (bar_dc, coords_draw.right,
                                          coords_draw.bottom);
  old_component_bmp = (HBITMAP) SelectObject (component_dc, component_bmp);
  SetTextColor (component_dc, COLOR_FG_NORM);

  coords_component.left = 0;
  coords_component.top = 0;
  coords_component.right = coords_draw.right;
  coords_component.bottom = coords_draw.bottom;

  /* Paint background color in anticipation */
  FillRect (component_dc, &coords_component, brush_bg_norm);

  /* In case there are no active windows, don't bother painting any
     title */
  len_active_wnd_title = GetWindowTextLength (handle_active_wnd);
  if ((handle_active_wnd != main_wnd_handle)
      && (0 != len_active_wnd_title))
    {
      len_active_wnd_title = sizeof (TCHAR) * (len_active_wnd_title + 1);
      wnd_title_display = malloc (len_active_wnd_title);

      if (NULL == wnd_title_display)
        debug_output ("!!! couldn't malloc() memory for active window title");
      else if (GetWindowText (handle_active_wnd, wnd_title_display,
                              len_active_wnd_title))
        {
          coords_component.left = 3; /* totally arbitrary */
          /* Paint the window title in! */
          DrawText (component_dc, wnd_title_display, -1, &coords_component,
                    DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
          if (_DEBUG)
            debug_output_ex ("^^^ currently focused: %s", wnd_title_display);
          free (wnd_title_display);
        }
    }
  /* Transfer title to status bar */
  BitBlt (bar_dc, coords_draw.left, 0, coords_draw.right, coords_draw.bottom,
          component_dc, 0, 0, SRCCOPY);

  /* Clean up */
  DeleteObject (brush_bg_norm);
  DeleteObject (component_bmp);
  DeleteObject (old_component_bmp);
  DeleteObject (old_component_font);
  DeleteDC (component_dc);

  return coords_component;
}
