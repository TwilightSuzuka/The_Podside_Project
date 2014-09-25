/* mp3.h $Revision: 1.1 $ $Date: 2009/05/11 09:02:51 $
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

typedef enum { mp3, id3v1,id3v2,lyrics,riff,xing, unknown } frame_tag;

typedef struct frame_header {
	int incomplete; /* set if the frame is not complete */
	int size;       /* size of the frame */
	int time;       /* playing time of this frame in mili sec*/
	frame_tag tag;  /* what kind of frame is this */
	union {
		struct {
		int layer;	
		int version;
		int crc;
		int padding;
		int private;
		int mode;
		int mode_ext;
		int copyright;
		int original;
		int emphasis;
		int bitrate;
		int samplerate; } mp3;
		struct {
			char *message; } id3;
	};

} frame_header;

extern void get_header(unsigned char *p, int size, frame_header *f);

