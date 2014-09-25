/* param.c $Revision: 1.10 $ $Date: 2011/06/09 02:15:43 $
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
#include "main.h"
#include "param.h"
#include "option.h"
#include "BladeMP3EncDLL.h"



char *programpath = NULL;
char *programhelpfile = NULL;


char *server = NULL;
int port = 8000;
char *mountpoint = NULL;
int logintype = XAUDIO_LOGIN;
char *user=NULL;
char *password = NULL;
char *description=NULL;
int bitrate=64000;
int publicflag = 1;
int min_bitrate = 0;
int max_bitrate = 0;
int encoder_bitmode = ABR;
int preset = LQP_VOICE;
int appendflag = 1;
int autonameflag = 0;
char *autonameprefix = NULL;
char *name=NULL;
char *genre=NULL;
char *info_url=NULL;
char *remotedumpfile=NULL;
char *recordfile = NULL;
int in_buffers = 5;
int out_buffers = 20;
int originalflag=0;
int crcflag=0;
int copyrightflag=0;
int downsampleflag = 0;
int reservoirflag = 0;
int targetquality = 5;
int encoderquality = 2;
int stereoflag=0;
int tagyear=2000, tagtrack=0,taggenre=12;
char *tagtitle=NULL, *tagartist=NULL, *tagalbum=NULL, *tagcomment=NULL;
int current_sound_device=0; /* a nuber in the range given by num_soundcards() */
int current_sound_format=11; /* a number in the range to MAX_SOUND_FORMATS -1 */
char *inputfile = NULL;
int minimizedflag = 0;
int recordingflag = 0;
int broadcastingflag = 0;
int playingflag = 0;
int listeningflag = 0;

int ag_high_dB;
int ag_low_dB;
int ag_silence_dB;
int ag_step;
int ag_on = 1, ag_disable=0;

int vc_enable = 0; // Boolean
int vc_risetime = 30; // ms
int vc_falltime = 10; // seconds
int target_dB = -3;  // dB
// int vc_noisefloor = 55; replaced by -ag_silence_dB
int vc_noiseattenuation = 6; // dB


int autoreconnect = 0;
int reconnectdelay = 60;
int initialrcdelay = 60;
int reconnectcount = 3;

int error_as_message = 0;

int log_scale = 1;
int out_scale = 1;

int repeat_flag = 0;
int meter_detached_flag = 0;

static void usage(char *message)
{ if (programhelpfile==NULL)
    WinHelp(hMainWindow,"m3w.hlp",HELP_CONTENTS, 0L);
  else
    WinHelp(hMainWindow,programhelpfile,HELP_CONTENTS, 0L);
}


int load_configfile(char *name)
{ if (parse_configfile(name))
	{ int offset;
	  offset = strlen(name);
	  if (offset==0) return 0;
	  while (offset>0 && name[offset-1]!='\\') 
		  offset--;
      SetWindowText(hMainWindow,name+offset);
      display_main_dialog_information(hMainDialog);
	  return 1;
	}
  return 0;
}

/* option chars available:
                             
                             
available short options  CJK R O S U W Z ab e  ghij kl uvw yz 0123456789 
*/

