/* id3tag.c $Revision: 1.1 $ $Date: 2009/05/11 09:02:51 $
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
#include <stdio.h>
#include "main.h"
#include "resource.h"
#include "encoder.h"
#include "broadcast.h"
#include "id3tag.h"

#if 0 
/* currently not in use */
/* this is from the lame source code see http://www.sulaco.org */

static unsigned char *
set_4_byte_value(unsigned char *bytes, unsigned long value)
{
    int index;
    for (index = 3; index >= 0; --index) {
        bytes[index] = (unsigned char)(value & 0xfful);
        value >>= 8;
    }
    return bytes + 4;
}

#define FRAME_ID(a, b, c, d) \
    ( ((unsigned long)(a) << 24) \
    | ((unsigned long)(b) << 16) \
    | ((unsigned long)(c) <<  8) \
    | ((unsigned long)(d) <<  0) )
#define TITLE_FRAME_ID FRAME_ID('T', 'I', 'T', '2')
#define ARTIST_FRAME_ID FRAME_ID('T', 'P', 'E', '1')
#define ALBUM_FRAME_ID FRAME_ID('T', 'A', 'L', 'B')
#define YEAR_FRAME_ID FRAME_ID('T', 'Y', 'E', 'R')
#define COMMENT_FRAME_ID FRAME_ID('C', 'O', 'M', 'M')
#define TRACK_FRAME_ID FRAME_ID('T', 'R', 'C', 'K')
#define GENRE_FRAME_ID FRAME_ID('T', 'C', 'O', 'N')

static unsigned char *
set_frame(unsigned char *frame, unsigned long id, const char *text,
    size_t length)
{
    if (length) {
        frame = set_4_byte_value(frame, id);
        /* Set frame size = total size - header size.  Frame header and field
         * bytes include 2-byte header flags, 1 encoding descriptor byte, and
         * for comment frames: 3-byte language descriptor and 1 content
         * descriptor byte */
        frame = set_4_byte_value(frame, ((id == COMMENT_FRAME_ID) ? 5 : 1)
                + length);
        /* clear 2-byte header flags */
        *frame++ = 0;
        *frame++ = 0;
        /* clear 1 encoding descriptor byte to indicate ISO-8859-1 format */
        *frame++ = 0;
        if (id == COMMENT_FRAME_ID) {
            /* use id3lib-compatible bogus language descriptor */
            *frame++ = 'X';
            *frame++ = 'X';
            *frame++ = 'X';
            /* clear 1 byte to make content descriptor empty string */
            *frame++ = 0;
        }
        while (length--) {
            *frame++ = *text++;
        }
    }
    return frame;
}

int output_v2_tag(char *title,
				  char *artist,
				  char *album,
				  int year,
				  char *comment,
				  int track,
				  int genre)
				  /* put a version2 id3 tag in the output stream
				  return 1 on success, 0 on failure */
{ mp3_buffer out;
  int title_length, artist_length, album_length, comment_length;
  int tag_size;
  char year_str[5];
  int year_length;
  char track_str[3];
  int track_length;
  char genre_str[6];
  int genre_length;
  unsigned char *p;
  int adjusted_tag_size;

  title_length = strlen(title);
  artist_length = strlen(artist);
  album_length = strlen(album);
  comment_length = strlen(comment);

/* calulate size of tag starting with 10-byte tag header */
  tag_size = 10;
  if (title_length) {
	/* add 10-byte frame header, 1 encoding descriptor byte ... */
	tag_size += 11 + title_length;
  }
  if (artist_length) {
	tag_size += 11 + artist_length;
  }
  if (album_length) {
	tag_size += 11 + album_length;
  }
  if (year>0)
  { year_length = sprintf(year_str, "%d", year);
    tag_size += 11 + year_length;
  }
  else 
    year_length = 0;
  if (comment_length) {
   /* add 10-byte frame header, 1 encoding descriptor byte,
	* 3-byte language descriptor, 1 content descriptor byte ... */
	tag_size += 15 + comment_length;
  }
  if (track>0) {
	track_length = sprintf(track_str, "%d", track);
	tag_size += 11 + track_length;
  } else {
	track_length = 0;
  }
  if (genre != GENRE_NUM_UNKNOWN) {
	genre_length = sprintf(genre_str, "(%d)", genre);
	tag_size += 11 + genre_length;
  } else {
	genre_length = 0;
  }
  if (0) {
	/* add 128 bytes of padding */
	tag_size += 128;
  }
  out = get_empty_mp3buffer();
  if (out == NULL || out->max< tag_size)
    return 0;
  memset(out->buffer,0,tag_size);
  out->size=tag_size;
  out->offset=0;
  p = out->buffer;
/* set tag header starting with file identifier */
  *p++ = 'I'; *p++ = 'D'; *p++ = '3';
/* set version number word */
  *p++ = 3; *p++ = 0;
/* clear flags byte */
  *p++ = 0;
/* calculate and set tag size = total size - header size */
  adjusted_tag_size = tag_size - 10;
/* encode adjusted size into four bytes where most significant 
* bit is clear in each byte, for 28-bit total */
  *p++ = (adjusted_tag_size >> 21) & 0x7fu;
  *p++ = (adjusted_tag_size >> 14) & 0x7fu;
  *p++ = (adjusted_tag_size >> 7) & 0x7fu;
  *p++ = adjusted_tag_size & 0x7fu;

/*
* NOTE: The remainder of the tag (frames and padding, if any)
* are not "unsynchronized" to prevent false MPEG audio headers
* from appearing in the bitstream.  Why?  Well, most players
* and utilities know how to skip the ID3 version 2 tag by now
* even if they don't read its contents, and it's actually
* very unlikely that such a false "sync" pattern would occur
* in just the simple text frames added here.
*/

/* set each frame in tag */
  p = set_frame(p, TITLE_FRAME_ID, title, title_length);
  p = set_frame(p, ARTIST_FRAME_ID,artist, artist_length);
  p = set_frame(p, ALBUM_FRAME_ID, album, album_length);
  p = set_frame(p, YEAR_FRAME_ID, year_str, year_length);
  p = set_frame(p, COMMENT_FRAME_ID, comment, comment_length);
  p = set_frame(p, TRACK_FRAME_ID, track_str, track_length);
  p = set_frame(p, GENRE_FRAME_ID, genre_str, genre_length);
/* clear any padding bytes */
  memset(p, 0, tag_size - (p - out->buffer));

  mp3_ready(out);
  return 1;
}

