/* sound.h $Revision: 1.2 $ $Date: 2009/06/08 15:08:20 $
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
#define MAX_SOUND_FORMATS (12+4)
#define MAXMETER 0x7FFF
extern int current_sound_bytes;
extern WAVEFORMATEX wavefmtex;

/* information procedures */
extern char *sound_devicename(int i);
extern int sound_support(int caps, int format);
extern char *sound_description(int format);
extern int sound_caps(int i);
extern int sound_maxdevices(void);
extern int sound_buffer_full(void); /* number of buffers filled */
extern void fill_sound_buffer(LPWAVEHDR p);
extern int get_samples_per_sec(void);
extern int get_stereo_flag(void);

/* these are internaly forced to be called strictly nested */
extern void init_sound(HANDLE);
extern void start_sound();
extern void stop_sound(void);
extern void exit_sound(void);

/* this is called in MM_WIM_DATA and WM_ENCODERREADY*/
extern void sound_buffer_data(LPWAVEHDR p);

/* this is called by the sound dialog */
extern void change_sound_format(void);

extern LPWAVEHDR get_sound_data(void);
extern void release_sound_data(LPWAVEHDR p);

/* called to prepare headers */
extern void prepare_sound_buffer(LPWAVEHDR p);
extern void unprepare_sound_buffer(LPWAVEHDR p);

extern double dB_to_factor(double dB);
extern double factor_to_dB(double f);
extern int dB_to_level(double dB);
extern double level_to_dB(int level);