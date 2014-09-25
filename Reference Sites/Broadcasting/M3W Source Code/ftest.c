/* ftest.c $Revision: 1.1 $ $Date: 2009/05/11 09:02:51 $
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



/* for debuging: test outgoing frames */
#ifdef DEBUG

/* disable Warning about deprecated C Standard Function */
#pragma warning(disable : 4996)

#include <windows.h>
#include <stdio.h>
#include "encoder.h"
#include "main.h"

static const int tabsel_123 [2] [3] [16] = {
   { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
     {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
     {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

   { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};

static const long freqs[9] = { 44100, 48000, 32000,
                        22050, 24000, 16000,
                        11025, 12000,  8000 };


static int head_check(unsigned long head)
{
  /*
    look for a valid header.  
    if check_layer > 0, then require that
    nLayer = check_layer.  
   */
  if ((head >>8)==(('T'<<16) | ('A'<<8) | 'G'))
	  return TRUE;

  if( (head & 0xffe00000) != 0xffe00000) {
    /* syncword */
	return FALSE;
  }

  if( ((head>>12)&0xf) == 0xf) {
    /* bits 12,13,14,15 = 1111  invalid bitrate */
    return FALSE;
  }
  if( ((head>>10)&0x3) == 0x3 ) {
    /* bits 10,11 = 11  invalid sampling freq */
    return FALSE;
  }
  if( ((head>>17)&0x3) == 0 ) {
	  /* bits 17,18 == 00 invalid layer */
	return FALSE;
  }
  return TRUE;
}


static int get_framesize(unsigned long head)
{  int framesize, padding, bitrate_index,sampling_frequency,lsf,mpeg25,lay;

  if ((head >>8)==(('T'<<16) | ('A'<<8) | 'G'))
	  return 128;
 
   if( head & (1<<20) ) {
      lsf = (head & (1<<19)) ? 0x0 : 0x1;
      mpeg25 = 0;
    }
    else {
      lsf = 1;
      mpeg25 = 1;
    }
    lay = 4-((head>>17)&3);
    if(mpeg25) 
      sampling_frequency = 6 + ((head>>10)&0x3);
    else
      sampling_frequency = ((head>>10)&0x3) + (lsf*3);

    bitrate_index = ((head>>12)&0xf);
    padding   = ((head>>9)&0x1);
    if (lay == 1)
    {	framesize  = (long) tabsel_123[lsf][0][bitrate_index] * 12000;
	framesize /= freqs[sampling_frequency];
	framesize  = ((framesize+padding)<<2);
    } else
    if (lay == 2)
    {	framesize = (long) tabsel_123[lsf][1][bitrate_index] * 144000;
	framesize /= freqs[sampling_frequency];
	framesize += padding;
    } else
    if (lay == 3)
    { if (bitrate_index==0)
	return -1;
      else{
        framesize  = (long) tabsel_123[lsf][2][bitrate_index] * 144000;
        framesize /= freqs[sampling_frequency]<<(lsf);
        framesize = framesize + padding;
      }
    } 
    else
      return -1;
    return framesize;
}

#define BUFMASK 0xFFFF
#define BUFSIZE (BUFMASK+1)
static unsigned char buffer[BUFSIZE];
static int bufsize=0;

static void putbyte(unsigned char c)
{ buffer[bufsize&BUFMASK]=c;
  bufsize++;  
}

static unsigned char getbyte(int pos)
{ if (pos >=bufsize)
      return 0;
  return buffer[pos&BUFMASK];
}

static unsigned long gethead(int pos)
{ return (getbyte(pos)<<24) | (getbyte(pos+1)<<16) |
         (getbyte(pos+2)<<8) | getbyte(pos+3);
}

static int pos=0;
static unsigned long head=0;

void bc_test(mp3_buffer data,int mp3_size)
{ int framesize, i,k;
  static char msg[100];
  char *mp3_out;

  if (mp3_size<=0) return;
  mp3_out= data->buffer+data->offset;
  
  for (i=0;i<mp3_size;i++)
	  putbyte(mp3_out[i]);

  while (pos < bufsize)
  { 
    if (head_check(head))
    { framesize = get_framesize(head); 
      i = pos-3;
      k=1;
      while (k<5)
      { if (i+framesize>= bufsize)
	      return;
	    if (head_check(gethead(i+framesize)))
		{  k++; i=i+framesize; framesize=get_framesize(gethead(i));}
	    else
		  break;
	  }
      if (k==4)
      {  sprintf(msg,"Corrupted frame at %d (%lx,%lx) size:%d",i,head,gethead(i+framesize),framesize);
		 pos = i;
	     errormsg(msg,0); 
	  }
    }	 
   pos++;
   head = (head << 8) | getbyte(pos);
  }
}
#endif