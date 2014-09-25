/* main.c $Revision: 1.15 $ $Date: 2011/06/09 02:15:43 $
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
#include <commctrl.h>
#include <htmlhelp.h> 
#include <stdio.h>
#include "resource.h"
#include "main.h"
#include "param.h"
#include "encoder.h"
#include "sound.h"
#include "queue.h"
#include "output.h"
#include "broadcast.h"
#include "option.h"
#include "BladeMP3EncDLL.h"
#include "play.h"
#include "id3tag.h"
#include "autogain.h"
#include "volumecompression.h"
#include "meter.h"



HWND hMainDialog=NULL;
HWND hMainWindow=NULL;
HWND hVolCompress=NULL;

HICON hm3wIcon;
static HINSTANCE hInstance;
#define CONFIGFILEMAX 512
static char szConfigFileName[CONFIGFILEMAX]="";
static char szConfigFilter[]="Configuration Files\0*.m3w\0All Files\0*.*\0\0";
static OPENFILENAME ocfn = {0};
static int error_count =0;



BOOL APIENTRY   
StartupDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{ switch ( message )
  { case WM_INITDIALOG:
      CheckDlgButton(hDlg,IDC_MINIMIZED,minimizedflag);
      CheckDlgButton(hDlg,IDC_RECORDING,recordingflag);
      CheckDlgButton(hDlg,IDC_BROADCASTING,broadcastingflag);
      CheckDlgButton(hDlg,IDC_LISTENING,listeningflag);
      CheckDlgButton(hDlg,IDC_PLAYING,playingflag);
  return TRUE;
    case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if ( LOWORD(wparam) == IDC_LISTENING && HIWORD(wparam) == BN_CLICKED)
      { if (IsDlgButtonChecked(hDlg,IDC_LISTENING))
          CheckDlgButton(hDlg,IDC_PLAYING,FALSE);
	  }
      else if ( LOWORD(wparam) == IDC_PLAYING && HIWORD(wparam) == BN_CLICKED)
      { if (IsDlgButtonChecked(hDlg,IDC_PLAYING))
          CheckDlgButton(hDlg,IDC_LISTENING,FALSE);
	  }
      else if( wparam == IDOK )
      { 
		minimizedflag = IsDlgButtonChecked(hDlg,IDC_MINIMIZED);
		recordingflag = IsDlgButtonChecked(hDlg,IDC_RECORDING);
		broadcastingflag = IsDlgButtonChecked(hDlg,IDC_BROADCASTING);
		listeningflag = IsDlgButtonChecked(hDlg,IDC_LISTENING);
		playingflag = IsDlgButtonChecked(hDlg,IDC_PLAYING);
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
      else if( wparam == IDCANCEL )
      {
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
     break;
  }
  return FALSE;
}

BOOL APIENTRY   
TagDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{ switch ( message )
  { case WM_INITDIALOG:
	  SetDlgItemText(hDlg,IDC_TAG_TITLE,tagtitle);
	  SetDlgItemText(hDlg,IDC_TAG_ARTIST,tagartist);
	  SetDlgItemText(hDlg,IDC_TAG_ALBUM,tagalbum);
      SetDlgItemInt(hDlg,IDC_TAG_YEAR,tagyear,FALSE);
	  SetDlgItemText(hDlg,IDC_TAG_COMMENT,tagcomment);
      SetDlgItemInt(hDlg,IDC_TAG_TRACK,tagtrack,FALSE);
      init_genre_dialog(hDlg);
      SendDlgItemMessage(hDlg,IDC_TAG_GENRE,CB_SETCURSEL,taggenre,0);  
  return TRUE;
    case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
      { 
        GetDlgItemText(hDlg,IDC_TAG_TITLE,tmp_option,MAXTMPOPTION);
		set_option(&tagtitle,tmp_option);
        GetDlgItemText(hDlg,IDC_TAG_ARTIST,tmp_option,MAXTMPOPTION);
	    set_option(&tagartist,tmp_option);
        GetDlgItemText(hDlg,IDC_TAG_ALBUM,tmp_option,MAXTMPOPTION);
	    set_option(&tagalbum,tmp_option);
        tagyear = GetDlgItemInt(hDlg,IDC_TAG_YEAR,NULL,FALSE);
        GetDlgItemText(hDlg,IDC_TAG_COMMENT,tmp_option,MAXTMPOPTION);
		set_option(&tagcomment,tmp_option);
        tagtrack = GetDlgItemInt(hDlg,IDC_TAG_TRACK,NULL,FALSE);
		{ int index;
          index = SendDlgItemMessage(hDlg,IDC_TAG_GENRE,CB_GETCURSEL,0,0);
		  if (index!=CB_ERR) 
	        taggenre = SendDlgItemMessage(hDlg,IDC_TAG_GENRE,CB_GETITEMDATA,index,0); 
		  else
		    taggenre = 12;
		}
        output_v1_tag(tagtitle,
			          tagartist,
				      tagalbum,
				      tagyear,
				      tagcomment,
				      tagtrack,
				      taggenre);
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
      else if( wparam == IDCANCEL )
      {
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
     break;
  }
  return FALSE;
}


void display_autogain(void)
{ 
  EnableWindow(GetDlgItem(hMainDialog,IDC_AUTOGAIN),!ag_disable);
  SendDlgItemMessage(hMainDialog,IDC_AUTOGAIN,BM_SETCHECK,ag_on?BST_CHECKED:BST_UNCHECKED,0);
}

void display_autovolume(void)
{   
SendDlgItemMessage(hMainDialog,IDC_AUTOVOLUME,BM_SETCHECK,vc_enable?BST_CHECKED:BST_UNCHECKED,0);
}
static void display_sound_status(void)
{ 
  SetDlgItemNumber(hMainDialog,IDC_SOUNDBYTES,current_sound_bytes,FALSE); 
}

int linear2log(int i)
/* return value between 0 and 0x7FFF,
   where 0 = -60dB or less and 0x7FFF = 0 dB or more
*/
{	double dB = level_to_dB(i);
    int result = (int)(0x7FFF*(1.0+dB/60.0));
    if (result < 0) result = 0;
	else if (result > 0x7FFF) result = 0x7FFF;
	return result;
}

