/* volumecompression.h $Revision: 1.3 $ $Date: 2011/06/05 00:57:02 $
 * This File is part of m3w. 
 *
 *	M3W a mp3 streamer for the www
 *
 *	Copyright (c) 2001, 2002, 2009 Martin Ruckert (mailto:ruckertm@acm.org)
 *  The Code in this file was contributed by Philip Van Baren
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

extern void set_volume_compression_parameters(int target_dB, int vc_risetime, int vc_falltime, int vc_noisefloor, int vc_noisegain);
extern void stop_volumecompression();
extern void volume_compression(LPWAVEHDR p, int stereo, int freq);
extern double volume_compression_gain();
extern void set_volumecompression(int on);
extern void reset_volumecompression(void);