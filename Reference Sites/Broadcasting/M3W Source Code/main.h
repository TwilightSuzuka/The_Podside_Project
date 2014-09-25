/* main.h $Revision: 1.3 $ $Date: 2009/07/16 17:36:18 $
 * This File is part of m3w. 
 *
 *	M3W a mp3 streamer for the www
 *
 *	Copyright (c) 2001, 2002 Martin Ruckert (mailto:ruckertm@acm.org)
 * 
 * m3w is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * m3w is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with m3w; if not, write to the
 * Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <windows.h>
extern void errormsg(char *message, int error_code);
extern void fatal_error(char *message);
extern int error_code;
extern void message(int i, char *msg);
extern HWND hMainDialog;
extern HWND hMainWindow;
extern void display_main_dialog_information(HWND hDlg);
extern void display_autogain(void);
extern void SetDlgItemNumber(HWND hDlg, int IDC_Item, int n,int sign);
/* local messages */

#define WM_BROADCAST  			(WM_USER + 0)
#define WM_MESSAGE  		    (WM_USER + 1)
#define WM_ENDPLAYER 		    (WM_USER + 2)
#define WM_ENCODERREADY			(WM_USER + 3)
#define WM_ERRORMSG				(WM_USER + 4)
#define WM_SENDDISPLAY			(WM_USER + 5)

extern int log_scale;
extern int out_scale;
extern int linear2log(int i);
extern HICON hm3wIcon;
