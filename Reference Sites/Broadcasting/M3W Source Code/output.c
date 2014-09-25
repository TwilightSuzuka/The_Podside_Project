/* output.c
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

/* disable Warning about deprecated C Standard Function */
#pragma warning(disable : 4996)

#include <windows.h>
#include <stdlib.h>
#include "encoder.h"
#include "output.h"
#include "main.h"
#include "param.h"
#include "resource.h"
#include "option.h"

static int out_file_open = 0;
static int out_recording =0;
static int out_size =0;
HANDLE out_file;



static void out_record_open(void)
{ int open_mode;
  if (out_file_open)
  {  errormsg("output file already open",0);
     return;
  }
  if ((autonameflag || recordfile==NULL || recordfile[0]==0) && !get_mp3_out_file())
    return;
  if (appendflag)
    open_mode=OPEN_ALWAYS;
  else
  { out_file = CreateFile(recordfile,	GENERIC_READ,FILE_SHARE_READ,
                       (LPSECURITY_ATTRIBUTES) NULL,OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,NULL);
	if (out_file != INVALID_HANDLE_VALUE) 
	{ LARGE_INTEGER size;
      GetFileSizeEx(out_file,&size);
	  CloseHandle(out_file);
	  if (size.QuadPart!=0 
	      && IDYES!=MessageBox(NULL,"Overwrite previous recording?",
	                       recordfile,MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)
	      && !get_mp3_out_file())
            return;
	}
	open_mode=CREATE_ALWAYS;
  }
  out_file = CreateFile(recordfile,	GENERIC_WRITE,FILE_SHARE_READ,
                       (LPSECURITY_ATTRIBUTES) NULL,open_mode,
                        FILE_ATTRIBUTE_NORMAL,NULL);
  if (out_file == INVALID_HANDLE_VALUE) 
  {  errormsg("Unable to open mp3 output file.",0); 
     return;
  }
  if (appendflag)
   out_size= SetFilePointer(out_file, 0, 0, FILE_END ); 
  else
   out_size= SetFilePointer(out_file, 0, 0, FILE_BEGIN ); 
  out_file_open=1;
}

void display_out_status(void)
{  SetDlgItemNumber(hMainDialog,IDC_OUTSIZE,out_size,FALSE);
}

static void out_record_close(void)
{ if (!out_file_open)
    return;
  CloseHandle(out_file);
  out_file_open = 0;
}

/* the interface */

/* function called to output mp3 data */
void out_mp3_data(mp3_buffer n)
{ char *mp3_out;
  int mp3_size;
  DWORD dwBytesWritten;
  if (!out_recording)
    return;
  if (!out_file_open)
  {  errormsg("No output file",0);
     return;
  }
  mp3_out= n->buffer;
  mp3_size = n->size;
  while (mp3_size > 0)
  { if (!WriteFile(out_file, (LPSTR) mp3_out, mp3_size,
          &dwBytesWritten, NULL))
      { error_code = GetLastError();
        errormsg("Unable to write to mp3 file",error_code);
		out_record_stop();
		return;
      }
    mp3_out = mp3_out + dwBytesWritten;
    out_size = out_size + dwBytesWritten; 
    mp3_size = mp3_size - dwBytesWritten;
  };

}

/* function called to start recording output file */
void out_record_start(void)
{ if (out_recording)
    return;
  if(!out_file_open)
    out_record_open();
  if (out_file_open)
  { out_recording = 1;
    SetDlgItemText(hMainDialog,IDC_RECORD_INDICATOR,"RECORDING");
  }
}

/* function called to stop recording output file */
void out_record_stop(void)
{ if (!out_recording)
    return;
  out_recording = 0;
  SetDlgItemText(hMainDialog,IDC_RECORD_INDICATOR,"");
  out_record_close();
}

void out_record_exit(void)
{ if (out_recording)
    out_record_stop();
  if (out_file_open)
    out_record_close();
}

int get_mp3_out_file(void)
/* return true if we could get a valid output file name */
{ if (autonameflag)
	{  
	SYSTEMTIME t;
	GetLocalTime(&t);
	sprintf(tmp_option,"%s%04d-%02d-%02d-%02d-%02d-%02d.mp3", autonameprefix==NULL?"":autonameprefix,  
			t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);
	set_option(&recordfile,tmp_option);
	SetDlgItemText(hMainDialog,IDC_OUTFILENAME,recordfile); 
	return 1;
	}
  else
  { OPENFILENAME ofn;
	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.lStructSize=sizeof(OPENFILENAME);
    ofn.hwndOwner=hMainWindow;
    ofn.lpstrFilter="MP3 File\0*.mp3\0All Files\0*.*\0\0";
    ofn.nFilterIndex=1;
    if (recordfile!=NULL)
    strncpy(tmp_option,recordfile,MAXTMPOPTION);
    else tmp_option[0] = '\0';
    ofn.lpstrFile=(LPSTR)tmp_option;
    ofn.nMaxFile=MAXTMPOPTION;
    ofn.lpstrTitle="Record To";
    ofn.lpstrDefExt="mp3";     
    if (!appendflag)
     ofn.Flags= OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_NOREADONLYRETURN; 
    else 
     ofn.Flags= OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOREADONLYRETURN; 

    if(GetSaveFileName((LPOPENFILENAME)&ofn))
	{ set_option(&recordfile,tmp_option);
      SetDlgItemText(hMainDialog,IDC_OUTFILENAME,recordfile+ofn.nFileOffset); 
      return 1;
	}
  }
    return 0;
}

