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
#include "siaynoq.h"
#include "tools.h"

const LPTSTR SIAYNOQ_SINGULARITY_MUTEX = "siaynoq_singularity_mutex";

int APIENTRY
WinMain (HINSTANCE instance, HINSTANCE prev, LPTSTR cmd_line, int cmd_show)
{
  /** xxx *** Security warning *** xxx **
   *
   * The mutex below is used to limit the shell to a single instance.
   * Microsoft notes, however, that it is possible for a malicious
   * entity to create this mutex before the shell does, thereby
   * preventing it from running.
   *
   * As to WHY that would be done, I have no idea.  This is just to warn
   * the security conscious developers.
   * 
   ** xxx *** Security warning *** xxx **/
  HANDLE mutex = CreateMutex (NULL, TRUE, SIAYNOQ_SINGULARITY_MUTEX);

  if (NULL == mutex)
    {
      if (ERROR_ALREADY_EXISTS == GetLastError ())
        debug_output ("!!! Existing instance found; refusing to spawn another");
      else
        debug_output ("!!! CreateMutex() returned NULL");
    }
  else if (siaynoq_init (instance))
    {
      MSG msg;

      CreateThread (NULL, 0, siaynoq_run_startup_items, NULL, 0, NULL);

      while (GetMessage (&msg, NULL, 0, 0) > 0)
        {
          TranslateMessage (&msg);
          DispatchMessage (&msg);
        }

      if (!(ReleaseMutex (mutex)))
        debug_output ("!!! Failed in releasing singularity mutex");

      return msg.wParam;
    }

  return 255;
}