option_spec options[] = {
/* description short long kind default handler */
{"the server where to send the request", 's', "server",     "server",     str_arg, "localhost", {&server}},
{"the port where to send the request",   'p', "port",       "port",       int_arg, "8000", {&port}},
{"the mountpoint",                       'm', "mountpoint", "mointpoint", str_arg, "live", {&mountpoint}},
{"the login type",                       'L', "logintype",  "value",      int_arg, "0", {&logintype}},
{"the user name",                        'F', "user",        NULL,        str_arg, "source",{&user}},
{"the password",                         'P', "password",   "password",   str_arg, "topsecret", {&password}},
{"filename for a configuration file",    'f', "configfile", "file",       fun_arg, NULL, {(void *)load_configfile}},
{"to print usage information",           '?', "help",       NULL,         fun_arg, NULL,{(void *)usage}},

{"a description for the stream",         'D', "description","description",str_arg, NULL, {&description}},
{"the stream bitrate",                   'B', "bitrate",    "bitrate",    int_arg, "64000", {&bitrate}},
{"to make the stream private",           0, "private",    NULL,         off_arg,  NULL, {&publicflag}},
{"the stream minimum bitrate",           0, "min",    "bitrate",        int_arg, "0", {&min_bitrate}},
{"the stream maxium bitrate",            0, "max",    "bitrate",        int_arg, "0", {&max_bitrate}},
{"the encoder bitrate mode (0=abr,1=vbr,2=cbr,3=pre)",'M', "bitmode","mode",int_arg,ABR, {&encoder_bitmode}},
{"the lame encoder mode preset value",        0, "presetvalue", "value",     int_arg, "5000", {&preset}},
{"set flag to switch on appending output",0, "append",NULL,             on_arg, NULL,{&appendflag}},
{"set flag to switch off appending output",0, "overwrite",NULL,         off_arg, NULL,{&appendflag}},
{"set flag to switch on automatic output file name",0, "autoname",NULL, on_arg, NULL,{&autonameflag}},
{"set a common prefix to the automatic output file name",0, "autonameprefix", NULL, str_arg, NULL, {&autonameprefix}},
{"the stream name",                      'n', "name",       "name",       str_arg, NULL, {&name}},
{"the stream genre",                     0, "genre",      "genre",      str_arg, NULL, {&genre}},   
{"the place where to get information",   0, "url",        "url",        str_arg, NULL, {&info_url}},
{"filename for a remote dump",           'd', "remotedump", "file",       str_arg, NULL, {&remotedumpfile}},
{"filename for a local recording",       'r', "recordfile", "file",       str_arg, NULL, {&recordfile}},
{"maximum time to buffer soundcard",     0, "inbuffer",   "seconds",    int_arg, "5", {&in_buffers}},
{"maximum time to buffer network",       'o', "outbuffer",  "seconds",    int_arg, "20", {&out_buffers}},
{"set flag to mark copies",              'c', "copyflag",    NULL,        off_arg, NULL,{&originalflag}},
{"set flag to generate crc",             0, "crcflag",     NULL,        on_arg, NULL,{&crcflag}},
{"set flag to indicate copyright",       0, "copyright",   NULL,        on_arg, NULL,{&copyrightflag}},
{"set flag to disable downsampling",     0, "downsampleflag", NULL,     on_arg, NULL,{&downsampleflag}},
{"set flag to disable bitreservoir",     0, "reservoirflag", NULL,      on_arg, NULL,{&reservoirflag}},
{"the bitrate quality value",            'q', "quality",     "value",     int_arg, "5", {&targetquality}},
{"the encoder quality value",            'E', "encoderquality","value",     int_arg, "2", {&encoderquality}},
{"set flag to choose true stereo",       't', "stereo",      NULL,        on_arg, NULL,{&stereoflag}},
{"the ID3 Tag year",                     'Y', "tagyear",    "year",       int_arg, "2000", {&tagyear}},
{"the ID3 Tag track",                    'T', "tagtrack",   "track",      int_arg, "0", {&tagtrack}},
{"the ID3 Tag genre",                    'G', "taggenre",   "value",      int_arg, "12", {&taggenre}},
{"the ID3 Tag title",                    'H', "tagtitle",   "title",      str_arg, NULL, {&tagtitle}},
{"the ID3 Tag artist",                   'A', "tagartist",  "artist",     str_arg, NULL, {&tagartist}},
{"the ID3 Tag album",                    0, "tagalbum",   "album",      str_arg, NULL, {&tagalbum}},
{"the ID3 Tag comment",                  0, "tagcomment", "comment",    str_arg, NULL, {&tagcomment}},
{"the soundcard number",                 'N', "soundcard",  "value",      int_arg, "0", {&current_sound_device}},
{"the sound format number",              'Q', "soundformat","value",      int_arg, "11", {&current_sound_format}},
{"the Name of Input File",               'I', "inputfile",    NULL,       str_arg, NULL,{&inputfile}},

{"set flag to start minimized",          'x', "minimized",   NULL,        on_arg, NULL,{&minimizedflag}},
{"set flag to start recording",          0, "recording",   NULL,        on_arg, NULL,{&recordingflag}},
{"set flag to start broadcasting",       0, "broadcasting",NULL,        on_arg, NULL,{&broadcastingflag}},
{"set flag to start playing",            0, "playing",     NULL,        on_arg, NULL,{&playingflag}},
{"set flag to start listening",          0, "listening",   NULL,        on_arg, NULL,{&listeningflag}},


{"the autogain silence level",           0, "silence",    "dB",    int_arg, "-55", {&ag_silence_dB}},
{"the autogain low level",               0, "lowlevel",   "dB",    int_arg, "-40", {&ag_low_dB}},
{"the autogain high level",              0, "highlevel",  "dB",    int_arg, "-6", {&ag_high_dB}},
{"the autogain step level",              0, "steplevel",  "percent",    int_arg, "2", {&ag_step}},
{"the autogain flag",                    0, "autogain",   NULL,         on_arg, NULL, {&ag_on}},

{"the volume compressor rise time",      0, "risetime",   "ms",    int_arg, "30",  {&vc_risetime}},
{"the volume compressor fall time",      0, "falltime",   "sec",   int_arg, "10",  {&vc_falltime}},
{"the volume compressor level",          0, "compress",   "dB",    int_arg, "-6",   {&target_dB}},
{"the volume compressor noise attenuation", 0, "noiseatten",  "dB",    int_arg, "0",   {&vc_noiseattenuation}},
{"the volume compressor flag",           0, "volumecompress", NULL, on_arg, NULL,  {&vc_enable}},


{"the auto reconnect flag",              'X', "autoreconnect",   NULL,         on_arg, NULL, {&autoreconnect}},
{"the initial auto reconnect delay",     0, "rcidelay","seconds",    int_arg, "60", {&initialrcdelay}},
{"the auto reconnect delay",             0, "rcdelay",       "seconds",    int_arg, "60", {&reconnectdelay}},
{"the auto reconnect count",             0, "rccount",       NULL,    int_arg, "3", {&reconnectcount}},
{"the error as message flag",            0, "noerror",   NULL,         on_arg, NULL, {&error_as_message}},

{"display linear meter scale",0, "linearscale",NULL,         off_arg, NULL,{&log_scale}},
{"display input meter scale",0, "inputscale",NULL,         off_arg, NULL,{&out_scale}},
{"set flag to repeat a playing input file",0, "repeat",NULL,             on_arg, NULL,{&repeat_flag}},
{"set flag to start with a detached volume meter",0, "detachmeter",NULL,             on_arg, NULL,{&meter_detached_flag}},

{NULL}
};

