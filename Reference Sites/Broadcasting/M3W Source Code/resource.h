/* resource.h $Revision: 1.7 $ $Date: 2011/06/05 00:57:02 $
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
#define IDD_MAIN                        101
#define IDD_ABOUT                       102
#define IDR_MENU                        103
#define IDD_SOUNDCARD                   104
#define IDD_ENCODER                     105
#define IDD_BROADCAST                   106
#define IDI_M3W                         107
#define IDI_STOP                        108
#define IDI_PLAY                        109
#define IDI_PAUSE                       110
#define IDI_SEND                        111
#define IDI_RECORD                      112
#define IDD_MESSAGE                     113
#define IDD_TAG                         114
#define IDD_STARTUP                     115
#define IDB_BITMAP_CLIP                 116
#define IDB_BITMAP_NOCLIP               117
#define IDD_VOLUMECOMPRESSION           118
#define IDD_CONFIGURATION               119


#define IDC_STATIC			-1
#define IDC_OUTFILE                     1000
#define IDC_INFILE                      1001
#define IDC_BROADCAST                   1002
#define IDC_ENCODER                     1003
#define IDC_REPEAT                      1004
#define IDC_INPOSITION                  1005
#define IDC_INFILENAME                  1006
#define IDC_INSTOP                      1007
#define IDC_INPLAY                      1008
#define IDC_INPAUSE                     1009
#define IDC_ENCODERNAME                 1010
#define IDC_BITRATETEXT                 1011
#define IDC_ENCODERBITRATE              1012
#define IDC_SENDBUFFER                  1013
#define IDC_SEND                        1014
#define IDC_OUTSTOP                     1015
#define IDC_SENDSTOP                    1016
#define IDC_RECORD                      1017
#define IDC_OUTFILENAME                 1018
#define IDC_OUTSIZETEXT                 1019
#define IDC_OUTSIZE                     1020
#define IDC_BITRATETEXT2                1021
#define IDC_SENDBITRATE                 1022
#define IDC_SOUNDCARDNAME               1023
#define IDC_SOUNDCARDFORMAT             1024
#define IDC_SOUNDPLAY                   1026
#define IDC_SOUNDSTOP                   1027
#define IDC_ENCODEBUFFER                1028
#define IDC_SENDNAME                    1029
#define IDC_VERSION                     1030
#define IDC_COPYRIGHT                   1031
#define IDC_THE_SERVER                  1032
#define IDC_JOINT_STEREO                1033
#define IDC_THE_PORT                    1034
#define IDC_STEREO                      1035
#define IDC_THE_PASSWORD                1036
#define IDC_MODE                        1037
#define IDC_THE_STREAMNAME              1038
#define IDC_STREAMNAME                  1039
#define IDC_THE_MOUNTPOINT              1040
#define IDC_THE_BUFFERSIZE              1041
#define IDC_SOUNDLIST                   1042
#define IDC_FREQUENCYLIST               1043
#define IDC_BUFFERSIZE                  1044
#define IDC_INPUTLIST                   1045
#define IDC_ENCODERMODE                 1046
#define IDC_INPROGRES		        1047
#define IDC_SOUNDBUFFERS	        1048
#define IDC_SOUNDBYTES		        1049
#define IDC_ENCODERIN                   1050
#define IDC_ENCODEROUT                  1051
#define IDC_ENCODERTRUERATE             1052
#define IDC_MESSAGE                     1053
#define IDC_ENCODERSITE                 1054
#define IDC_ENCODERCONST                1055
#define IDC_ENCODERVAR                  1056
#define IDC_ENCODERAVG                  1057
#define IDC_ENCODER_BITRATE             1058
#define IDC_RECORD_INDICATOR            1059
#define IDC_RECORD_APPEND               1060
#define IDC_SENDBYTES                   1061
#define IDC_SEND_INDICATOR              1062
#define IDC_OUTPROGRES                  1063
#define IDC_MESSAGETICKER               1064

#define IDC_PRIVATE                     1066
#define IDC_ORIGINAL                    1067
#define IDC_CRC                         1068
#define IDC_DOWNSAMPLE                  1069
#define IDC_RESERVOIR                   1070
#define IDC_ENCODERPRE                  1071
#define IDC_ENCODER_PRESETS             1072
#define IDC_THE_GENRE                   1073
#define IDC_THE_INFO_URL                1074
#define IDC_THE_DESCRIPTION             1075
#define IDC_THE_DUMPFILE                1076
#define IDC_ENCODER_MINBITRATE          1078
#define IDC_ENCODER_MAXBITRATE          1079
#define IDC_TARGET_QUALITY		1080
#define IDC_TXT_PRESETS                 1081
#define IDC_TXT_MINBITRATE              1082
#define IDC_TXT_MAXBITRATE              1083
#define IDC_TXT_BITRATE                 1084
#define IDC_TXT_QUALITY                 1085
#define IDC_PLAY_INDICATOR              1086
#define IDC_VOLUME2			1087
#define IDC_TAG_TITLE                   1088
#define IDC_TAG_ARTIST                  1089
#define IDC_TAG_ALBUM                   1090
#define IDC_TAG_YEAR                    1091
#define IDC_TAG_GENRE                   1092
#define IDC_TAG_COMMENT                 1093
#define IDC_TAG_TRACK                   1094
#define IDC_CLIP                        1095
#define IDC_CLIP2                       1096
#define IDC_MINIMIZED                   1097
#define IDC_BROADCASTING                1098
#define IDC_RECORDING                   1099
#define IDC_LISTENING                   1100
#define IDC_PLAYING                     1101
#define IDC_ENCODERQUALITY              1102
#define IDC_AG_HIGH                     1103
#define IDC_AG_LOW                      1104
#define IDC_AG_SILENCE                  1105
#define IDC_AG_STEP                     1106
#define IDC_AUTOGAIN                    1107
#define IDC_XAUDIO                      1108
#define IDC_HTTP                        1109
#define IDC_ICY                         1110
#define IDC_UDP                         1111
#define IDC_THE_USER                    1112
#define IDC_RC_ENABLE                   1113
#define IDC_RC_DELAY                    1114
#define IDC_RC_IDELAY                   1115
#define IDC_RC_COUNT                    1116
#define IDC_RECORD_AUTONAME             1117
#define IDC_LISTEN_INDICATOR            1118
#define IDC_AUTOVOLUME                  1119
#define IDC_RISETIME                    1120
#define IDC_FALLTIME                    1121
#define IDC_COMPRESS                    1122
#define IDC_FLOOR                       1123
#define IDC_NOISEGAIN                   1124
#define IDC_VC_ENABLE                   1125
#define IDC_CONFIGURATION               1127
#define IDC_LOG                         1128
#define IDC_LIN                         1129
#define IDC_VOLUMECONTROL				1130
#define IDC_OUT                         1131
#define IDC_IN                          1132

#define IDC_SLIDER_SILENCE              1133
#define IDC_SLIDER_STEP                 1134
#define IDC_SLIDER_COMPRESS             1135
#define IDC_SLIDER_RISETIME             1136
#define IDC_SLIDER_FALLTIME             1137
#define IDC_SLIDER_FALLTIME2            1138
#define IDC_SLIDER_NOISEGAIN            1139
#define IDC_SLIDER_HIGH                 1140
#define IDC_SLIDER_LOW                  1141


#define ID_OPTIONS_SONDCARD             40001
#define ID_OPTIONS_INPUT                40002
#define ID_OPTIONS_OUTPUT               40003
#define ID_OPTIONS_BROADCAST            40004
#define ID_OPTIONS_ENCODER              40005
#define ID_FILE_OPEN                    40006
#define ID_FILE_SAVE                    40007
#define ID_FILE_SAVEAS                  40008
#define ID_FILE_EXIT                    40009
#define ID_HELP_ABOUT                   40010
#define ID_HELP_M3W                     40011
#define ID_OPTIONS_TAG                  40012
#define ID_OPTIONS_STARTUP              40013
#define ID_OPTIONS_ERRORSASMESSAGES	40014
#define ID_OPTIONS_RELAXED		40015
#define ID_OPTIONS_MIXER                40016
#define ID_OPTIONS_VOLUMECOMPRESSION	40017
#define ID_HELP_CONFIGURATION           40018
#define ID_OPTIONS_DETACH               40019


#define VERSIONSTR		"Version 3.3"

