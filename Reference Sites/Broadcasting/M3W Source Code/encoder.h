/* encoder.h $Revision: 1.2 $ $Date: 2009/05/18 10:25:25 $
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
typedef
struct {
char *buffer;
int max;
int size;
int offset;
} *mp3_buffer;

extern char szEncoderName[];
extern char szEncoderSite[];

extern int init_encoder(void);
extern void kill_encoder(void);
extern void start_encoder(void);
extern void stop_encoder(void);
extern void mp3_ready(mp3_buffer mp3_out);
extern void submit_encoder_input(LPWAVEHDR p);
extern LPWAVEHDR get_empty_wavbuffer(void);
extern unsigned int WaveBufferSize;


extern mp3_buffer new_mp3buffer(void);
extern void free_mp3buffer(mp3_buffer n);
extern mp3_buffer check_mp3buffer(mp3_buffer n);
extern void display_encoder_status(void);
extern void free_wavbuffer(LPWAVEHDR p);