/* play.c $Revision: 1.2 $ $Date: 2011/06/09 02:15:43 $
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
#include "resource.h"
#include "play.h"
#include "main.h"
#include "sound.h"
#include "encoder.h"
#include "broadcast.h"
#include "mp3.h"
#include "output.h"
#include "param.h"
#include "option.h"

/* flags read in the player thread, written in main thread */
static int playing=0; 
static int pause=0; 
static int terminating=0;

/* the player theread will play a file when started */
static HANDLE hThread;

/* Return TRUE if playback thread is active */
int is_playing(void)
{ return((playing || pause) && !terminating); }

static void signal_termination(void)
{  if (FALSE==PostMessage(hMainWindow,WM_ENDPLAYER,0,0))
  {  error_code = GetLastError();
     errormsg(" unable to post message",error_code); 
	 end_play_file(); /* last effort to get things in line */
  }
}

static HANDLE open_play_file(void)
{ int i;
  char *filename;
  HANDLE *in_file;
  i = SendDlgItemMessage(hMainDialog,IDC_INPUTLIST,CB_GETCURSEL,0,0); 
  if (i == CB_ERR)
  { get_mp3_in_file();
    i = SendDlgItemMessage(hMainDialog,IDC_INPUTLIST,CB_GETCURSEL,0,0); 
    if (i == CB_ERR)
      return INVALID_HANDLE_VALUE;
  }
  filename = (char *)SendDlgItemMessage(hMainDialog,IDC_INPUTLIST,CB_GETITEMDATA,i,0);
  if (filename==NULL)
  { errormsg("Empty input file name",0);
	return INVALID_HANDLE_VALUE;
  }
  in_file = CreateFile(filename,	GENERIC_READ,0,
    (LPSECURITY_ATTRIBUTES) NULL,OPEN_EXISTING,
     FILE_ATTRIBUTE_NORMAL,NULL);
  if (in_file == INVALID_HANDLE_VALUE) 
  {  errormsg("Unable to open mp3 input file.",0); 
     CloseHandle(in_file);
  }
  return in_file;
}


/* to avoid frames that overlap buffer boundaries,
   we keep a buffer capable of two times MAXBUFFER,
   if the last frame is incomplete we move it to the beginning
   and read new data at a given offset into the buffer */

static   mp3_buffer in;

static DWORD play_bytes(void)
/* analyse the in buffer send complete frames to the output,
   move incomplete frames to the beginning of a new buffer
   and adjust the in size.
   return the total play_time in miliseconds of the frames send
*/
{ DWORD play_time;
  frame_header f;
  mp3_buffer next;

  play_time = 0;
  in->offset = 0;
  while (in->offset< in->size)
  { get_header(in->buffer+in->offset,in->size-in->offset,&f);
    if (f.incomplete)
		break;
	else
	{ play_time = play_time + f.time;
	  in->offset = in->offset+ f.size;
	}
	if (play_time > 500)
	  break;  
  }
  if (in->offset == 0)
	fatal_error("No progress made writing mp3 file");
  /* this is not clean, get_empty_mp3buffer might be used by another thread */
  next = get_empty_mp3buffer();
  if (next == NULL)
  {	in->size = in->size - in->offset;
    memmove(in->buffer,in->buffer+in->offset, in->size);
	in->offset = 0;
	return play_time;
  }    
  if (in->size > in->offset)
  {	next->size = in->size - in->offset;
    memmove(next->buffer,in->buffer+in->offset, next->size);
	next->offset = 0;
	in->size = in->offset;
  }
  in->offset = 0;
  mp3_ready(in);
  in = next;
  return play_time;
}


