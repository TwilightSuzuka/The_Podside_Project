/* autogain.h $Revision: 1.2 $ $Date: 2011/06/05 00:57:02 $
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
extern void init_Mixer_device(HWND hwnd, HWAVEIN hwavein);
extern void init_Mixer_data(void);
extern void set_Mixer_data(void);
extern void check_volume(LPWAVEHDR p, int stereo, int freq);
extern int peak_volume(int channel);
extern int rms_volume(int channel);
extern int clip_volume(int channel);
extern void change_Mixer_Control(HMIXER hMixer,DWORD dwControlID);
extern void change_Mixer_Line(HMIXER hMixer,DWORD dwLineID);
extern void close_Mixer_data(void);
extern void set_ag_levels(void);