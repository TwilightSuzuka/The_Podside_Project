/* encoder.c $Revision: 1.3 $ $Date: 2009/06/25 14:28:43 $
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
#include<commctrl.h>
#include <stdio.h>
#include "main.h"
#include "BladeMP3EncDLL.h"
#include "encoder.h"
#include "sound.h"
#include "param.h"
#include "queue.h"
#include "resource.h"
#include "output.h"
#include "broadcast.h"


/* the LAME interface */
char szEncoderName[128];
char szEncoderSite[128];

static int encoder_stream_open = 0;
static double actual_bitrate=0.0;
static int actual_channels = 2;
static int actual_sample_rate = 11025;
static long int bytes_in=0, bytes_out=0;
unsigned int WaveBufferSize = 2*2*2*576*10; /*This is a decent bet. to contain at least 10 frames */
static int mp3BufferSize = 4096;
static int current_sound_buffers = 4; /* Number of buffers to use */
static int sendbuffersize = 4;

static BEINITSTREAM		beInitStream=NULL;
static BEENCODECHUNK		beEncodeChunk=NULL;
static BEDEINITSTREAM	beDeinitStream=NULL;
static BECLOSESTREAM		beCloseStream=NULL;
static BEVERSION		beVersion=NULL;
static BEWRITEVBRHEADER	beWriteVBRHeader=NULL;
static DWORD		dwSamples		=0;
static DWORD		dwMP3Buffer		=0;
static HBE_STREAM	hbeStream		=0;

int v1_valid[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256,320, 0};
int v2_valid[] = {8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};

static int round_valid(int version, int bitrate)
{ int *v;
  int br;
  if (version) 
    v = v1_valid; 
  else 
    v = v2_valid;
  br = *v;
  while (*v != 0 )
  { if (*v <= bitrate)
      br = *v;
    else
      break;
    v++;
  }
  return br;
}


static int InitLame(void)
{    
      BE_VERSION	Version ={0,};
      HINSTANCE	hDLL=NULL;

      /* Load lame_enc.dll library */
	hDLL=LoadLibrary("lame_enc.dll");
	if(hDLL==NULL)
       { fatal_error("Unable to load lame_enc.dll");
	  return 0;
       }

	/* Get Interface functions */
	beInitStream	= (BEINITSTREAM) GetProcAddress(hDLL, TEXT_BEINITSTREAM);
	beEncodeChunk	= (BEENCODECHUNK) GetProcAddress(hDLL, TEXT_BEENCODECHUNK);
	beDeinitStream	= (BEDEINITSTREAM) GetProcAddress(hDLL, TEXT_BEDEINITSTREAM);
	beCloseStream	= (BECLOSESTREAM) GetProcAddress(hDLL, TEXT_BECLOSESTREAM);
	beVersion		= (BEVERSION) GetProcAddress(hDLL, TEXT_BEVERSION);
	beWriteVBRHeader= (BEWRITEVBRHEADER) GetProcAddress(hDLL,TEXT_BEWRITEVBRHEADER);
	if(!beInitStream || !beEncodeChunk || !beDeinitStream || 
          !beCloseStream || !beVersion || !beWriteVBRHeader)
       { fatal_error("Unable to load all lame interfaces");
	  return 0;
       }

       beVersion(&Version);
	sprintf(szEncoderName, "Lame %u.%02u %u/%u/%u",
			Version.byMajorVersion, Version.byMinorVersion,
			Version.byDay, Version.byMonth, Version.wYear);
	strcpy(szEncoderSite,Version.zHomepage);
       SetDlgItemText(hMainDialog,IDC_ENCODERNAME,szEncoderName);
	return 1;
}