/*not used v2 tags */
#endif
static const char *const genre_names[] =
{
    /*
     * NOTE: The spelling of these genre names is identical to those found in
     * Winamp and mp3info.
     */
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge",
    "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B",
    "Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
    "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
    "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical", "Instrumental",
    "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "Alt. Rock",
    "Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop",
    "Instrumental Rock", "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
    "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy",
    "Cult", "Gangsta Rap", "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
    "Native American", "Cabaret", "New Wave", "Psychedelic", "Rave",
    "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz",
    "Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock", "Folk",
    "Folk/Rock", "National Folk", "Swing", "Fast-Fusion", "Bebob", "Latin",
    "Revival", "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
    "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
    "Big Band", "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech",
    "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass",
    "Primus", "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
    "Punk Rock", "Drum Solo", "A Cappella", "Euro-House", "Dance Hall",
    "Goa", "Drum & Bass", "Club-House", "Hardcore", "Terror", "Indie",
    "BritPop", "Negerpunk", "Polsk Punk", "Beat", "Christian Gangsta Rap",
    "Heavy Metal", "Black Metal", "Crossover", "Contemporary Christian",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
    "Synthpop"
};

#define GENRE_NAME_COUNT \
    ((int)(sizeof genre_names / sizeof (const char *const)))

#define GENRE_NUM_UNKNOWN 255

void init_genre_dialog(HWND hDlg)
{ int i, index;
  for (i = 0; i< GENRE_NAME_COUNT; i++)
  { 
     index = SendDlgItemMessage(hDlg,IDC_TAG_GENRE,CB_ADDSTRING,0,
                                (LPARAM)genre_names[i]);
     SendDlgItemMessage(hDlg,IDC_TAG_GENRE,CB_SETITEMDATA,index,
                             (LPARAM)(DWORD)i); 
  }
}

int output_v1_tag(char *title,
				  char *artist,
				  char *album,
				  int year,
				  char *comment,
				  int track,
				  int genre)
/* put a version1 id3 tag in the output stream
   return 1 on success, 0 on failure */
{  mp3_buffer out;
   unsigned char tag[128];

   unsigned char *p = tag;
  out = get_empty_mp3buffer();
  if (out == NULL || out->max< 128)
	return 0;
  memset(out->buffer,0,128);
  out->size=128;
  out->offset=0;
  out->buffer[0] = 'T';
  out->buffer[1] = 'A';
  out->buffer[2] = 'G';
  if (title!=NULL)
    strncpy(out->buffer+3,title,30);
  if (artist!=NULL)
    strncpy(out->buffer+33,artist,30);
  if (album!=NULL)
    strncpy(out->buffer+63,album,30);
  if (year>0)
    sprintf(out->buffer+93,"%d",year);
  if (comment!=NULL);
    strncpy(out->buffer+97,album,30);
  if (track>0) /* this is a version 1.1 tag */
  { out->buffer[125] = 0;
    out->buffer[126] = track;
	out->buffer[127] = genre;
  }
  mp3_ready(out);
  return 1;
}