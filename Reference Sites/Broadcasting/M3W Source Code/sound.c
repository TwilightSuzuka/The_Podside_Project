/* sound.c $Revision: 1.6 $ $Date: 2011/06/05 00:57:02 $
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
#include <stdio.h>
#include <math.h>
#include "sound.h"
#include "main.h"
#include "queue.h"
#include "encoder.h"
#include "param.h"
#include "resource.h"
#include "play.h"
#include "autogain.h"
#include "volumecompression.h"
#include "meter.h"

#define WAVE_FORMAT_48M08       0x00001000       /* 48   kHz, Mono,   8-bit  */
#define WAVE_FORMAT_48S08       0x00002000       /* 48   kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_48M16       0x00004000       /* 48   kHz, Mono,   16-bit */
#define WAVE_FORMAT_48S16       0x00008000       /* 48   kHz, Stereo, 16-bit */
#define WAVE_FORMAT_96M08       0x00010000       /* 96   kHz, Mono,   8-bit  */
#define WAVE_FORMAT_96S08       0x00020000       /* 96   kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_96M16       0x00040000       /* 96   kHz, Mono,   16-bit */
#define WAVE_FORMAT_96S16       0x00080000       /* 96   kHz, Stereo, 16-bit */


static struct { int id; int fq; int channels; int bytes; char *description; }
dwFormats[MAX_SOUND_FORMATS] = {
{WAVE_FORMAT_1M08,11025,1,1,"11,025 kHz, mono, 8-bit"},
{WAVE_FORMAT_1M16,11025,1,2,"11.025 kHz, mono, 16-bit"},
{WAVE_FORMAT_1S08,11025,2,1,"11.025 kHz, stereo, 8-bit"},
{WAVE_FORMAT_1S16,11025,2,2,"11.025 kHz, stereo, 16-bit"},
{WAVE_FORMAT_2M08,22050,1,1,"22.05 kHz, mono, 8-bit"},
{WAVE_FORMAT_2M16,22050,1,2,"22.05 kHz, mono, 16-bit"},
{WAVE_FORMAT_2S08,22050,2,1,"22.05 kHz, stereo, 8-bit"},
{WAVE_FORMAT_2S16,22050,2,2,"22.05 kHz, stereo, 16-bit"},
{WAVE_FORMAT_4M08,44100,1,1,"44.1 kHz, mono, 8-bit"},
{WAVE_FORMAT_4M16,44100,1,2,"44.1 kHz, mono, 16-bit"},
{WAVE_FORMAT_4S08,44100,2,1,"44.1 kHz, stereo, 8-bit"},
{WAVE_FORMAT_4S16,44100,2,2,"44.1 kHz, stereo, 16-bit"},
{WAVE_FORMAT_48M08,48000,1,1,"48 kHz, mono, 8-bit"},
{WAVE_FORMAT_48M16,48000,1,2,"48 kHz, mono, 16-bit"},
{WAVE_FORMAT_48S08,48000,2,1,"48 kHz, stereo, 8-bit"},
{WAVE_FORMAT_48S16,48000,2,2,"48 kHz, stereo, 16-bit"}
/* not supported by mp3
{WAVE_FORMAT_96M08,96000,1,1,"96 kHz, mono, 8-bit"},
{WAVE_FORMAT_96M16,96000,1,2,"96 kHz, mono, 16-bit"},
{WAVE_FORMAT_96S08,96000,2,1,"96 kHz, stereo, 8-bit"},
{WAVE_FORMAT_96S16,96000,2,2,"96 kHz, stereo, 16-bit"}
*/

};

static WAVEINCAPS wic;

double dB_to_factor(double dB)
{ return pow(10.0,0.05*dB);
}
double factor_to_dB(double f)
{   if (f <=0) return -100;
	return 20.0*log10(f);
}

int dB_to_level(double dB)
{ 
  return (int)(dB_to_factor(dB)*0x7FFF);
}


double level_to_dB(int level)
{ return factor_to_dB((double)level/(double)0x7FFF);
}


static int percent_to_dB(int percent)
{ 
  return (int)factor_to_dB(percent/100.0);
}
	


int current_sound_bytes = 0;
static int current_sound_fill =0;

int sound_maxdevices()
{ return waveInGetNumDevs();
}

