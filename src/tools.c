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
#include "tools.h"
#include "config.h"

void
debug_output (LPCTSTR msg)
{
  if (DEBUG)
    {
      FILE *fp;
      fp = fopen ("d:\\siaynoq.log", "a");
      fprintf (fp, "%s\n", msg);
      fflush (fp);
      fclose (fp);
    }
}


BOOL
adjust_process_priviledge ()
{
  OSVERSIONINFO os_vers_info;
  os_vers_info.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

  if (!(GetVersionEx (&os_vers_info)))
    debug_output ("!!! GetVersionEx() failed");
  else
    {
      if ((4 <= os_vers_info.dwMajorVersion)
          && (10 > os_vers_info.dwMinorVersion)
          && (VER_PLATFORM_WIN32_NT == os_vers_info.dwPlatformId))
        {
          HANDLE token_handle;
          TOKEN_PRIVILEGES token_privs;

          token_privs.PrivilegeCount = 1;
          token_privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

          if (LookupPrivilegeValue (NULL, SE_SHUTDOWN_NAME, &token_privs.Privileges[0].Luid))
            {
              if (OpenProcessToken (GetCurrentProcess (),
                                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                    &token_handle))
                {
                  AdjustTokenPrivileges (token_handle,
                                         FALSE,
                                         &token_privs,
                                         sizeof (TOKEN_PRIVILEGES),
                                         NULL,
                                         NULL);
                  CloseHandle (token_handle);
                  return TRUE;
                }
              else
                debug_output ("!!! Can't open process token");
            }
          else
            debug_output ("!!! Failed to look up priviledge value");
        }
      else
        debug_output ("~~~ Not running on NT; adjusting priviledges not necessary");
    }
  return FALSE;
}


LPVOID
shared_mem_struct_init (HANDLE file_mapping_obj, LPCTSTR file_map_name, const size_t struct_size)
{
  LPVOID shared_mem;
  BOOL init_shared_mem;

  if (NULL != file_mapping_obj)
    {
      debug_output ("!!! File mapping object is not NULL; maybe it's already been initialized?");
      return NULL;
    }

  file_mapping_obj = CreateFileMapping (INVALID_HANDLE_VALUE,
                                        NULL,
                                        PAGE_READWRITE,
                                        0,
                                        struct_size,
                                        file_map_name);

  init_shared_mem = (GetLastError () != ERROR_ALREADY_EXISTS);

  if (NULL == file_mapping_obj)
    {
      debug_output ("!!! Failed in creating mapped file for shared memory.");
      return NULL;
    }

  shared_mem = MapViewOfFile (file_mapping_obj, FILE_MAP_WRITE, 0, 0, 0);

  if (NULL == shared_mem)
    debug_output ("!!! Failed in mapping view of shared memory.");
  else
    {
      if (init_shared_mem)
        memset (shared_mem, 0, struct_size);
      else
        {
          CloseHandle (file_mapping_obj);
          file_mapping_obj = NULL;
        }
    }

  return shared_mem;
}


BOOL
shared_mem_struct_free (LPVOID shared_mem, HANDLE file_mapping_obj)
{
  BOOL retval;

  retval = TRUE;

  if (NULL != shared_mem)
    {
      retval &= (0 != UnmapViewOfFile (shared_mem));

      if (retval)
        shared_mem = NULL;

      if ((DEBUG) && (!retval))
        debug_output ("!!! Failed in unmapping view of file");
    }

  if (NULL != file_mapping_obj)
    {
      retval &= (0 != CloseHandle (file_mapping_obj));

      if (retval)
        file_mapping_obj = NULL;

      if (DEBUG)
        {
          if (retval)
            debug_output ("!!! Closed file mapping handle");
          else
            {
              LPVOID lpMsgBuf;
              LPVOID lpDisplayBuf;
              DWORD dw = GetLastError ();

              FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            dw,
                            MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPTSTR) &lpMsgBuf,
                            0, NULL);

              lpDisplayBuf = (LPVOID) LocalAlloc (LMEM_ZEROINIT, 
                                                  (lstrlen ((LPCTSTR) lpMsgBuf) + 100) * sizeof (TCHAR)); 
              wsprintf ((LPTSTR) lpDisplayBuf, 
                        TEXT("!!! CloseHandle() failed with error %d: %s"),
                        dw, lpMsgBuf);
              debug_output (lpDisplayBuf);

              LocalFree (lpMsgBuf);
              LocalFree (lpDisplayBuf);
            }
        }
    }

  return retval;
}


LPBYTE
reg_get_value (LPCTSTR value_name, LPDWORD size, const LPDWORD reg_type, HKEY key, LPTSTR subkey_name)
{
  LPBYTE retval;
  HKEY reg_key;

  retval = NULL;

  if (NULL == key)
    key = HKEY_CURRENT_USER;

  if (NULL == subkey_name)
    subkey_name = "Software\\Disgruntled Paradigms, Inc.\\Siaynoq";

  if (ERROR_SUCCESS == (RegCreateKeyEx (key,
                                        subkey_name,
                                        0, NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ,
                                        NULL,
                                        &reg_key,
                                        NULL)))
    {
      if (ERROR_SUCCESS != (RegQueryValueEx (reg_key, value_name, 0, reg_type, retval, size)))
        debug_output ("!!! Failed to query registry value");

      RegCloseKey (reg_key);
    }
  else
    debug_output ("!!! Failed to create/open registry key");

  return retval;
}


