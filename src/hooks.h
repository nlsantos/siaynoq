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
#ifndef _HOOKS_H_ // include guard
#define _HOOKS_H_

#include <windows.h>

#ifdef EXPORT_DLL
#  define DLL_CONVENTION __declspec(dllexport)
#else
#  define DLL_CONVENTION __declspec(dllimport)
#endif

#define SIAYNOQ_HOOKS_INIT "siaynoq_hooks_init"
#define SIAYNOQ_HOOKS_FREE "siaynoq_hooks_free"

#ifdef __cplusplus
extern C {
#endif

  BOOL DLL_CONVENTION siaynoq_hooks_init (HWND);
  BOOL DLL_CONVENTION siaynoq_hooks_free ();

#ifdef __cplusplus
}
#endif

#endif // _HOOKS_H_