static int OpenLame(PWAVEFORMATEX wavefmtex)
/* must be called while encoder thread is waiting 
   returns 1 in case of success, else returns 0
  */
{  int mpeg1;
   int buffer_factor;
   BE_CONFIG beConfig={0,};

   if  (encoder_stream_open)
	   errormsg("Encoder already open",0);

  
  if (wavefmtex->nSamplesPerSec == 32000 ||
       wavefmtex->nSamplesPerSec == 44100 ||
       wavefmtex->nSamplesPerSec == 48000)
       mpeg1 = 1;
   else
       mpeg1 = 0;

  memset(&beConfig,0,sizeof(beConfig));
  beConfig.dwConfig = BE_CONFIG_LAME;
  beConfig.format.LHV1.dwStructVersion= 1;
  beConfig.format.LHV1.dwStructSize= sizeof(beConfig);

  actual_sample_rate = wavefmtex->nSamplesPerSec;
    beConfig.format.LHV1.dwSampleRate= actual_sample_rate; /* INPUT FREQUENCY */
  if (!downsampleflag)
    beConfig.format.LHV1.dwReSampleRate= 0;/* Let the encoder decide*/
  else
    beConfig.format.LHV1.dwReSampleRate= actual_sample_rate;
  if (wavefmtex->nChannels == 1)
  { beConfig.format.LHV1.nMode= BE_MP3_MODE_MONO;
    actual_channels = 1;
  }
  else if (stereoflag)
  { beConfig.format.LHV1.nMode= BE_MP3_MODE_STEREO;
    actual_channels = 2;
  }
  else
  { beConfig.format.LHV1.nMode= BE_MP3_MODE_JSTEREO;
    actual_channels = 2;
  }

  beConfig.format.LHV1.nPreset= LQP_NOPRESET;/* QUALITY PRESET SETTING */

  if (encoder_bitmode==VBR)
  { beConfig.format.LHV1.bEnableVBR= TRUE;/* USE VBR */
    beConfig.format.LHV1.nVBRQuality= targetquality;
    beConfig.format.LHV1.dwVbrAbr_bps = 0; 
    beConfig.format.LHV1.nVbrMethod = VBR_METHOD_DEFAULT;
  }  
  else if (encoder_bitmode==CBR)
  {  beConfig.format.LHV1.bEnableVBR= FALSE; /* DO NOT USE VBR */
     beConfig.format.LHV1.dwBitrate= round_valid(mpeg1,bitrate/1000);/* CONSTANT BIT RATE */
     beConfig.format.LHV1.nVbrMethod = VBR_METHOD_NONE;
  }
  else if (encoder_bitmode==PRE)
  {  beConfig.format.LHV1.nPreset= preset;/* QUALITY PRESET SETTING */
     if (!mpeg1 && (preset == LQP_CD || preset == LQP_STUDIO))
        beConfig.format.LHV1.nPreset= LQP_HIFI; 
		/* the others wont work since the requested bitrates are not supported */
  } else /*  if (encoder_bitmode==ABR) and default */
  { beConfig.format.LHV1.bEnableVBR= TRUE;/* changed for Version 3.93.1 */
    beConfig.format.LHV1.nVbrMethod = VBR_METHOD_ABR;
    /* not used */
    beConfig.format.LHV1.nVBRQuality= targetquality;
	/* but this is all important > 0*/
    beConfig.format.LHV1.dwVbrAbr_bps = bitrate; 
	if (min_bitrate != 0)
      beConfig.format.LHV1.dwBitrate= round_valid(mpeg1,min_bitrate/1000) ;/* MINIMUM BIT RATE */
    else
	  beConfig.format.LHV1.dwBitrate= 0; /* let the encoder decide */
	if (max_bitrate != 0)
      beConfig.format.LHV1.dwMaxBitrate= round_valid(mpeg1,max_bitrate/1000);/* MAXIMUM BIT RATE */
    else
      beConfig.format.LHV1.dwMaxBitrate= 0; /* let the encoder decide */
  }
  /* Hi Byte must be equal to not Lo Byte */
  beConfig.format.LHV1.nQuality = encoderquality | (((~encoderquality)&0xFF)<<8);

  if (mpeg1)
    beConfig.format.LHV1.dwMpegVersion= MPEG1; /* MPEG VERSION (I or II) */
  else
    beConfig.format.LHV1.dwMpegVersion= MPEG2; /* MPEG VERSION (I or II) */

  beConfig.format.LHV1.dwPsyModel= 0;/* USE DEFAULT PSYCHOACOUSTIC MODEL */ 
  beConfig.format.LHV1.dwEmphasis= 0;/* NO EMPHASIS TURNED ON */
  beConfig.format.LHV1.bOriginal= originalflag;/* SET ORIGINAL FLAG */
  beConfig.format.LHV1.bCRC= crcflag;/* INSERT CRC */
  beConfig.format.LHV1.bCopyright= copyrightflag;/* SET COPYRIGHT FLAG */
  beConfig.format.LHV1.bPrivate= TRUE;/* SET PRIVATE FLAG */
  beConfig.format.LHV1.bWriteVBRHeader= FALSE;/* NO XING VBR HEADER */

  beConfig.format.LHV1.bNoRes= reservoirflag;	/* No Bit resorvoir */

  error_code = beInitStream(&beConfig, &dwSamples, &dwMP3Buffer, &hbeStream);
  if(error_code != BE_ERR_SUCCESSFUL)
  {  errormsg("Error opening encoding stream",error_code);
     hbeStream=0;
     return 0;
  }
  actual_bitrate=bitrate;
  bytes_in = 0;
  bytes_out = 0;
  encoder_stream_open=1;

  /* aproximately 1 Buffer = 0.5 seconds */
#ifdef DEBUG
  buffer_factor =  (int)(0.1*actual_sample_rate*actual_channels/dwSamples  + 0.5);
#else
  buffer_factor =  (int)(0.5*actual_sample_rate*actual_channels/dwSamples  + 0.5);
#endif
  if (buffer_factor < 1) buffer_factor = 1;
  current_sound_buffers= 2 * in_buffers;
  if (current_sound_buffers < 4)
	  current_sound_buffers = 4;
  PostMessage(GetDlgItem(hMainDialog,IDC_INPROGRES), PBM_SETRANGE, 0, 
	                 MAKELPARAM(0,current_sound_buffers));
  { int tmp = dwMP3Buffer * buffer_factor;
    if (tmp > mp3BufferSize) mp3BufferSize= tmp;
  }
  sendbuffersize = 2 * out_buffers;
  if (sendbuffersize < 4)
	sendbuffersize = 4;
  PostMessage(GetDlgItem(hMainDialog,IDC_OUTPROGRES), PBM_SETRANGE, 0, 
       MAKELPARAM(0, sendbuffersize));
  return 1;
}

