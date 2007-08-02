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
#ifndef _CONFIG_H_ /* include guard */
#define _CONFIG_H_

#ifndef DEBUG
#  define DEBUG 0
#endif

#ifndef _DEBUG
#  define _DEBUG 0
#endif

#define MAX_TRACK_WIDTH 60 /* percent; (arbitrary) minimum is 10 */

#ifdef _HOTKEYS_H_
static SIAYNOQHOTKEY hotkeys[] = {
  /* modifier,			virtual keycode,	function to execute,		param */
  { MOD_WIN,			VK_F9,		switch_users,			{ 0 }, },
  { MOD_WIN | MOD_CONTROL,	VK_F9,		exit_shell,			{ 0 }, },
  { MOD_WIN | MOD_CONTROL,	VK_F10,		logoff_user,			{ 0 }, },
  { MOD_WIN | MOD_CONTROL,	VK_F11,		load_hooks_dll,			{ 0 }, },
  { MOD_WIN | MOD_ALT,		VK_F11,		unload_hooks_dll,		{ 0 }, },
  { MOD_WIN,			VK_RETURN,	put_focused_window_on_track,	{ 0 }, },
  { MOD_WIN | MOD_CONTROL,	VK_RETURN,	zoom_focused_window,		{ 0 }, },
};
#endif /* _HOTKEYS_H_ */

#define MAIN_MOD_KEY VK_LWIN

#ifdef _SIAYNOQ_H_
static const UINT TIMEOUT_WINDOW_ACTIVATE = 500; /* in ms */
static const LPTSTR TIME_DATE_FORMAT = "%a %d %b %H:%M:%S";

static const LPTSTR FONT_NAME = "Verdana";
static const UINT FONT_SIZE = 14;

static const LPTSTR LAYOUT_FLOAT = "><>";
static const LPTSTR LAYOUT_TILE = "[]=";

static const COLORREF COLOR_BG_NORM = RGB (150, 150, 150);
static const COLORREF COLOR_FG_NORM = RGB (0, 0, 0);
static const COLORREF COLOR_BG_LABEL = RGB (0, 0, 0);
static const COLORREF COLOR_FG_LABEL = RGB (255, 255, 255);
#endif /* _SIAYNOQ_H_ */

#ifdef _TILING_H_
static TILING_RULE tiling_rules[] = {
  /* window class,		should they be tiled? */
  { "#32770",			FALSE }, /* Window's task manager */
  { "#32771",			FALSE }, /* Window's `CoolSwitch' */
  { "ConsoleWindowClass",	FALSE }, /* Window's command shell */
  { "Shell_TrayWnd",		FALSE }, /* Window's notification area */
  { "ROCKETDOCK",		FALSE }, /* RocketDock main window */
  { "RDBlankWnd",		FALSE }, /* from RocketDock, but I don't know what this does */
  { "RDIconWnd",		FALSE }, /* RocketDock icons */
  { "RDTitleWnd",		FALSE }, /* RocketDock title/icon caption */
  { "WorkerW",			FALSE }, /* some sort of auxillary window for RocketDock */
  { "",				FALSE },
  { NULL,			FALSE },
};
#endif /* _TILING_H_ */

#endif /* _CONFIG_H_ */