static char *commaprint(unsigned long n)
{
	static int comma = ',';
	static char retbuf[30];
	char *p = &retbuf[sizeof(retbuf)-1];
	int i;

#if 0
	if(comma == '\0') {
		struct lconv *lcp = localeconv();
		if(lcp != NULL) {
			if(lcp->thousands_sep != NULL &&
				*lcp->thousands_sep != '\0')
				comma = *lcp->thousands_sep;
			else comma = ',';
		}
	}
#endif
	*p = '\0';
    i = 3;
	do {
		if(i-- == 0)
		{ *--p = comma;
		  i= 2;
		}
		*--p = '0' + (char)(n % 10);
		n /= 10;
	} while(n != 0);

	return p;
}


void SetDlgItemNumber(HWND hDlg, int IDC_Item, int n,int sign)
{ SetDlgItemText(hDlg,IDC_Item,commaprint(n));
}


void display_main_dialog_information(HWND hDlg)
{ int offset, index;
	   SetDlgItemText(hDlg,IDC_SENDNAME,mountpoint);
       SetDlgItemNumber(hDlg,IDC_ENCODERBITRATE,bitrate,FALSE);
       if /* (encoder_mode == Mono)
         SetDlgItemText(hDlg,IDC_ENCODERMODE,"Mono");
       else if */ (stereoflag)
         SetDlgItemText(hDlg,IDC_ENCODERMODE,"Stereo");
       else
         SetDlgItemText(hDlg,IDC_ENCODERMODE,"Joint Stereo");
       SetDlgItemText(hDlg,IDC_SOUNDCARDNAME,sound_devicename(current_sound_device));
       SetDlgItemText(hDlg,IDC_SOUNDCARDFORMAT,sound_description(current_sound_format));
	   if (recordfile!=NULL)
       { offset=strlen(recordfile); 
         while (offset > 0 && recordfile[offset-1]!='\\')
           offset--;
         SetDlgItemText(hDlg,IDC_OUTFILENAME, recordfile+offset);
       }
       if (inputfile!=NULL)
       { offset=strlen(inputfile); 
         while (offset > 0 && inputfile[offset-1]!='\\')
           offset--;
         index = SendDlgItemMessage(hDlg,IDC_INPUTLIST,
                            CB_ADDSTRING,0,
                            (LPARAM) inputfile+offset);
         SendDlgItemMessage(hDlg,IDC_INPUTLIST,
                            CB_SETITEMDATA,index,(LPARAM)(DWORD)inputfile); 
         SendDlgItemMessage(hDlg,IDC_INPUTLIST, CB_SETCURSEL,0,0);                                   
      }

	  SendDlgItemMessage(hDlg,IDC_REPEAT,BM_SETCHECK,repeat_flag?BST_CHECKED:BST_UNCHECKED,0);
      if (appendflag)
	    SendDlgItemMessage(hDlg,IDC_RECORD_APPEND,BM_SETCHECK,BST_CHECKED,0);
	  else
	    SendDlgItemMessage(hDlg,IDC_RECORD_APPEND,BM_SETCHECK,BST_UNCHECKED,0);
      if (autonameflag)
	    SendDlgItemMessage(hDlg,IDC_RECORD_AUTONAME,BM_SETCHECK,BST_CHECKED,0);
	  else
	    SendDlgItemMessage(hDlg,IDC_RECORD_AUTONAME,BM_SETCHECK,BST_UNCHECKED,0);
      display_autogain();
	  display_autovolume();
      CheckMenuItem(GetMenu(hMainWindow),ID_OPTIONS_ERRORSASMESSAGES,
	     MF_BYCOMMAND|(error_as_message?MF_CHECKED:MF_UNCHECKED));
	  CheckRadioButton(hDlg,IDC_LOG,IDC_LIN,log_scale?IDC_LOG:IDC_LIN);
	  CheckRadioButton(hDlg,IDC_OUT,IDC_IN,out_scale?IDC_OUT:IDC_IN);
}

BOOL APIENTRY  
MainDialogProc( HWND hDlg, UINT mId, WPARAM wparam, LPARAM lparam )
{ HICON hIcon;
  switch ( mId )
  {  case WM_INITDIALOG:

       meter_create(hInstance,GetDlgItem(hDlg,IDC_VOLUMECONTROL));
	   /* Paint the buttons */
       hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_STOP));
       SendDlgItemMessage(hDlg,IDC_INSTOP,BM_SETIMAGE,(WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);
       SendDlgItemMessage(hDlg,IDC_OUTSTOP,BM_SETIMAGE,(WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);
       SendDlgItemMessage(hDlg,IDC_SENDSTOP,BM_SETIMAGE,(WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);
       SendDlgItemMessage(hDlg,IDC_SOUNDSTOP,BM_SETIMAGE,(WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);
       hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_PAUSE));
       SendDlgItemMessage(hDlg,IDC_INPAUSE,BM_SETIMAGE,(WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);
       hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_PLAY));
       SendDlgItemMessage(hDlg,IDC_INPLAY,BM_SETIMAGE,(WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);
       SendDlgItemMessage(hDlg,IDC_SOUNDPLAY,BM_SETIMAGE,(WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);
       hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_RECORD));
       SendDlgItemMessage(hDlg,IDC_RECORD,BM_SETIMAGE,(WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);
       hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_SEND));
       SendDlgItemMessage(hDlg,IDC_SEND,BM_SETIMAGE,(WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);
       SetDlgItemNumber(hDlg,IDC_SOUNDBYTES,current_sound_bytes,FALSE);
	   display_main_dialog_information(hDlg);

      return TRUE;
   case WM_COMMAND:
      switch( LOWORD(wparam) )
      {
	  // user pressed the record/stop button...
	  case IDC_SOUNDPLAY:
	    start_sound();
		 meter_format();
           break;
         case IDC_SOUNDSTOP:
 	    stop_sound();
           break;
         case IDC_RECORD:
 	    out_record_start();
           break;
         case IDC_OUTSTOP:
 	    out_record_stop();
           break;
         case IDC_SEND:
 	       start_bc();
           break;

         case IDC_SENDSTOP:
 	       stop_bc();
           break;
		 case IDC_INPLAY:
		   start_play_file();
		   break;
		 case IDC_INPAUSE:
		   pause_play_file();
		   break;
		 case IDC_INSTOP:
		   stop_play_file();
		   break;
		 case IDC_REPEAT:
		   if (HIWORD(wparam)==BN_CLICKED)
		     repeat_flag =  (BST_CHECKED == SendDlgItemMessage(hDlg,IDC_REPEAT,BM_GETCHECK,0,0));
		   break;
		 case IDC_RECORD_APPEND:
		   if (HIWORD(wparam)==BN_CLICKED)
		     appendflag =  (BST_CHECKED == SendDlgItemMessage(hDlg,IDC_RECORD_APPEND,BM_GETCHECK,0,0));
		   break;
		 case IDC_RECORD_AUTONAME:
		   if (HIWORD(wparam)==BN_CLICKED)
		     autonameflag =  (BST_CHECKED == SendDlgItemMessage(hDlg,IDC_RECORD_AUTONAME,BM_GETCHECK,0,0));
		   break;
		 case IDC_AUTOGAIN:
		   if (HIWORD(wparam)==BN_CLICKED)
		     ag_on = !ag_on;
		   display_autogain();
		   break;
		 case IDC_INPUTLIST:
			 if ((HIWORD(wparam)==CBN_SELENDOK) && is_playing()) {
				 stop_play_file();
				 // Restart with the newly selected item
				 start_play_file();
			 }
		 case IDC_AUTOVOLUME:
		   if (HIWORD(wparam)==BN_CLICKED)
		     set_volumecompression((BST_CHECKED == SendDlgItemMessage(hDlg,IDC_AUTOVOLUME,BM_GETCHECK,0,0)));
		   break;
		 case IDC_LOG:
			 log_scale=1;
			 meter_format();
			 break;
		 case IDC_LIN:
			 log_scale=0;
			 meter_format();
			 break;
		 case IDC_OUT:
			 out_scale=1;
			 meter_format();
			 break;
		 case IDC_IN:
			 out_scale=0;
			 meter_format();
			 break;
       }
      break;
           
   case WM_SYSCOMMAND:
     if( wparam == SC_CLOSE ) 
     {    meter_delete();
		  EndDialog(hDlg, TRUE);
          stop_sound();
          return TRUE;
     }
     break;
  }
  return FALSE;
}