static void CloseLame(void)
{   beCloseStream(hbeStream);
	hbeStream = 0;
	encoder_stream_open=0;
}

/* the encoder thread */

/* sound buffers can be in two queues: empty and full */
static queue empty;
static queue full; 
static int encoder_alive = 0;
static int encoder_terminate = 0;
static HANDLE hThread;

/* the encoder thread reads the full queue, encodes buffers received until a NULL
  buffer is sent. It will then deinitialize the stream and start over.

*/

static DWORD EncoderThread(LPDWORD lpdwParam) 
{ DWORD mp3_written=0;
  int samples_encoded=0;
  int samples;
  double bitrate_history = 0;
  LPWAVEHDR encoder_in;
  mp3_buffer mp3_out;
  encoder_alive = 1;
  while (1) 
  {	
/*	PostMessage(hMainWindow,WM_SENDDISPLAY,0,ENCODER_IDLE); */
	encoder_in = NULL;
	while (encoder_in == NULL || encoder_in->dwBytesRecorded==0)
    { if (encoder_terminate) goto thread_exit;
      if (encoder_in!=NULL) enqueue(&empty,encoder_in);
	  if (is_empty(&full))
	    wait_nonempty(&full);
      encoder_in = dequeue(&full);  
	}
    samples_encoded = 0;
	if (!OpenLame(&wavefmtex))
       goto thread_exit;
	while (encoder_in != NULL)
    { if (encoder_terminate) goto thread_exit;
	  if (encoder_in->dwBytesRecorded/2 > dwSamples)
        samples = dwSamples;
      else
        samples = encoder_in->dwBytesRecorded/2;
      while(samples > 0)
      { if (encoder_terminate) goto thread_exit;
		mp3_out = get_empty_mp3buffer(); 
	    if (mp3_out == NULL) 
		{  message(0,"Dropping encoder data"); 
		   break; /* no space, no encoding, we drop this encoder_in */
		}
		error_code = beEncodeChunk(hbeStream, 
                               samples, 
                               (PSHORT)(encoder_in->lpData)+samples_encoded, 
                               mp3_out->buffer+mp3_out->size, &mp3_written);
        if(error_code != BE_ERR_SUCCESSFUL)
        {  errormsg("encoding failed",error_code);
		   break;
		}
        mp3_out->size = mp3_out->size+mp3_written;
        samples_encoded = samples_encoded+samples;
        bytes_in = bytes_in + samples*2;
        bytes_out = bytes_out + mp3_written;
	    if (bitrate_history < 0.97) bitrate_history = bitrate_history+0.02;
        actual_bitrate = actual_bitrate*bitrate_history+
                   ((1.0-bitrate_history)*8*mp3_written*actual_sample_rate*actual_channels)/ 
                         samples;

		mp3_ready(mp3_out);
        if (encoder_in->dwBytesRecorded/2 > dwSamples + samples_encoded)
          samples = dwSamples;
        else
          samples = encoder_in->dwBytesRecorded/2 - samples_encoded;
      }
	  enqueue(&empty,encoder_in);
	  if (is_empty(&full))
	    wait_nonempty(&full);
      encoder_in = dequeue(&full); 
	  samples_encoded = 0;
    }  
thread_exit:
	if (encoder_in != NULL) enqueue(&empty,encoder_in);
	if (encoder_stream_open )
	{  mp3_out = get_empty_mp3buffer(); 
	   if (mp3_out == NULL) 
	   {  message(0,"Dropping encoder data"); 
	      continue; /* no space, no encoding, we drop this encoder_in */
	   }
       error_code = beDeinitStream(hbeStream, mp3_out->buffer+mp3_out->size, &mp3_written);
 	   if(error_code != BE_ERR_SUCCESSFUL)
	      errormsg("finalizing mp3 stream failed",error_code);
       mp3_out->size = mp3_out->size+mp3_written;
       bytes_out = bytes_out + mp3_written;
	   mp3_ready(mp3_out);
       CloseLame();
	} 
	if (encoder_terminate)
	{  encoder_alive = 0;
	   ExitThread(0);
       return 0;
	}
  } 
}


