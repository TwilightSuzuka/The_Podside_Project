/* param.h $Revision: 1.6 $ $Date: 2011/06/09 02:15:43 $
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


extern char *programpath;
extern char *programhelpfile;

extern char *recordfile;

extern char *inputfile;

extern int encoder_bitmode;
#define ABR 0
#define VBR 1
#define CBR 2
#define PRE 3

extern int appendflag;
extern int autonameflag;
extern char *autonameprefix;
extern char *mountpoint;
extern char *server;
extern int port;
extern char *password;
extern int publicflag;
extern int bitrate;
extern int min_bitrate;
extern int max_bitrate;
extern int preset;
extern int targetquality;
extern int encoderquality;
extern char *name;
extern char *genre;
extern char *info_url;
extern char *description;
extern char *remotedumpfile;
extern int originalflag;
extern int crcflag;
extern int copyrightflag;
extern int downsampleflag;
extern int reservoirflag;
extern int in_buffers;
extern int out_buffers;
extern int stereoflag;
extern int tagyear, tagtrack,taggenre;
extern char *tagtitle, *tagartist, *tagalbum, *tagcomment;
extern int current_sound_device;
extern int current_sound_format;

extern void param_init(void);
extern int load_configfile(char *name);

extern int minimizedflag;
extern int recordingflag;
extern int broadcastingflag;
extern int playingflag;
extern int listeningflag;

extern int ag_high_dB;
extern int ag_low_dB;
extern int ag_silence_dB;
extern int ag_step;
extern int ag_on, ag_disable;

extern int vc_enable;
extern int vc_risetime;
extern int vc_falltime;
extern int target_dB;
extern int vc_noiseattenuation;

#define RELAXFACTOR 5
#define REFRESH_TIME 1000

#define XAUDIO_LOGIN 0
#define ICY_LOGIN 1
#define HTTP_LOGIN 2
#define UDP_LOGIN 3

extern int logintype;
extern char *user;

extern int autoreconnect;
extern int reconnectdelay;
extern int initialrcdelay;
extern int reconnectcount;

extern int error_as_message;

extern int log_scale;
extern int out_scale;
extern int repeat_flag;
extern int meter_detached_flag;