BOOL APIENTRY   
AboutDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
      SetDlgItemText(hDlg,IDC_VERSION,VERSIONSTR);
      return TRUE;
    case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
      {
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
      else if( wparam == IDCANCEL )
      {
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
     break;
  }
  return FALSE;
}

BOOL APIENTRY   
ConfigurationDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
      SendDlgItemMessage(hDlg,IDC_CONFIGURATION,CB_SETCURSEL,0,0);  
      option_usage(GetDlgItem(hDlg,IDC_CONFIGURATION));
      return TRUE;
    case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
      {
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
     break;
  }
  return FALSE;
}

BOOL APIENTRY   
SoundDialogProc( HWND hDlg, UINT mId, WPARAM wparam, LPARAM lparam )
{ 
  switch ( mId )
  { case WM_INITDIALOG:
      { int i, n, index, caps;
        n = sound_maxdevices();
        for (i = 0; i< n; i++)
        { 
          index = SendDlgItemMessage(hDlg,IDC_SOUNDLIST,CB_ADDSTRING,0,
                                (LPARAM)sound_devicename(i));
         SendDlgItemMessage(hDlg,IDC_SOUNDLIST,CB_SETITEMDATA,index,
                             (LPARAM)(DWORD)i); 
       }
       SendDlgItemMessage(hDlg,IDC_SOUNDLIST,CB_SETCURSEL,0,0);  
       SendDlgItemMessage(hDlg,IDC_SOUNDLIST,CB_SELECTSTRING,-1,
                          (LPARAM)sound_devicename(current_sound_device));
       index = SendDlgItemMessage(hDlg,IDC_SOUNDLIST,CB_GETCURSEL,0,0); 
	   index = SendDlgItemMessage(hDlg,IDC_SOUNDLIST,CB_GETITEMDATA,index,0);
	   caps = sound_caps(index);
       for (i=0; i<MAX_SOUND_FORMATS;i++)
       { if (sound_support(caps,i))
         { index = SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_ADDSTRING,0,(LPARAM)sound_description(i));
          SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_SETITEMDATA,index,(LPARAM)(DWORD)i); 
         }
       }
       SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_SETCURSEL,0,0);  
       SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_SELECTSTRING,-1,
                          (LPARAM)sound_description(current_sound_format));  
	 }
     SetDlgItemInt(hDlg,IDC_SOUNDBUFFERS,in_buffers,FALSE);
 
     return TRUE;
     case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if (HIWORD(wparam) == CBN_SELCHANGE && LOWORD(wparam) == IDC_SOUNDLIST)
      { int i, caps, index;
        index = SendDlgItemMessage(hDlg,IDC_SOUNDLIST,CB_GETCURSEL,0,0);  
	    index = SendDlgItemMessage(hDlg,IDC_SOUNDLIST,CB_GETITEMDATA,index,0);
        caps = sound_caps(index);
        SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_RESETCONTENT,0,0);
        for (i=0; i<MAX_SOUND_FORMATS;i++)
       { if (sound_support(caps,i))
         { index = SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_ADDSTRING,0,
                                (LPARAM)sound_description(i) );
          SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_SETITEMDATA,index,
                             (LPARAM)(DWORD)i); 
	   }
       }
       SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_SETCURSEL,0,0);  
       SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_SELECTSTRING,-1,(LPARAM)sound_description(current_sound_format));  
    } 
      else if( wparam == IDOK )
      { int index;
        index = SendDlgItemMessage(hDlg,IDC_SOUNDLIST,CB_GETCURSEL,0,0); 
	    index = SendDlgItemMessage(hDlg,IDC_SOUNDLIST,CB_GETITEMDATA,index,0);
        current_sound_device = index; 
        index = SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_GETCURSEL,0,0);  
        index = SendDlgItemMessage(hDlg,IDC_FREQUENCYLIST,CB_GETITEMDATA,index,0);
	    current_sound_format = index;
        in_buffers = GetDlgItemInt(hDlg,IDC_SOUNDBUFFERS,NULL,FALSE);
        change_sound_format();

  	    SetDlgItemText(hMainDialog,IDC_SOUNDCARDNAME,sound_devicename(current_sound_device));
        SetDlgItemText(hMainDialog,IDC_SOUNDCARDFORMAT,sound_description(current_sound_format));
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
      else if( wparam == IDCANCEL )
      {
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
     break;
  }
  return FALSE;
}