/* The encoder thread interface */

int init_encoder(void)
/* called at startup */
{  DWORD dwThreadId;
   create_queue(&empty);
   create_queue(&full);
   InitLame();
   hThread = CreateThread(
        NULL,                        /* no security attributes        */
        0,                           /* use default stack size        */
        (LPTHREAD_START_ROUTINE) EncoderThread, /* thread function       */
        NULL,                        /* argument to thread function   */
        0,                           /* use default creation flags    */
        &dwThreadId);                /* returns the thread identifier */
   if (hThread == NULL)
   {  fatal_error("Unable to create encoder thread"); return 0;}
   return 1;
}



void stop_encoder(void)
/* the soft way to stop the encoder, it continues to empty buffers */
{ submit_encoder_input(NULL); /* signal end of batch */
}

/* when encoding is done we flush the buffers */
static void flush_encoder_input(void)
{ /* flush full buffers */
  LPWAVEHDR p;
  while (!is_empty(&full))
  {  p = dequeue(&full);
     if (p) enqueue(&empty,p);
  }
}



void terminate_encoder(void)
/* the hard way to stop it immediately */
{ flush_encoder_input();
  stop_encoder();
}

void start_encoder(void)
{ 
  terminate_encoder(); /* just in case it is still running */
}



/* killing the encoder: when encoding is done we flush the buffers */
void free_wavbuffer(LPWAVEHDR p)
{ if (p==NULL)
    return;
  unprepare_sound_buffer(p);
  if (p->lpData != NULL)
    free(p->lpData);
  free(p);
}