void do_argument(int pos, char * arg)
{ if (arg == NULL)
    return;
  if (pos == 0) /* program name */
  { int i,n;
    char *configfile;
    i = 0;
	n = 0;
	while (arg[i]!=0)
	{ if (arg[i] == '\\') 
		n = i+1;
	  i++;
	}
	programpath = malloc(n  + 1); /* path + '0' */
    if (programpath==NULL) 
    { fatal_error("Out of memory");
	  return;
	}
	strncpy(programpath,arg,n);
	programpath[n]=0;
	programhelpfile = malloc(n  + 7 + 1); /* path  + m3w.hlp  + '0'*/
    if (programhelpfile==NULL) 
    { fatal_error("Out of memory");
	  return;
	}
    strcpy(programhelpfile,programpath);
	strcat(programhelpfile,"m3w.chm");
	configfile = malloc(n  + 11 + 1); /* arg + default.m3w  + '0'*/
    if (configfile==NULL) 
    { fatal_error("Out of memory");
	  return;
	}
    strcpy(configfile,programpath);
	strcat(configfile,"default.m3w");
	parse_configfile(configfile); /* global configfile first */
	parse_configfile("default.m3w"); /* next local configfile */
	}
  else if (pos == 1) /* configuration file */
	load_configfile(arg);
  else
    errormsg("extra argument on commandline ignored",0);
   
}



void param_init(void)
{ option_defaults();
  parse_commandstr(GetCommandLine());
}