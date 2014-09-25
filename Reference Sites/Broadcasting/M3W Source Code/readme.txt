README.TXT


Introduction:
=============
In the meantime, this is still a well tested version of m3w, a streaming
mp3 client to an icecast server. 

Still, some features are missing 
and some bugs may linger in the code.

Getting Started:
================
For recording to disk, just press the play button in the soundcard
section and the record button in the output section.
If the volume bar in the soundcard section is not moving,
you will need to set some options for the soundcard using the
Options Menu, and/or select the right input channel and input volume
using the standard windows sound mixer (sndvol32.exe) usualy located
in the taskbar behind the loudspeaker symbol. 
Your can also double click the loudspeaker symbol in the Control Panel and select the Recording set of Sliders.

For streaming to the internet, you need access to a streaming
server, for example icecast, icecast2 (see www.icecast.org) or
the Darwin Streaming server.
You need to know the name of the server, the port (usually 8000),
the mountpoint, and the password for the server. This information must
be entered using the Broadcast Dialog located in the Options menu.
There are no reasonable default values for it.
Once the information is set, press the broadcast button in the
broadcast section of the main window. There is some progress information
in the Message bar at the bottom of the main window to tell you about
success or failure.


High Lights:
============
Support:
Check www.cs.fhm.edu/~ruckert/m3w for support.

Buffering:
All data is buffered whenever possible. So no data gets lost.

Bitrates:
The target bitrates of the encoder, the true measured bitrate of
the encoder and the true measured bitrate of the internet connection
are displayed. This way you can see what your throughput realy is
and adjust the parameters.

Please report
=============
all error messages or crashes or suggestions to ruckertm@acm.org

Have Fun!
   Martin