struct control 
    { int id_slider, id_edit; HWND h_slider, h_edit; int *var; int min,max, old; } ctls[] ={
	{IDC_SLIDER_COMPRESS,IDC_COMPRESS, 0,0,&target_dB,-20,0,0},
	{IDC_SLIDER_RISETIME,IDC_RISETIME,0,0,&vc_risetime,1,150,0},
	{IDC_SLIDER_FALLTIME,IDC_FALLTIME,0,0,&vc_falltime,1,150,0},
	{IDC_SLIDER_NOISEGAIN,IDC_NOISEGAIN,0,0,&vc_noiseattenuation,-20,0,0},
	{IDC_SLIDER_HIGH,IDC_AG_HIGH,0,0,&ag_high_dB,-20,0,0},
	{IDC_SLIDER_LOW,IDC_AG_LOW,0,0,&ag_low_dB,-60,-10,0},
	{IDC_SLIDER_SILENCE,IDC_AG_SILENCE,0,0,&ag_silence_dB,-100,-20,0},
	{IDC_SLIDER_STEP,IDC_AG_STEP,0,0,&ag_step,0,20,0}
};

#define NUM_CONTROLS (sizeof(ctls)/sizeof(struct control))

void controls_init(HWND hDlg)
{ int i;
  for (i=0;i<NUM_CONTROLS;i++)
  { struct control *p=ctls+i;
    int var;
	p->h_edit=GetDlgItem(hDlg,p->id_edit);
    p->h_slider=GetDlgItem(hDlg,p->id_slider);
	var = *p->var;
	if ((p->max<=0 && var>0 )|| (p->min>=0 && var<0 ))
	  *p->var = var = -var; /* correct sign errors */
	if (var > p->max) *p->var = var= p->max;
	else if (var < p->min) *p->var = var= p->min;
    SetDlgItemInt(hDlg,p->id_edit,var,TRUE);
	SendMessage(p->h_slider,TBM_SETRANGE,FALSE,MAKELONG(0,p->max - p->min));
	SendMessage(p->h_slider,TBM_SETPOS,TRUE,var-p->min);
	p->old=var;
  }
}
void controls_restore(void)
{ int i;
  for (i=0;i<NUM_CONTROLS;i++)
  { struct control *p=ctls+i;
    *p->var = p->old;
  }
}

int slider_update(HWND hDlg,HWND h_slider, int pos)
{ int i;
  for (i=0;i<NUM_CONTROLS;i++)
  { struct control *p=ctls+i;
    if (h_slider==p->h_slider)
    { *p->var = p->min+pos;
	  SetDlgItemInt(hDlg,p->id_edit,*p->var,TRUE);
	  return 1;
	}
  }
  return 0;
}
int edit_update(HWND hDlg,HWND h_edit)
{ int i;
  for (i=0;i<NUM_CONTROLS;i++)
  { struct control *p=ctls+i;
    if (h_edit==p->h_edit)
	{ int var =GetDlgItemInt(hDlg,p->id_edit,NULL,TRUE);
	  if (var > p->max) 
	  { var= p->max;
	    SetDlgItemInt(hDlg,p->id_edit,var,TRUE);
	  }
	  else if (var < p->min)
	  { var= p->min;
	    SetDlgItemInt(hDlg,p->id_edit,var,TRUE);
	  }
	  if (*p->var == var) return 0;
      *p->var = var;
	  SendMessage(p->h_slider,TBM_SETPOS,TRUE,var-p->min);
	  return 1;
	}
  }
  return 0;
}

void controls_apply(void)
{ set_ag_levels();
  set_volume_compression_parameters(target_dB, vc_risetime, vc_falltime, -ag_silence_dB, vc_noiseattenuation);
  meter_format();
}

BOOL APIENTRY   
VolumeCompressionDialogProc( HWND hDlg, UINT mId, WPARAM wparam, LPARAM lparam )
{ 
  static HWND h_ok;
  switch ( mId )
  { case WM_INITDIALOG:
      controls_init(hDlg);
	  h_ok = GetDlgItem(hDlg,IDOK);
      return TRUE;
    case WM_HSCROLL:
	{ int pos;
      if (LOWORD(wparam)==SB_THUMBTRACK )
	    pos = HIWORD(wparam);
	  else
	    pos = SendMessage((HWND)lparam,TBM_GETPOS,0,0);
	  if (slider_update(hDlg,(HWND)lparam,pos))
	    controls_apply();
	  return TRUE;
    }
    break;
  case WM_SYSCOMMAND:
    if( wparam == SC_CLOSE ) 
    { controls_restore();
      controls_apply();	  
      DestroyWindow(hDlg); 
	  hVolCompress = NULL; 
	  return TRUE; 
    }
    break;
  case WM_COMMAND:
    if(HIWORD(wparam)==EN_KILLFOCUS && edit_update(hDlg, (HWND)lparam)) 
    { controls_apply();
 	  return TRUE;
	}
	else if( wparam == IDOK)
	{ HWND h_focus = GetFocus();
	  if (h_focus==h_ok)
	  {	DestroyWindow(hDlg); 
	    hVolCompress = NULL; 
        return TRUE;
	  }
	  else if(edit_update(hDlg,h_focus))
		controls_apply();
    }
    else if( wparam == IDCANCEL )
    { controls_restore();
	  controls_apply();
      DestroyWindow(hDlg); 
	  hVolCompress = NULL; 
      return TRUE;
    }
    break;
  }
  return FALSE;
}