char *sound_devicename(int card)
/* card between 0 and sound_maxdevice() */
{ 
 static char no_such_soundcard[] = "Undefined Soundcard"; 
 if (MMSYSERR_NOERROR != waveInGetDevCaps(card, &wic,sizeof(WAVEINCAPS)))
	return no_such_soundcard;
  else 
       return wic.szPname;
}

char *sound_description(int i)
/* i between 0 and MAX_SOUND_FORMATS -1 */
{ return dwFormats[i].description;
}

int sound_caps(int sound_card)
{  waveInGetDevCaps(sound_card, &wic,sizeof(WAVEINCAPS));
   return wic.dwFormats;
}

int sound_support(int caps, int format)
{ return (dwFormats[format].id & caps )
         && (dwFormats[format].bytes==2);  /* Lame supports only 16 bit formats */
}

  
WAVEFORMATEX wavefmtex;

int get_samples_per_sec(void)
{
	return wavefmtex.nSamplesPerSec;
}

int get_stereo_flag(void)
{
	return(wavefmtex.nChannels == 2);
}

void  set_sound_format(int format)
{ memset( &wavefmtex, 0, sizeof(WAVEFORMATEX) );
  wavefmtex.wFormatTag = WAVE_FORMAT_PCM;
  wavefmtex.nChannels = dwFormats[format].channels;
  wavefmtex.nSamplesPerSec = dwFormats[format].fq;
  wavefmtex.nAvgBytesPerSec = dwFormats[format].fq
                              *dwFormats[format].channels
                              *dwFormats[format].bytes;
  wavefmtex.nBlockAlign = dwFormats[format].channels
                          *dwFormats[format].bytes;
  wavefmtex.wBitsPerSample = dwFormats[format].bytes*8;
  wavefmtex.cbSize = 0;
  if (wavefmtex.nChannels==2)
  { ShowWindow(GetDlgItem(hMainDialog,IDC_VOLUME2),SW_SHOW);
    ShowWindow(GetDlgItem(hMainDialog,IDC_CLIP2),SW_SHOW);
  }
  else
  { ShowWindow(GetDlgItem(hMainDialog,IDC_VOLUME2),SW_HIDE);
    ShowWindow(GetDlgItem(hMainDialog,IDC_CLIP2),SW_HIDE);
  }
	// Recompute the buffer size to keep things close to 5 buffers/second
	// and independant of sample rate and stereo/mono
	// Round up to a multiple of 8*576 (the mp3 frame size)
	WaveBufferSize = (((wavefmtex.nAvgBytesPerSec/5)+(8*576)-1)/(8*576))*(8*576);


}

static int sound_recording = 0;
static int opened = 0;

static HWAVEIN hwavein = 0;		/* device ID for recording input */

static void close_sound(void);



void prepare_sound_buffer(LPWAVEHDR p)
{ error_code =waveInPrepareHeader( hwavein, p, sizeof(WAVEHDR));
  if(  MMSYSERR_NOERROR != error_code)
    errormsg("Unable to prepare header",error_code );
}

void unprepare_sound_buffer(LPWAVEHDR p)
{  error_code = waveInUnprepareHeader( hwavein, p, sizeof(WAVEHDR));
  if( MMSYSERR_NOERROR != error_code )
      errormsg("Unable to unprepare header",error_code );
}

void fill_sound_buffer(LPWAVEHDR p)
{ error_code = waveInAddBuffer( hwavein, p, sizeof(WAVEHDR) );
  if ( error_code != MMSYSERR_NOERROR )
	errormsg("Error adding buffer.",error_code); 
  else
  	current_sound_fill++; 
}

/* the external procedures */

//static HANDLE sound_data_available;
//static HANDLE queue_access;
static int lost_sound = 0;

static void display_lost_sound(void)
{  static char msg[20];
   if (lost_sound <= 0)
    return;
   sprintf(msg,"Sound Gap (%d)",lost_sound);
   message(0,msg);
}



void init_sound(HANDLE hInstance)
{ set_ag_levels();
}

