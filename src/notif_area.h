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
#ifndef _NOTIF_AREA_H_ /* include guard */
#define _NOTIF_AREA_H_

#include "config.h"
#include "tools.h"

#ifndef _WIN32_DCOM
#  define _WIN32_DCOM
#endif

/* We need this, else MSVC ups and quits with an error.  (shlobj.h
   checks for this definition; maybe we shouid just take what we need
   from it?  I don't know; will need to check.) */
#undef _WIN32_IE
#define _WIN32_IE 0x0400

#include <tchar.h>
#include <windows.h>
#include <shlobj.h>

#define REGSTR_PATH_SHELLSERVICEOBJECTDELAYED (TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad"))

void siaynoq_notif_load_shell_service_objects ();
void siaynoq_notif_start_shell_service_objects (CLSID);
void siaynoq_notif_unload_shell_service_objects ();

#ifdef __MINGW32__
extern const GUID IID_IOleCommandTarget;

typedef struct IOleCommandTarget IOleCommandTarget, *PIOleCommandTarget;

typedef struct
{
  HRESULT (__stdcall *QueryInterface)(PIOleCommandTarget, REFIID, PVOID*);
  ULONG (__stdcall *AddRef)(PIOleCommandTarget);
  ULONG (__stdcall *Release)(PIOleCommandTarget);
  HRESULT (__stdcall *QueryStatus)(PIOleCommandTarget, const GUID*, ULONG, PVOID, PVOID);
  HRESULT (__stdcall *Exec)(PIOleCommandTarget, const GUID*, DWORD, DWORD, VARIANT*, VARIANT*);
} IOleCommandTargetVtbl, *PIOleCommandTargetVtbl;

struct IOleCommandTarget
{
  const PIOleCommandTargetVtbl lpVtbl;
};
#endif /* __MINGW32__ */

#endif /* _NOTIF_AREA_H_ */
