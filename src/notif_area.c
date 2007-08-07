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

/* ---------------- SPECIAL NOTE
 *
 * A lot of thanks are in order.
 *
 * Thanks to those who blazed the trail: The coders who figured out how
 * Windows 2000 and later use COM for the notification area, and how to
 * make it work on shell replacements.  I don't have a complete list,
 * and I don't want to leave anyone out, so I'll leave it at that.
 *
 * Thanks, thanks, and thanks to those I stole the code from.
 *
 * PureLS, written in ANSI C and using only Win32 API calls, was the
 * perfect candidate to steal code from.  People familiar with PureLS's
 * codebase will immediately recognize the parts of its shellsvc
 * components that I stole.
 *
 * I also stole code from emergeDesktop's emergeTray applet, especially
 * the icon-handling stuff.
 * 
 */
#include "notif_area.h"

const GUID IID_IOleCommandTarget = {0xB722BCCBL,0x4E68,0x101B,{0xA2,0xBC,0x00,0xAA,0x00,0x40,0x47,0x70}};

void
siaynoq_notif_load_shell_service_objects ()
{
  HRESULT com_result;

  HKEY key_services;
  LONG reg_retval;
  UINT enum_idx;

  _TCHAR reg_val_name[32];
  _TCHAR reg_val_data[40];
  DWORD reg_val_name_len = 32 * sizeof (_TCHAR);
  DWORD reg_val_data_len = 40 * sizeof (_TCHAR);

  CLSID clsid;
  _TCHAR clsid_str[40];

  com_result = CoInitializeEx (NULL, (COINIT_APARTMENTTHREADED
                                      | COINIT_DISABLE_OLE1DDE));
  if ((S_OK != com_result) && (S_FALSE != com_result))
    {
      debug_output_ex ("!!! COM initialization failed; returned %i instead",
                       com_result);
      MessageBox(GetDesktopWindow(), TEXT("COM initialization failed"),
                 TEXT("siaynoq"),
                 MB_OK | MB_ICONERROR | MB_TOPMOST);
      return;
    }

  /* Start SysTrayInvoker */
  CLSIDFromString ((LPOLESTR) TEXT("{730F6CDC-2C86-11D2-8773-92E220524153}"),
                   &clsid);
  siaynoq_notif_start_shell_service_objects (clsid);

  debug_output_ex ("~~~ Trying to open reg key: %s", REGSTR_PATH_SHELLSERVICEOBJECTDELAYED);
  reg_retval = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                             REGSTR_PATH_SHELLSERVICEOBJECTDELAYED,
                             0, KEY_READ, &key_services);
  if (ERROR_SUCCESS != reg_retval)
    {
      debug_output ("!!! Failed trying to open reg key");
      return;
    }

  enum_idx = 0;
  while (ERROR_SUCCESS == RegEnumValue (key_services,
                                        enum_idx++,
                                        reg_val_name,
                                        &reg_val_name_len,
                                        NULL, NULL,
                                        (LPBYTE) reg_val_data,
                                        &reg_val_data_len))
    {
/* MinGW doesn't support _tcsncpy_s() yet */
#pragma warning(push)
#pragma warning(disable:4996)
      _tcsncpy (clsid_str, reg_val_data, 40);
#pragma warning(pop)
      clsid_str[39] = '\0';

      CLSIDFromString ((LPOLESTR) clsid_str, &clsid);
      siaynoq_notif_start_shell_service_objects (clsid);
    }
  RegCloseKey (key_services);
}


void
siaynoq_notif_start_shell_service_objects (CLSID clsid)
{
  HRESULT com_result;
  IOleCommandTarget *cmd_target;

  com_result = CoCreateInstance ((REFCLSID) &clsid,
                                 NULL,
                                 CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                                 (REFIID) &IID_IOleCommandTarget,
                                 (LPVOID*) &cmd_target);
  if (FAILED (com_result))
    {
      debug_output_ex ("!!! Can't create COM instance; returned %i",
                       com_result);
      return;
    }

  debug_output ("~~~ executing IOleCommandTarget instance");
  cmd_target->lpVtbl->Exec (cmd_target,
                            &CGID_ShellServiceObject,
                            2, 0, NULL, NULL);
}


void
siaynoq_notif_unload_shell_service_objects ()
{
  CoUninitialize();
}
