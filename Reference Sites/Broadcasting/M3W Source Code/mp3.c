/* mp3.c $Revision: 1.1 $ $Date: 2009/05/11 09:02:51 $
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
#include "mp3.h"
  

static int get_mp3_header(unsigned char *p, int size, frame_header *f)
/* we know the first 8 bits are all 1, It is most likely a  mp3 frame 
   return 1 if it is a mp3 frame and set f.
   return 0 if it is not an mp3 frame
 */

{ unsigned long int head;
  unsigned int tmp;
  unsigned int samples;
  unsigned int slotsize;

/* MP3 FRAME FORMAT
   Information from: http://www.mp3-tech.org/programmer/frame_header.html

   header 4 Bytes
   crc	  2 Bytes (optional)
   audio data
*/
  if (size < 4) /* we are unable to test */
  { f->incomplete = 1;
    return 1;
  }
  head = (((((p[0] << 8) | p[1]) << 8) | p[2]) << 8) | p[3];
/*
   Header Format:   AAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM 
     bits        name         description
   A 11(31-21)   frame_sync   must be all 1
*/

  if ((head>>21) != 0x7FF)
	  return 0;
  f->tag = mp3;
/*
   B 2 (20-19)   version      00 - MPEG Version 2.5 (later extension of MPEG 2)
                              01 - reserved
                              10 - MPEG Version 2 (ISO/IEC 13818-3)
                              11 - MPEG Version 1 (ISO/IEC 11172-3) 

                              Note: MPEG Version 2.5 was added lately to the MPEG 2
                              standard. It is an extension used for very low bitrate files,
                              allowing the use of lower sampling frequencies. If your
                              decoder does not support this extension, it is recommended
                              for you to use 12 bits for synchronization instead of 11 bits. 

  both bits can be regarded as ID (bit 19) and IDext (bit 20)
  depending on the ID, the value of samples per frame is determined.
  ID = 0 means 576 samples ID = 1 means 1152 Samples (one repectively two granules) 

*/

  tmp = (head>>19) & 0x03;
  if (tmp == 0)      {f->mp3.version = 2; samples = 576; } 
  else if (tmp == 1) return 0;
  else if (tmp == 2) {f->mp3.version = 1; samples = 576; } 
  else               {f->mp3.version = 0; samples = 1152; } 

/*
   C 2 (18,17)   layer        Layer description
                                  00 - reserved
                                  01 - Layer III
                                  10 - Layer II
                                  11 - Layer I
  on the layer also depends the slotsize it is 4 bytes for layer 1 and 1 byte otherwise
*/
  tmp = (head>>17) & 0x03;
  if (tmp == 0)      return 0; 
  else if (tmp == 1) {f->mp3.layer = 2; slotsize = 1; }
  else if (tmp == 2) {f->mp3.layer = 1; slotsize = 1; }
  else               {f->mp3.layer = 0; slotsize = 4; }

/*
   D 1 (16)      crc          Protection bit
                                  0 - Protected by CRC (16bit CRC follows header)
                                  1 - Not protected
*/
   f->mp3.crc = !((head >> 16) & 0x01);
/*
   E 4 (15,12)   bitrate_index Bitrate index 0000 means free format 1111 means bad value
                                  NOTES: All values are in kbps
                                  V1 - MPEG Version 1
                                  V2 - MPEG Version 2 and Version 2.5
                                  L1 - Layer I
                                  L2 - Layer II
                                  L3 - Layer III
				  bitrate index == 00 means free format.
                                  The free bitrate must remain
                                  constant, and must be lower than the maximum allowed bitrate.
                                  Decoders are not required to support decoding of free bitrate
                                  streams.
                                  bitrate index = 1111 means that the value is unallowed. 

                                  MPEG files may feature variable bitrate (VBR). Each frame
                                  may then be created with a different bitrate. It may be used in
                                  all layers. Layer III decoders must support this method.
                                  Layer I & II decoders may support it. 

                                  For Layer II there are some combinations of bitrate and
                                  mode which are not allowed. (see documentation)
*/
{
static const unsigned int 
bitrate[3/*version*/][3/*layer*/][16/*bitrate_index*/] =
{{/*version 1*/
    /*layer 1*/	{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },
    /*layer 2*/ { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
    /*layer 3*/ { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 }},
 {/*version 2*/
    /*layer 1*/ { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
    /*layer 2*/ { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
    /*layer 3*/	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }},
 {/*version 2.5*/
    /*layer 1*/	{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
    /*layer 2*/	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
    /*layer 3*/	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }
	}
};
   tmp = (head>>12) & 0x0F;
   if (tmp == 0 || tmp==0x0F)
     return 0;
   f->mp3.bitrate = bitrate[f->mp3.version][f->mp3.layer][tmp];
}
/*
  F 2 (11,10) samplerate_index Sampling rate frequency index 
*/
{
static const unsigned int 
samplerate[3/*version*/][4/*samplerate_index*/] =
{
	{ 44100, 48000, 32000, 0 },
	{ 22050, 24000, 16000, 0 },
	{ 11025, 12000, 8000, 0 }
};
  tmp = (head>>10) & 0x03;
  if (tmp == 3)
    return 0;
  f->mp3.samplerate = samplerate[f->mp3.version][tmp];
}
/*
  G 1 (9)  padding      Padding bit
                        0 - frame is not padded
                        1 - frame is padded with one extra slot

                                  Padding is used to exactly fit the bitrate.As an example:
                                  128kbps 44.1kHz layer II uses a lot of 418 bytes and some
                                  of 417 bytes long frames to get the exact 128k bitrate. For
                                  Layer I slot is 32 bits long, for Layer II and Layer III slot is 8
                                  bits long.
*/
  f->mp3.padding = (head >> 9) & 0x01;
/*
  H 1 (8)    private    Private bit. This one is only informative.
*/
  f->mp3.private = (head >> 8) & 0x01;
/*
  I 2 (7,6)  mode       Channel Mode
                                  00 - Stereo
                                  01 - Joint stereo (Stereo)
                                  10 - Dual channel (2 mono channels)
                                  11 - Single channel (Mono)

                                  Note: Dual channel files are made of two independant mono
                                  channel. Each one uses exactly half the bitrate of the file.
                                  Most decoders output them as stereo, but it might not always
                                  be the case.
                                      One example of use would be some speech in two
                                  different languages carried in the same bitstream, and then an
                                  appropriate decoder would decode only the choosen
                                  language. 
*/
  f->mp3.mode = (head >> 6) & 0x03;
/*
  J 2 (5,4) mode_ext  Mode extension (Only used in Joint stereo) see documentation
*/
  f->mp3.mode_ext = (head >> 4) & 0x03;
/*
  K 1 (3)   copyright  Copyright
                                  0 - Audio is not copyrighted
                                  1 - Audio is copyrighted
*/
  f->mp3.copyright = (head >> 3) & 0x01;
/*
  L 1 (2)   original   Original
                                  0 - Copy of original media
                                  1 - Original media
*/
  f->mp3.original = (head >> 2) & 0x01;
/*
  M 2 (1,0) emphasis   Emphasis
                                  00 - none
                                  01 - 50/15 ms
                                  10 - reserved
                                  11 - CCIT J.17

*/
  f->mp3.emphasis = head & 0x03;
	
/* now we determine size and playing time */

  f->size = ((1000/8) * samples * f->mp3.bitrate / f->mp3.samplerate)
	        + f->mp3.padding * slotsize;

/*  f->size = 1;  just find them all  

*/

  f->time = f->size * 8 / f->mp3.bitrate;


  f->incomplete = (size < f->size);

  return 1;
}


static int get_lyrics_header(unsigned char *p, int size, frame_header *f)
/* we know the first byte is 'L', It is most likely a lyrics frame 
   return 1 if it is a frame 
   return 0 else
 */

{  int i;
	if (size < 10) /* we are unable to test */
	{ f->incomplete = 1;
      return 1;
	}
  /* we know *p == 'R' */
  if (p[0] == 'L' && p[1] == 'Y' && p[2] == 'R' && p[3] == 'I' && p[4] == 'C' && p[5] == 'S' &&
      p[6] == 'B' && p[7] == 'E' && p[8] == 'G' && p[9] == 'I' && p[10] == 'N')
  { f->time=0;
	f->tag=lyrics;
	i = 11;
	while (i+8<size)
	{ if (p[0] == 'L' && p[1] == 'Y' && p[2] == 'R' && p[3] == 'I' && p[4] == 'C' && p[5] == 'S' &&
      ((p[6] == 'E' && p[7] == 'N' && p[8] == 'D') ||
       (p[6] == '2' && p[7] == '0' && p[8] == '0')))
		break;
	  i++;
	}
    f->size = i+9;
	f->incomplete = (size < f->size);
	return 1;
  }
  else
	return 0;
}

static int get_id3v2_header(unsigned char *p, int size, frame_header *f)
/* we know the first byte is 'T', It is most likely a  id3 tag frame 
   return 1 if it is a id3 frame ans set f.
   return 0 if it is not an mp3 frame
 */

{ int footer;
  if (size < 10) /* we are unable to test */
	{ f->incomplete = 1;
      return 1;
	}
  /* we know *p == 'I' */
  if (p[0] == 'I' && p[1] == 'D' && p[2] == '3')
  { footer = (p[5]>>4) & 0x01;
	f->size=10 + ((((((p[6]<<7) | p[7]) << 7) | p[8]) <<7) | p[9]);
	if (footer) f->size = f->size + 10;
    f->time=0;
	f->tag=id3v2;
	f->incomplete = (size < f->size);
	return 1;
  }
  else
	return 0;
}

static int get_id3v1_header(unsigned char *p, int size, frame_header *f)
/* we know the first byte is 'T', It is most likely a  id3 tag frame 
   return 1 if it is a id3 frame ans set f.
   return 0 if it is not an mp3 frame
 */

{ if (size < 3) /* we are unable to test */
	{ f->incomplete = 1;
      return 1;
	}
  /* we know *p == 'T' */
  if (p[0] == 'T' && p[1] == 'A' && p[2] == 'G')
  { f->size=128;
    f->time=0;
	f->tag=id3v1;
	f->incomplete = (size < f->size);
	return 1;
  }
  else
	return 0;
}

static int get_riff_header(unsigned char *p, int size, frame_header *f)
/* we know the first byte is 'R', It is most likely a  riff header 
   return 1 if it is a riff header 
   return 0 else
 */

{ int i;
	if (size < 4) /* we are unable to test */
	{ f->incomplete = 1;
      return 1;
	}
  /* we know *p == 'R' */
  if (p[0] == 'R' && p[1] == 'I' && p[2] == 'F' && p[3] == 'F')
  { f->time=0;
	f->tag=riff;
	i = 4;
	while (i+3<size)
	{ if (p[i] == 'd' && p[i+1]=='a' && p[i+2] == 't' && p[i+3]=='a')
		break;
	  i++;
	}
    f->size = i+4;
	f->incomplete = (size < f->size);
	return 1;
  }
  else
	return 0;
}

static int get_xing_header(unsigned char *p, int size, frame_header *f)
/* we know the first byte is 'X' or 'I', It is most likely a  Xing frame 
   return 1 if it is a Xing header frame and set f.
   return 0 if it is not an xing frame
*/

{ if (size < 4) /* we are unable to test */
	{ f->incomplete = 1;
      return 1;
	}
  /* we know *p == 'X' or 'I' */
  if((p[0] == 'X' && p[1] == 'i' && p[2] == 'n' && p[3] == 'g') 
     ||
     (p[0] == 'I' && p[1] == 'n' && p[2] == 'f' && p[3] == 'o'))
  { f->size = 4;
	/* should be  ((h_id+1)*72000*h_bitrate) / pTagData->samprate; */
    f->time=0;
    f->tag=xing;
    f->incomplete = (size < f->size);
    return 1;
  }
  else
    return 0;
}


void get_header(unsigned char *p, int size, frame_header *f)
/* fill in a frame structure from the given buffer */
{  if (size < 1)
	{ f->incomplete = 1;
	  return;
	}
	else if (*p == 0xFF && get_mp3_header(p,size,f))
		return;
	else if (*p == 'T' && get_id3v1_header(p,size,f))
		return;
	else if (*p == 'I' && get_id3v2_header(p,size,f))
		return;
	else if (*p == 'R' && get_riff_header(p,size,f))
		return;
	else if (*p == 'X' && get_xing_header(p,size,f))
		return;
	else if (*p == 'I' && get_xing_header(p,size,f))
		return;
	else /* this is unknown */
	{ f->tag = unknown;
	  f->size = 1;
	  f->time = 0;
	  f->incomplete = (f->size > size);
	}
}