static void flush_encoder_empty(void)
{ /* flush empty buffers */
  while (!is_empty(&empty))
    free_wavbuffer(dequeue(&empty));
}

void kill_encoder(void)
{ encoder_terminate =1;
  terminate_encoder();
  error_code = WaitForSingleObject(hThread,2000);
  if (error_code != WAIT_OBJECT_0)
      errormsg("Unable to kill encoder",error_code);
  flush_encoder_empty();
}

/* we want to know the active buffers for display purpose */

void display_encoder_status(void)
{  int i;
   SetDlgItemNumber(hMainDialog,IDC_ENCODERIN,bytes_in,FALSE);
   SetDlgItemNumber(hMainDialog,IDC_ENCODEROUT,bytes_out,FALSE);
   SetDlgItemNumber(hMainDialog,IDC_ENCODERTRUERATE,(int)actual_bitrate,FALSE);
   i = queue_size(&full);
   SendDlgItemMessage(hMainDialog,IDC_INPROGRES, PBM_SETPOS,i>1?i-1:0,0);
}

/* we need new allocated buffers */
static int n_buffers = 0;

mp3_buffer new_mp3buffer(void)
{ 
  char *p;
  mp3_buffer n;
  if (n_buffers >= sendbuffersize)
  { message(0, "mp3 buffer limit reached");
    return NULL;
  }
  n = malloc(sizeof(*n));
  p = malloc(mp3BufferSize);
  if (p==NULL || n == NULL)
  {  fatal_error("Out of memory (mp3 buffer)");
     return NULL;
  }
  n->buffer = p;
  n->max = mp3BufferSize;
  n->offset = 0;
  n->size = 0;
  n_buffers++;
  return n;
}

mp3_buffer check_mp3buffer(mp3_buffer n)
{ if (n->max < mp3BufferSize)
  { free(n->buffer);
    n->buffer = malloc(mp3BufferSize);
    if ( n->buffer == NULL)
    {  fatal_error("Out of memory (mp3 buffer)");
       free(n);
       n_buffers--;
       return NULL;
    }
    n->max = mp3BufferSize;
  }
  n->offset = 0;
  n->size = 0;
  return n;
}

void free_mp3buffer(mp3_buffer n)
{ if (n==NULL)
    return;
  if (n->buffer!=NULL)
    free(n->buffer);
  free(n);
  n_buffers--;
}

static LPWAVEHDR new_wavbuffer(void)
{ LPWAVEHDR p;
  p = malloc(sizeof(WAVEHDR));
  if (p==NULL)
  {  errormsg("Out of Memory (wave header)",0);
     return NULL;
  }
  memset(p, 0, sizeof(WAVEHDR) );
  p->dwBufferLength = WaveBufferSize;
  p->lpData = malloc(WaveBufferSize);
  if (p->lpData == NULL)
  { free(p);
    errormsg("Out of memory (pcm buffer).",0 );
    return NULL;
  }
  prepare_sound_buffer(p);
  return p;  

}


/* the soundcard, once in a while needs a buffer for filling it */
LPWAVEHDR get_empty_wavbuffer(void)
{ 
  if (!is_empty(&empty)) /* if there is a prepared buffer take it*/
    return (LPWAVEHDR)dequeue(&empty);
  if (queue_size(&full) < current_sound_buffers)
    return new_wavbuffer();
  message(0,"wav buffer limit reached");
  return NULL;
}

/* after the sound data is encoded, the encoder thread releases it */

/* after the sound data is filled the soundcard submits it */
void submit_encoder_input(LPWAVEHDR p)
{ enqueue(&full, p); 
}  
    

void mp3_ready(mp3_buffer mp3_out)
{
  if (mp3_out!=NULL)
  { out_mp3_data(mp3_out);
    bc_write(mp3_out);  
  }
}