BOOL
reg_set_value (LPTSTR value_name, LPVOID data, DWORD size, DWORD reg_type,
               HKEY key, LPTSTR subkey_name, BOOL opt_volatile, BOOL flush_key)
{
  BOOL retval;
  HKEY reg_key;

  retval = TRUE;

  if (NULL == key)
    key = HKEY_CURRENT_USER;

  if (NULL == subkey_name)
    subkey_name = "Software\\Disgruntled Paradigms, Inc.\\Siaynoq";

  if (ERROR_SUCCESS == (RegCreateKeyEx (key,
                                        subkey_name,
                                        0, NULL,
                                        (opt_volatile) ? REG_OPTION_VOLATILE : REG_OPTION_NON_VOLATILE,
                                        KEY_WRITE,
                                        NULL,
                                        &reg_key,
                                        NULL)))
    {
      if (ERROR_SUCCESS == (RegSetValueEx (reg_key, value_name, 0, reg_type, data, size)))
        {
          if (flush_key)
            RegFlushKey (reg_key);
        }
      else
        {
          debug_output ("!!! Failed to set registry value");
          retval &= FALSE;
        }
      RegCloseKey (reg_key);
    }
  else
    {
      debug_output ("!!! Failed to create/open registry key");
      retval &= FALSE;
    }

  return retval;
}


/**
 * Query an open registry key for value-related information.
 *
 * Sets the address of the return value (NOT ITS VALUE) to NULL on
 * failure.
 */
DWORD
reg_get_value_info (HKEY reg_key, LPDWORD max_name_len, LPDWORD max_value_len)
{
  LPDWORD value_count;

  if (ERROR_SUCCESS != RegQueryInfoKey(reg_key, NULL, NULL, NULL, NULL, NULL, NULL,
                                       value_count, max_name_len, max_value_len,
                                       NULL, NULL))
    {
      debug_output ("!!! Error while querying key for info");
      value_count = NULL;
    }

  return *value_count;
}


/**
 * Encapsulate execution of specified paths.
 *
 * Hide the mess necessary when dealing with CreateProcess() and
 * ShellExecute() and let a coder focus on fixing bugs.
 *
 * If use_shell is TRUE, system_spawn() will use ShellExecute(),
 * CreateProcess() otherwise.  Consequently, when setting use_shell to
 * TRUE, make sure that the action parameter contains a valid object
 * `verb'.
 *
 * The return value differs according to the mode selected to execute
 * the command line; see the documentation on the encapsulated API calls
 * for more details.
 */
int
system_spawn (LPTSTR cmd_line, LPCTSTR params, LPCTSTR curr_dir,
              BOOL use_shell, LPCTSTR action)
{
  int retval = 0;

  debug_output ("~~~ Trying to run command-line");
  debug_output (cmd_line);

  if (use_shell)
    { /* Quick note: ShellExecute() returns  32 or lower on failure */
      retval = (int) ShellExecute (NULL, action, cmd_line, params,
                                   curr_dir, SW_SHOWDEFAULT);
      if (32 >= retval)
        debug_output ("!!! system_spawn() failed while using ShellExecute()");
    }
  else
    { /* Quick note: CreateProcess() returns non-zero on failure */
      STARTUPINFO startup_info;
      PROCESS_INFORMATION proc_info;

      GetStartupInfo (&startup_info);
      retval = CreateProcess (NULL, cmd_line, NULL, NULL, FALSE,
                              DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                              NULL, NULL, &startup_info, &proc_info);

      if (!retval)
        {
          CloseHandle (proc_info.hProcess);
          CloseHandle (proc_info.hThread);
        }
      else
        debug_output ("!!! system_spawn() failed while using CreateProcess()");
    } /* not (use_shell) */

  return retval;
}


#ifndef _HOOKS_H_
#include "hooks.h"
static HANDLE siaynoq_lib_handle_hook = NULL;

HANDLE
hooks_dll_load (HWND main_wnd_handle)
{
  if (NULL == siaynoq_lib_handle_hook)
    {
      debug_output ("!!! Hooks library is not loaded; trying to load...");
      siaynoq_lib_handle_hook = LoadLibrary ("wafer.dll");

      if (NULL == siaynoq_lib_handle_hook)
        debug_output ("!!! Failed in loading hooks library.");
      else
        {
          BOOL (*hook_init_proc)() = (BOOL (*)()) GetProcAddress (siaynoq_lib_handle_hook,
                                                                  SIAYNOQ_HOOKS_INIT);
          if (NULL == hook_init_proc)
            debug_output ("!!! Failed to get address for hook init function.");
          else if (!((hook_init_proc) (main_wnd_handle)))
            debug_output ("!!! Failed to install system-wide hook(s).");
        }
    }

  return siaynoq_lib_handle_hook;
}

BOOL
hooks_dll_unload ()
{
  BOOL retval;

  retval = TRUE;

  if (NULL != siaynoq_lib_handle_hook)
    {
      BOOL (*hook_free_proc)() = (BOOL (*)()) GetProcAddress (siaynoq_lib_handle_hook,
                                                              SIAYNOQ_HOOKS_FREE);

      if (NULL == hook_free_proc)
        {
          debug_output ("!!! Failed to get address for hook free function.");
          retval &= FALSE;
        }
      else if (!((hook_free_proc) ()))
        {
          debug_output ("!!! Failed to uninstall system-wide hook(s).");
          retval &= FALSE;
        }

      if (FreeLibrary (siaynoq_lib_handle_hook))
        {
          siaynoq_lib_handle_hook = NULL;
          debug_output ("!!! Freed hooks library.");
        }
      else
        {
          debug_output ("!!! Failed in freeing hooks library.");
          retval &= FALSE;
        }
    }
  else
    debug_output ("!!! Can't unload; handle to DLL is NULL");

  return retval;
}
#endif