static 
int show_encoder_controlls( HWND hDlg, int choice)
{
  if (choice == IDC_ENCODERAVG)
	  {	ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_PRESETS),SW_HIDE);
	    ShowWindow(GetDlgItem(hDlg,IDC_TARGET_QUALITY),SW_HIDE);
	    ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_BITRATE),SW_SHOW);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_MINBITRATE),SW_SHOW);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_MAXBITRATE),SW_SHOW);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_PRESETS),SW_HIDE);
	    ShowWindow(GetDlgItem(hDlg,IDC_TXT_QUALITY),SW_HIDE);
	    ShowWindow(GetDlgItem(hDlg,IDC_TXT_BITRATE),SW_SHOW);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_MINBITRATE),SW_SHOW);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_MAXBITRATE),SW_SHOW);
		return TRUE;
	  }
	  else if (choice == IDC_ENCODERCONST)
	  {	ShowWindow(GetDlgItem(hDlg,IDC_TARGET_QUALITY),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_MINBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_MAXBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_PRESETS),SW_HIDE);
	    ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_BITRATE),SW_SHOW);
	    ShowWindow(GetDlgItem(hDlg,IDC_TXT_PRESETS),SW_HIDE);
	    ShowWindow(GetDlgItem(hDlg,IDC_TXT_QUALITY),SW_HIDE);
	    ShowWindow(GetDlgItem(hDlg,IDC_TXT_MINBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_MAXBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_BITRATE),SW_SHOW);
		return TRUE;
	  }
	  else if (choice == IDC_ENCODERVAR)
	  { ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_BITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_MINBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_MAXBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_PRESETS),SW_HIDE);
	   	ShowWindow(GetDlgItem(hDlg,IDC_TARGET_QUALITY),SW_SHOW);
	    ShowWindow(GetDlgItem(hDlg,IDC_TXT_PRESETS),SW_HIDE);
	    ShowWindow(GetDlgItem(hDlg,IDC_TXT_BITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_MINBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_MAXBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_QUALITY),SW_SHOW);
	    return TRUE;
	  }	  else if (choice == IDC_ENCODERPRE)
	  {	ShowWindow(GetDlgItem(hDlg,IDC_TARGET_QUALITY),SW_HIDE);
	    ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_BITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_MINBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_MAXBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_ENCODER_PRESETS),SW_SHOW);
	    ShowWindow(GetDlgItem(hDlg,IDC_TXT_QUALITY),SW_HIDE);
	    ShowWindow(GetDlgItem(hDlg,IDC_TXT_BITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_MINBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_MAXBITRATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_TXT_PRESETS),SW_SHOW);
	    return TRUE;
	  }
	  else
		  return FALSE;
}