int submit_sound_buffer(void)
/* returns 1 if ok otherwise 0; */
{ if (!sound_recording)
    return 1;
  if (current_sound_fill < 2)
	{ LPWAVEHDR p;
	  p = get_empty_wavbuffer();
	  while((p != NULL) && (p->dwBufferLength != WaveBufferSize)) {
		  // Discard any old buffers allocated with a different length
		  free_wavbuffer(p);
		  p = get_empty_wavbuffer();
	  }
      if (p!=NULL) 
	  { fill_sound_buffer(p); 
		return 1;
	  }
	}
  return 0;

}

static void open_sound_record(void)
{ if (sound_recording)
    return; 
  if (!opened) 
  { 
    set_sound_format(current_sound_format);
    error_code = waveInOpen(
	 &hwavein, 
	(UINT)current_sound_device, 
	(LPCWAVEFORMATEX)&wavefmtex, 
       (DWORD)hMainWindow, (DWORD)NULL, CALLBACK_WINDOW|WAVE_ALLOWSYNC );
	if (WAVERR_BADFORMAT == error_code) {
		errormsg("Could not open the soundcard for recording.\n"
			     "Specified format is not supported.",error_code);
	} else if (MMSYSERR_BADDEVICEID == error_code) {
		errormsg("Could not open the soundcard for recording.\n"
			"Specified device does not exist.",error_code);
	} else if (MMSYSERR_ALLOCATED == error_code) {
		errormsg("Could not open the soundcard for recording.\n"
			"Specified resource is already allocated.",error_code);
	} else if (MMSYSERR_NOERROR !=  error_code) {
      errormsg("Could not open the soundcard for recording.",error_code);
	} else {
	  opened=1;
      current_sound_fill = 0;
      current_sound_bytes=0;
      lost_sound=0;
	  init_Mixer_device(hMainWindow,hwavein);
    }
  }
	// Need to update the volume compression in case the sample rate changes
	set_volume_compression_parameters(target_dB, vc_risetime, vc_falltime, -ag_silence_dB, vc_noiseattenuation);
}

void start_sound()
{  if (sound_recording)
   {  message(0, "already recording");
      return;
   }
   stop_play_file();
   open_sound_record();
   if (!opened)
	 return;
   sound_recording=1;
   SetDlgItemText(hMainDialog,IDC_LISTEN_INDICATOR,"LISTENING");
   init_Mixer_data();
   set_Mixer_data();
   if (!submit_sound_buffer() || !submit_sound_buffer())
   { errormsg("Unable to get buffers for sound recording",0);
	 stop_sound();
     return;
   }
   reset_volumecompression();
   start_encoder();
   error_code = waveInStart( hwavein );
   if( error_code != MMSYSERR_NOERROR)
   { errormsg("Unable to start sound recording.",error_code);
     stop_sound();
	 return;
   }
   meter_on();
}

 
void  sound_buffer_data(LPWAVEHDR p)
{
 current_sound_fill--;
  if (!submit_sound_buffer())
  { lost_sound++;
    display_lost_sound();
	fill_sound_buffer(p); /* reuse buffer without encoding it */
    return;
  }
  if (!sound_recording)
    p->dwBytesRecorded = 0;
  check_volume(p,wavefmtex.nChannels == 2, wavefmtex.nSamplesPerSec);
  volume_compression(p, wavefmtex.nChannels == 2, wavefmtex.nSamplesPerSec);

  submit_encoder_input(p);    
  current_sound_bytes = current_sound_bytes + p->dwBytesRecorded;

}


void stop_sound(void)
{ if (sound_recording)
  { sound_recording = 0;
    error_code = waveInReset(hwavein);
    if( error_code != MMSYSERR_NOERROR)
      errormsg("Unable to stop sound recording.",error_code);
    stop_encoder();
  }
  SetDlgItemText(hMainDialog,IDC_LISTEN_INDICATOR,"");
  meter_off();
}

static void close_sound(void)
{ if (sound_recording)
    stop_sound();
  if (!opened)
    return;
  close_Mixer_data();
  error_code = waveInClose(hwavein);
  if( error_code != MMSYSERR_NOERROR)
     errormsg("Unable to close sound recording.",error_code);
  opened=0;
}

void exit_sound(void)
{ if (sound_recording) stop_sound();
  kill_encoder();
  close_sound();
}

void change_sound_format(void)
{ if (sound_recording)
     errormsg("Recording in Progress will be stoped",0);
  close_sound();
}
  