static DWORD PlayerThread(LPDWORD lpdwParam)
{ /* open a file if not ok signal termination and stop*/
  int play_time, start_time, sleep_time;
  int in_bytes;
  HANDLE in_file;

  in_file = open_play_file();

  if (in_file == INVALID_HANDLE_VALUE) 
  {  signal_termination();
     ExitThread(0);
     return 0;
  }
  start_time = timeGetTime();
  play_time =0;
  in = get_empty_mp3buffer();
  while (!terminating)
  {	if (pause)
	{ Sleep(500);
      start_time=start_time+500;
	  continue;
	}
    /* read one frame */
    if (!ReadFile(in_file, in->buffer+in->size, in->max - in->size, &in_bytes, NULL))
	{ error_code = GetLastError();
      errormsg("Unable to read mp3 file",error_code);
	  break;
    }
	if (in_bytes > 0) /* got some data */
    { in->size = in->size + in_bytes;
	  play_time = play_time + play_bytes();
	}
	else /* end of file */
    { if (repeat_flag)
		{   if (SetFilePointer(in_file,0,NULL,FILE_BEGIN)== 0xFFFFFFFF)
			{ error_code = GetLastError();
			  errormsg("Unable to repeat playing mp3 file",error_code);
			  break;
			}
	        else
			{ in->size = 0;
			  continue; /* start again */
			}
		}     
       else
         break;
    }
    sleep_time = timeGetTime();
	sleep_time = sleep_time - start_time;
	sleep_time = play_time - sleep_time;
	if (!terminating && (sleep_time > 0))
	  Sleep(sleep_time);
  }
  CloseHandle(in_file);
  free_mp3buffer(in);
  signal_termination();
  ExitThread(0);
  return 0;
}

/* the external interface */

void start_play_file(void)
{  DWORD dwThreadId;
  if (terminating)
    return;
  if (pause)
  {  pause = 0;
     SetDlgItemText(hMainDialog,IDC_PLAY_INDICATOR,"PLAYING");
     return;
  }	
  else if (playing)
	return;
  else
  { pause =0;
    playing = 1;
	stop_sound();
    hThread = CreateThread(
        NULL,                        /* no security attributes        */
        0,                           /* use default stack size        */
        (LPTHREAD_START_ROUTINE) PlayerThread, /* thread function       */
        NULL,                        /* argument to thread function   */
        0,                           /* use default creation flags    */
        &dwThreadId);                /* returns the thread identifier */
   if (hThread == NULL)
   {  errormsg("Unable to create player thread",0);
      pause =0;
      playing = 0;
	  return;
   }
   SetDlgItemText(hMainDialog,IDC_PLAY_INDICATOR,"PLAYING");
  }
}


void stop_play_file(void)
{ if (playing)
   terminating = 1;
}

void end_play_file(void)
{ terminating = 0;
  playing = 0;
  pause = 0;
  SetDlgItemText(hMainDialog,IDC_PLAY_INDICATOR,"");
}

void pause_play_file(void)
{ if (!playing || pause || terminating)
    return;
  pause = 1;
  SetDlgItemText(hMainDialog,IDC_PLAY_INDICATOR,"PAUSE");
}

void get_mp3_in_file(void)
{ OPENFILENAME ofn;
  tmp_option[0]=0;
  memset(&ofn,0,sizeof(OPENFILENAME));
  ofn.lStructSize=sizeof(OPENFILENAME);
  ofn.hwndOwner=hMainDialog;
  ofn.lpstrFilter="MP3 File\0*.mp3\0All Files\0*.*\0\0";
  ofn.nFilterIndex=1;
  ofn.lpstrFile=(LPSTR)tmp_option;
  ofn.nMaxFile=MAXTMPOPTION;
  ofn.lpstrDefExt="mp3";     
  ofn.Flags=OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;  
  if(GetOpenFileName((LPOPENFILENAME)&ofn))
  { int index;
	set_option(&inputfile, tmp_option);
    index = SendDlgItemMessage(hMainDialog,IDC_INPUTLIST,
                                CB_ADDSTRING,0,
                                (LPARAM) tmp_option+ofn.nFileOffset);
    SendDlgItemMessage(hMainDialog,IDC_INPUTLIST,
                       CB_SETCURSEL,index,0);  
    SendDlgItemMessage(hMainDialog,IDC_INPUTLIST,
                       CB_SETITEMDATA,index,(LPARAM)(DWORD)tmp_option); 
 } 
}