BOOL APIENTRY   
EncoderDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{ static struct {char *name; int value;} presets[] =
	{
		{"Phone",		LQP_PHONE},
		{"Short Wave",	LQP_SW},
		{"AM Radio",		LQP_AM},
		{"FM Radio",		LQP_FM},
		{"Voice",		LQP_VOICE},
		{"Radio",		LQP_RADIO},
		{"Tape",			LQP_TAPE},
		{"HiFi",			LQP_HIFI},
		{"CD",			LQP_CD},
		{"Studio",		LQP_STUDIO},
		{NULL,			0}
	};

  switch ( message )
  { case WM_INITDIALOG:
      SetWindowText(hDlg, szEncoderName);
      SetDlgItemText(hDlg,IDC_ENCODERSITE,szEncoderSite);
      SetDlgItemInt(hDlg,IDC_ENCODER_BITRATE,bitrate,FALSE);
      SetDlgItemInt(hDlg,IDC_ENCODER_MINBITRATE,min_bitrate,FALSE);
      SetDlgItemInt(hDlg,IDC_ENCODER_MAXBITRATE,max_bitrate,FALSE);
      CheckRadioButton(hDlg,IDC_JOINT_STEREO, IDC_STEREO,
	          stereoflag ? IDC_STEREO : IDC_JOINT_STEREO);
	  { int id;
        if (encoder_bitmode == VBR)
		  id = IDC_ENCODERVAR;
		else if (encoder_bitmode == CBR)
		  id = IDC_ENCODERCONST;
	    else if (encoder_bitmode == PRE)
		  id = IDC_ENCODERPRE;
        else /* if (encoder_bitmode == PRE)	 and default */
		  id = IDC_ENCODERAVG;
        CheckRadioButton(hDlg,IDC_ENCODERCONST, IDC_ENCODERPRE,id);
		show_encoder_controlls(hDlg, id);
	  }

      CheckDlgButton(hDlg,IDC_ORIGINAL,originalflag);
      CheckDlgButton(hDlg,IDC_CRC,crcflag);
      CheckDlgButton(hDlg,IDC_COPYRIGHT,copyrightflag);
      CheckDlgButton(hDlg,IDC_DOWNSAMPLE,downsampleflag);
      CheckDlgButton(hDlg,IDC_RESERVOIR,reservoirflag);
      { int i,index; 
	    char name[2];
		name[1] = 0;
        for (i = 0; i< 10; i++)
        { name[0] = '0'+i;
          index = SendDlgItemMessage(hDlg,IDC_TARGET_QUALITY,CB_ADDSTRING,0,
                                (LPARAM)name);
          SendDlgItemMessage(hDlg,IDC_TARGET_QUALITY,CB_SETITEMDATA,index,
                             (LPARAM)(DWORD)i); 
		}
        SendDlgItemMessage(hDlg,IDC_TARGET_QUALITY,CB_SETCURSEL,targetquality,0); 
	  }	
      { int i,index; 
	    char name[2];
		name[1] = 0;
        for (i = 0; i< 10; i++)
        { name[0] = '0'+i;
          index = SendDlgItemMessage(hDlg,IDC_ENCODERQUALITY,CB_ADDSTRING,0,
                                (LPARAM)name);
          SendDlgItemMessage(hDlg,IDC_ENCODERQUALITY,CB_SETITEMDATA,index,
                             (LPARAM)(DWORD)i); 
		}
        SendDlgItemMessage(hDlg,IDC_ENCODERQUALITY,CB_SETCURSEL,encoderquality,0); 
	  }	
       { int i, index;
	    for (i=0;presets[i].name!=NULL;i++)
		{ index = SendDlgItemMessage(hDlg,IDC_ENCODER_PRESETS,CB_ADDSTRING,0,(LPARAM)presets[i].name);
          SendDlgItemMessage(hDlg,IDC_ENCODER_PRESETS,CB_SETITEMDATA,index,(LPARAM)(DWORD)presets[i].value); 
		}
	    for (i=0;presets[i].name!=NULL;i++)
        { if (preset==presets[i].value)
			{ SendDlgItemMessage(hDlg,IDC_ENCODER_PRESETS,CB_SELECTSTRING,-1,(LPARAM)presets[i].name); 
		      break;
			}
		}
	  }
	   return TRUE;
   case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
        { bitrate = GetDlgItemInt(hDlg,IDC_ENCODER_BITRATE,NULL,FALSE);
          SetDlgItemNumber(hMainDialog,IDC_ENCODERBITRATE,bitrate,FALSE);
		  min_bitrate = GetDlgItemInt(hDlg,IDC_ENCODER_MINBITRATE,NULL,FALSE);
          max_bitrate = GetDlgItemInt(hDlg,IDC_ENCODER_MAXBITRATE,NULL,FALSE);
		  { int index;
            index = SendDlgItemMessage(hDlg,IDC_TARGET_QUALITY,CB_GETCURSEL,0,0); 
	        targetquality = SendDlgItemMessage(hDlg,IDC_TARGET_QUALITY,CB_GETITEMDATA,index,0); 
		  }
		  { int index;
            index = SendDlgItemMessage(hDlg,IDC_ENCODERQUALITY,CB_GETCURSEL,0,0); 
	        encoderquality = SendDlgItemMessage(hDlg,IDC_ENCODERQUALITY,CB_GETITEMDATA,index,0); 
		  }
		  { int index;
            index = SendDlgItemMessage(hDlg,IDC_ENCODER_PRESETS,CB_GETCURSEL,0,0); 
	        preset = SendDlgItemMessage(hDlg,IDC_ENCODER_PRESETS,CB_GETITEMDATA,index,0); 
		  }
		  originalflag = IsDlgButtonChecked(hDlg,IDC_ORIGINAL);
          crcflag = IsDlgButtonChecked(hDlg,IDC_CRC);
          copyrightflag = IsDlgButtonChecked(hDlg,IDC_COPYRIGHT);
          downsampleflag = IsDlgButtonChecked(hDlg,IDC_DOWNSAMPLE);
          reservoirflag = IsDlgButtonChecked(hDlg,IDC_RESERVOIR);

          if (IsDlgButtonChecked(hDlg,IDC_JOINT_STEREO))
          { SetDlgItemText(hMainDialog,IDC_ENCODERMODE,"Joint Stereo");
            stereoflag = 0;
          }
          else if (IsDlgButtonChecked(hDlg,IDC_STEREO))
          { SetDlgItemText(hMainDialog,IDC_ENCODERMODE,"Stereo");
            stereoflag = 1;
          }

          if (IsDlgButtonChecked(hDlg,IDC_ENCODERCONST))
            encoder_bitmode = CBR;
          else if (IsDlgButtonChecked(hDlg,IDC_ENCODERVAR))
            encoder_bitmode = VBR;
          else if (IsDlgButtonChecked(hDlg,IDC_ENCODERPRE))
            encoder_bitmode = PRE;
          else
            encoder_bitmode = ABR;

         EndDialog(hDlg, TRUE);
          return TRUE;
      }
      else if( wparam == IDCANCEL )
      {
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
      else if (show_encoder_controlls(hDlg, LOWORD(wparam)))
		return TRUE;
     break;
  }
  return FALSE;
}

BOOL APIENTRY   
BroadcastDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{

  switch ( message )
  { case WM_INITDIALOG:
      SetDlgItemText(hDlg,IDC_THE_MOUNTPOINT,mountpoint);
      SetDlgItemText(hDlg,IDC_THE_SERVER,server);
      SetDlgItemInt(hDlg,IDC_THE_PORT,port,FALSE);
      SetDlgItemText(hDlg,IDC_THE_PASSWORD,password);
      SetDlgItemText(hDlg,IDC_THE_USER,user);
      SetDlgItemText(hDlg,IDC_THE_STREAMNAME,name);
      SetDlgItemText(hDlg,IDC_THE_GENRE,genre);
      SetDlgItemText(hDlg,IDC_THE_INFO_URL,info_url);
      SetDlgItemText(hDlg,IDC_THE_DESCRIPTION,description);
      SetDlgItemText(hDlg,IDC_THE_DUMPFILE,remotedumpfile);
      CheckDlgButton(hDlg,IDC_PRIVATE,!publicflag);
	  SetDlgItemInt(hDlg,IDC_THE_BUFFERSIZE,out_buffers,FALSE);
	  { int id;
        if (logintype == XAUDIO_LOGIN)
		  id = IDC_XAUDIO;
		else if (logintype == HTTP_LOGIN)
		  id = IDC_HTTP;
	    else if (logintype == ICY_LOGIN) 
		  id = IDC_ICY;
	    else /*if (logintype == UDP_LOGIN) */
		  id = IDC_UDP;
        CheckRadioButton(hDlg,IDC_XAUDIO, IDC_UDP,id);
	  }
      CheckDlgButton(hDlg,IDC_RC_ENABLE,autoreconnect);
      SetDlgItemInt(hDlg,IDC_RC_DELAY,reconnectdelay,FALSE);
      SetDlgItemInt(hDlg,IDC_RC_IDELAY,initialrcdelay,FALSE);
      SetDlgItemInt(hDlg,IDC_RC_COUNT,reconnectcount,FALSE);

      return TRUE;
   case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
        { GetDlgItemText(hDlg,IDC_THE_MOUNTPOINT,tmp_option,MAXTMPOPTION);
		  set_option(&mountpoint,tmp_option);
          SetDlgItemText(hMainDialog,IDC_SENDNAME,mountpoint);
          GetDlgItemText(hDlg,IDC_THE_SERVER,tmp_option,MAXTMPOPTION);
		  set_option(&server,tmp_option);
          port = GetDlgItemInt(hDlg,IDC_THE_PORT,NULL,FALSE);
          GetDlgItemText(hDlg,IDC_THE_PASSWORD,tmp_option,MAXTMPOPTION);
		  set_option(&password,tmp_option);
          GetDlgItemText(hDlg,IDC_THE_USER,tmp_option,MAXTMPOPTION);
		  set_option(&user,tmp_option);
          GetDlgItemText(hDlg,IDC_THE_STREAMNAME,tmp_option,MAXTMPOPTION);
		  set_option(&name,tmp_option);
          GetDlgItemText(hDlg,IDC_THE_GENRE,tmp_option,MAXTMPOPTION);
		  set_option(&genre,tmp_option);
          GetDlgItemText(hDlg,IDC_THE_INFO_URL,tmp_option,MAXTMPOPTION);
		  set_option(&info_url,tmp_option);          
          GetDlgItemText(hDlg,IDC_THE_DESCRIPTION,tmp_option,MAXTMPOPTION);
		  set_option(&description,tmp_option);
          GetDlgItemText(hDlg,IDC_THE_DUMPFILE,tmp_option,MAXTMPOPTION);
		  set_option(&remotedumpfile,tmp_option);
          publicflag = !IsDlgButtonChecked(hDlg,IDC_PRIVATE);
		  out_buffers = GetDlgItemInt(hDlg,IDC_THE_BUFFERSIZE,NULL,FALSE);
          if (IsDlgButtonChecked(hDlg,IDC_XAUDIO))
            logintype = XAUDIO_LOGIN;
          else if (IsDlgButtonChecked(hDlg,IDC_HTTP))
            logintype = HTTP_LOGIN;
          else if (IsDlgButtonChecked(hDlg,IDC_ICY))
            logintype = ICY_LOGIN;
		  else
            logintype = UDP_LOGIN;
		  reconnectdelay = GetDlgItemInt(hDlg,IDC_RC_DELAY,NULL,FALSE);
		  initialrcdelay = GetDlgItemInt(hDlg,IDC_RC_IDELAY,NULL,FALSE);
		  reconnectcount = GetDlgItemInt(hDlg,IDC_RC_COUNT,NULL,FALSE);
          autoreconnect = IsDlgButtonChecked(hDlg,IDC_RC_ENABLE);

		  EndDialog(hDlg, TRUE);
          return TRUE;
      }
      else if( wparam == IDCANCEL )
      {
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
     break;
  }
  return FALSE;
}

BOOL APIENTRY   
MessageDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
     return FALSE; 
     return TRUE;
    case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
  }
  return FALSE;
}

static HWND hMessage;

LONG CALLBACK
MainWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{ 
  switch( msg )
  { case WM_CREATE:
      /* create main dialog */ 
      { RECT Rect;
        int w,h;
        hMainDialog = CreateDialog(hInstance,MAKEINTRESOURCE(IDD_MAIN) ,hwnd,
              MainDialogProc);
        GetWindowRect(hMainDialog,&Rect);
		AdjustWindowRect(&Rect, WS_CAPTION | WS_BORDER | WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU ,TRUE);
        w = Rect.right-Rect.left; h = Rect.bottom-Rect.top;
        SetWindowPos(hwnd,HWND_TOP,0,0,w,h,SWP_NOMOVE);
      }
      /* initialize open file name structure */
      ocfn.lStructSize=sizeof(OPENFILENAME);
      ocfn.hwndOwner=hwnd;
      ocfn.lpstrFilter=(LPSTR)szConfigFilter;
      ocfn.nFilterIndex=1;
      ocfn.lpstrFile=(LPSTR)szConfigFileName;

      ocfn.nMaxFile=CONFIGFILEMAX;
      ocfn.lpstrDefExt="m3w";     
      hMessage = GetDlgItem(hMainDialog,IDC_MESSAGETICKER);
/*      hMessage = CreateDialog(hInstance,
              MAKEINTRESOURCE(IDD_MESSAGE), hwnd, 
              MessageDialogProc);                  */
      DragAcceptFiles(hwnd,TRUE);
	  SetTimer(hwnd,1,REFRESH_TIME,NULL);

    return 0;
    case WM_DROPFILES:
      DragQueryFile((HDROP)wparam, 0, szConfigFileName, CONFIGFILEMAX-1);
	  message(0,"Drop");
      load_configfile(szConfigFileName);
	return 0;
	
      case WM_MESSAGE:
      { 
#define TICKERLEN 90
        static char ticker[TICKERLEN] = {0};
        unsigned int len;
        len = strlen((char *)lparam);
        		if (len>=TICKERLEN-3)
          strncpy(ticker, (char *)lparam+len-TICKERLEN+3,TICKERLEN-3);
        else if (strlen(ticker)+len>= TICKERLEN-3)
        { memmove(ticker, ticker+len+3,TICKERLEN-len-3);
          strcat(ticker,".  ");
          strcat(ticker,(char*)lparam);
		}
		else
        { strcat(ticker,".  ");
          strcat(ticker,(char*)lparam);
		}

        SetWindowText(hMessage,ticker);
        return 0;
       }
	  case WM_ERRORMSG:
		  { static char error[30];
            sprintf(error,"Error in m3w (%d)", wparam);
			MessageBox(NULL,(char*)lparam,error,MB_ICONEXCLAMATION |MB_OK);
			error_count--;
		  }
   return 0;
  case WM_CLOSE:
	  KillTimer(hwnd,1);
      exit_sound();
      out_record_exit();
      exit_queue();
      bc_exit();
      break;
    case WM_DESTROY:
      PostQuitMessage( 0 );
      break;
    case MM_WIM_DATA:
      sound_buffer_data((LPWAVEHDR)lparam);
      return 0;
	case MM_MIXM_LINE_CHANGE:
      change_Mixer_Line((HMIXER)wparam,lparam);
	  return 0;
	case MM_MIXM_CONTROL_CHANGE:
      change_Mixer_Control((HMIXER)wparam,lparam);
      return 0;
   case WM_ENDPLAYER:
      end_play_file();
      return 0;
   case WM_SENDDISPLAY:
	  display_send_status(lparam);
	  return 0;
   case WM_TIMER:
           display_encoder_status();
    	   display_sound_status();
	       display_send_status(BC_UPDATE);
	       display_out_status();
	  return 0;
   case WM_COMMAND:
    { switch( wparam ) 
      {
      case ID_OPTIONS_SONDCARD:
        DialogBox(hInstance,MAKEINTRESOURCE(IDD_SOUNDCARD),hwnd,SoundDialogProc);
        return 0;
      case ID_OPTIONS_VOLUMECOMPRESSION:
      /*  DialogBox(hInstance,MAKEINTRESOURCE(IDD_VOLUMECOMPRESSION),hwnd,VolumeCompressionDialogProc); */
		if (!IsWindow(hVolCompress))
		{ hVolCompress=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_VOLUMECOMPRESSION),hwnd,VolumeCompressionDialogProc);
		  ShowWindow(hVolCompress, SW_SHOW); 
		}
		return 0;
      case ID_OPTIONS_INPUT:
		get_mp3_in_file();
        return 0;
      case ID_OPTIONS_OUTPUT:
       get_mp3_out_file();
       return 0;
      case ID_OPTIONS_BROADCAST :
        DialogBox(hInstance,MAKEINTRESOURCE(IDD_BROADCAST),hwnd,BroadcastDialogProc);
        return 0;
      case ID_OPTIONS_ENCODER:
        DialogBox(hInstance,MAKEINTRESOURCE(IDD_ENCODER),hwnd,EncoderDialogProc);
        return 0;
      case ID_OPTIONS_TAG:
           DialogBox(hInstance,MAKEINTRESOURCE(IDD_TAG),hwnd,TagDialogProc);
        return 0;
      case ID_OPTIONS_STARTUP:
           DialogBox(hInstance,MAKEINTRESOURCE(IDD_STARTUP),hwnd,StartupDialogProc);
		return 0;
	  case ID_OPTIONS_ERRORSASMESSAGES:
		   error_as_message = !error_as_message;
		   CheckMenuItem(GetMenu(hMainWindow),ID_OPTIONS_ERRORSASMESSAGES,
	               MF_BYCOMMAND|(error_as_message?MF_CHECKED:MF_UNCHECKED));
        return 0;
	  case ID_OPTIONS_DETACH:
		   CheckMenuItem(GetMenu(hMainWindow),ID_OPTIONS_DETACH,
	               MF_BYCOMMAND|(meter_detach()?MF_CHECKED:MF_UNCHECKED));
        return 0;
	  case ID_OPTIONS_MIXER:
		   {		 
		     STARTUPINFO si;
			 PROCESS_INFORMATION pi;
			 ZeroMemory( &si, sizeof(si) );
			 si.cb = sizeof(si);
			 ZeroMemory( &pi, sizeof(pi) );
			 if( !CreateProcess(NULL, "sndvol32.exe /r", NULL, NULL, FALSE,
		 		0, NULL, NULL, &si,	&pi ) )
				errormsg("Unable to start mixer (sndvol32.exe /r)",0);
		   }
		  return 0;

      case ID_HELP_ABOUT:
        DialogBox(hInstance,MAKEINTRESOURCE(IDD_ABOUT),hwnd,AboutDialogProc);
        return 0; 
	  case ID_HELP_CONFIGURATION:
        DialogBox(hInstance,MAKEINTRESOURCE(IDD_CONFIGURATION),hwnd,ConfigurationDialogProc);
        return 0; 
      case ID_HELP_M3W:
		{ HWND hh; 
		  if (programhelpfile==NULL)
             hh = HtmlHelp(hwnd,"m3w.chm",HH_DISPLAY_TOPIC,(DWORD_PTR)NULL) ;
		  else
             hh = HtmlHelp(hwnd,programhelpfile,HH_DISPLAY_TOPIC,(DWORD_PTR)NULL) ;
		}
        return 0;
      case ID_FILE_OPEN:
        ocfn.Flags=OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        if(! GetOpenFileName((LPOPENFILENAME)&ocfn))
            return 0;         // user canceled file open dialog
        SetWindowText(hwnd,ocfn.lpstrFile+ocfn.nFileOffset);
		message(0,"Open");
		load_configfile(szConfigFileName);
        return 0;
      case ID_FILE_SAVE:
        if (strlen(szConfigFileName) > 0)
		{ write_configfile(szConfigFileName);
          return 0; 
		}
        /* else fall through to SAVE AS */
      case ID_FILE_SAVEAS:
        ocfn.Flags=OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |OFN_HIDEREADONLY;
        if (!GetSaveFileName((LPOPENFILENAME)&ocfn))
          return 0;
        SetWindowText(hwnd,ocfn.lpstrFile+ocfn.nFileOffset);
		write_configfile(szConfigFileName);
        return 0;
      case ID_FILE_EXIT:
        PostMessage( hwnd, WM_CLOSE, 0, 0 );
        return 0;
     default:


        break;
      }
      break;
    }
    default:
      break;      
  }
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrevInstance, 
                      LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HWND hwnd;
    WNDCLASS  wc;
    char ClassName[] = "m3wClass";


    hInstance = hInst;


    InitCommonControls();
    error_code = GetClassInfo(NULL,ClassName,&wc);
    if (error_code != 0)
    {  errormsg("m3w already running",0);
       return FALSE;
    }

    wc.style         = 0;
    wc.lpfnWndProc   = MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = hm3wIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_M3W)); 
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU);
    wc.lpszClassName = ClassName;
    if (!RegisterClass(&wc) )
      return FALSE;



    if (!(hwnd = CreateWindow( "m3wClass", "m3w",
              WS_CAPTION | WS_BORDER | WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU 
			  ,
              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,     
              NULL, NULL, hInstance, NULL )))
      return FALSE;
    hMainWindow=hwnd;

   param_init();
   init_queue();
   init_encoder();
   init_sound(hInstance);
   bc_init();
   meter_format();
   set_volumecompression(vc_enable);
   if (minimizedflag)
     ShowWindow( hwnd,SW_SHOWMINIMIZED); 
   else
     ShowWindow( hwnd,SW_SHOW); 


   display_main_dialog_information(hMainDialog);
   UpdateWindow( hwnd );

#ifdef DEBUG
   message(0,"Debug");
#endif
   message(0, "m3w"); message(0, VERSIONSTR);

  if (listeningflag)
	 start_sound();
  else if (playingflag)
	  start_play_file();
  if (recordingflag)
	  out_record_start();
  if (broadcastingflag)
	  start_bc();
  if (meter_detached_flag)
       SendMessage(hMainWindow,WM_COMMAND,ID_OPTIONS_DETACH,0);

  while( GetMessage(&msg, NULL, 0, 0) )
      if (IsDialogMessage(hMainDialog,&msg))
		  continue;
	  else if (IsWindow(hVolCompress) && IsDialogMessage(hVolCompress,&msg))
		  continue;
	  else
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    return( msg.wParam );
}

int error_code = 0;

void errormsg(char *msg, int error_code)
{ if (error_as_message)
	{  message(0,msg);
       return;
	}
  if (error_count > 3)
    return; 
  if (hMainWindow==NULL)
  {   error_count++;
	  MessageBox(NULL,msg,"Error in m3w",MB_ICONEXCLAMATION |MB_OK);
	  error_count--;
  }
  else if (FALSE==PostMessage(hMainWindow,WM_ERRORMSG,(WPARAM)error_code,(LPARAM)msg))
    MessageBox(NULL,"Unable to post Error Message",msg,MB_ICONEXCLAMATION |MB_OK);
  else
	error_count++;
}
 

void fatal_error(char *message)
{   static char error[30];
	sprintf(error,"FATAL ERROR (%d)", error_code);
    MessageBox(NULL,message,error,MB_ICONEXCLAMATION |MB_OK);
    SendMessage( hMainWindow, WM_CLOSE, 0, 0 );
}


void message(int i, char *msg)
{ /* i!= 0 is for debugging only */
#ifndef DEBUG
  if (i != 0) return;
#endif
  if (FALSE==PostMessage(hMainWindow,WM_MESSAGE,(WPARAM)i,(LPARAM)msg))
  {  if (error_as_message) return;
	 error_code = GetLastError();
     errormsg(" unable to post message",error_code); 
  }
}
