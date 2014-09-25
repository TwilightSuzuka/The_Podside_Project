/* broadcast.c $Revision: 1.3 $ $Date: 2010/03/03 16:20:22 $
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

/* this is a new implementation of bc.c using a broadcast thread */
/* disable Warning about deprecated C Standard Function */
#pragma warning(disable : 4996)

#include <windows.h>
#include <winsock.h>
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "resource.h"
#include "main.h"
#include "param.h"
#include "encoder.h"
#include "broadcast.h"
#include "queue.h"


#ifdef DEBUG
extern void bc_test(mp3_buffer data, int mp3_size);
#endif

/* some auxiliar functions */

int base64_encode(char * dest, char *src)
/* encode source to destination, return the number of characters
   in the destination */
{
  static char digit[65] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/',
  };
  unsigned int bits = 0;
  int bitcount = 0;
  int i = 0;
  while (*src != 0)
  { bits = (bits << 8) | *src;
    bitcount = bitcount + 8;
	src++;
	while (bitcount >= 6)
    { bitcount = bitcount -6;
	  dest[i++] = digit[bits>>bitcount];
	  bits = bits & (~(-1 << bitcount));
	}
  }
  if (bitcount==2)
    dest[i++] = digit[bits<<4];
  else  if (bitcount==4)
    dest[i++] = digit[bits<<2];
  while (i & 0x03)
    dest[i++] = '=';
  return i;
}


static long int current_time(void)
{ SYSTEMTIME now;
  GetSystemTime(&now);
  return (((now.wDay*24+now.wHour)*60)+now.wMinute)*60+now.wSecond;
}


/* streams need a contentid, that is randomly generated */
static int contentid=1;

static void contentid_default(void)
/* set defaults */    
{ 
  contentid = rand();
}

static queue bc_empty;
static queue bc_full; 
static long int send_size = 0;
static long int send_time = 0;
static SOCKET connection = INVALID_SOCKET; 
static int broadcasting = 0;
static int sending = 0;
static HANDLE hThread;
static int rc_tries = -1; /* -1 dont try at all otherwise its the count of the number of retries */

/* auxiliar functions for the broadcast thread */

/* the next function computes a login string in a global variable */
static char login_str[1000];
static int login_size = 0;


static void x_audio_login(void)
{
  int i;
  i = 0;
  message(0,"sending x-audio header");

  /* x-audiocast style headers */
  /* send password and mountpoint */
  i = i + sprintf(login_str+i, "SOURCE %s %s\n", password, mountpoint);
  /* send the Bitrate */
  if (bitrate > 0)
    i= i+ sprintf(login_str+i, "x-audiocast-bitrate: %d\n", bitrate/ 1000);
  
  /* stream name */
  if (name!=NULL && strlen(name)>0)
   i= i+sprintf(login_str+i, "x-audiocast-name: %s\n",name);
  
  /* genre */
  if (genre!=NULL&& strlen(genre)>0)
   i = i+sprintf(login_str+i, "x-audiocast-genre: %s\n",genre);

  /* URL */
  if (info_url!=NULL && strlen(info_url)>0)
    i=i+sprintf(login_str+i, "x-audiocast-url: %s\n",info_url);

  /* Public or private? */
  i=i+sprintf(login_str+i, "x-audiocast-public: %d\n",publicflag);

  /* Description string */
  if (description!=NULL && strlen(description)>0)
    i=i+ sprintf(login_str+i, "x-audiocast-description: %s\n",description); 

  contentid_default();
  i=i+sprintf(login_str+i, "x-audiocast-contentid: %d\n",contentid);
  
  if(remotedumpfile!=NULL && strlen(remotedumpfile)>0)
    i=i+sprintf(login_str+i, "x-audiocast-dumpfile: %s\n",remotedumpfile);
  
  login_str[i++]='\n';
  login_str[i++]='\n';
  login_str[i++]=0; 
  if (i>=1000) errormsg("login_str overrun in x-audioheader",i);
  login_size = i;
}

static void icy_login(void)
{
  int i;
  i = 0;
  message(0,"sending icy header");

  /* icy style headers */
  /* send password */
  i = i + sprintf(login_str+i, "%s\n", password);
  /* stream name */
  if (name!=NULL && strlen(name)>0)
   i= i+sprintf(login_str+i, "icy-name:%s\n",name);
  else
   i= i+sprintf(login_str+i, "icy-name:%s\n","unknown");
  /* URL */
  if (info_url!=NULL && strlen(info_url)>0)
    i=i+sprintf(login_str+i, "icy-url:%s\n",info_url);
  else
    i=i+sprintf(login_str+i, "icy-url:%s\n","http://unknown/");
  /* not used */
  i=i+sprintf(login_str+i,"icy-irc:\n");
  i=i+sprintf(login_str+i,"icy-aim:\n");
  i=i+sprintf(login_str+i,"icy-icq:\n");
  /* Public or private? */
  i=i+sprintf(login_str+i, "icy-pub:%i\n",publicflag);
  /* genre */
  if (genre!=NULL&& strlen(genre)>0)
   i = i+sprintf(login_str+i, "icy-genre:%s\n",genre);
  /* send the Bitrate */
  if (bitrate > 0)
    i= i+ sprintf(login_str+i, "icy-br:%d\n", bitrate/ 1000);
  else
    i= i+ sprintf(login_str+i, "icy-br:%d\n", 1);
  
  login_str[i++]='\n';
  login_str[i++]=0; 
  if (i>=1000) errormsg("login_str overrun in icy header",i);
  login_size = i;
}

static void http_login(void)
{
  int i;
  i = 0;
  message(0,"sending http header");

  /* http style headers */

  /* send mountpoint */
  if (mountpoint[0] == '/')
    i = i + sprintf(login_str+i, "SOURCE %s HTTP/1.0\r\n", mountpoint);
  else
    i = i + sprintf(login_str+i, "SOURCE /%s HTTP/1.0\r\n", mountpoint);
  /* send password */
  i = i + sprintf(login_str+i, "Authorization: Basic ");
  { char tmp[100];
    if (strlen(user)+strlen(password)+1 >= 100)
		errormsg("Password or User too long",0);
    sprintf(tmp,"%s:%s",user,password);
	i= i + base64_encode(login_str+i,tmp);
  }
  i = i + sprintf(login_str+i, "\r\n");
	/* stream name */
  if (name!=NULL && strlen(name)>0)
   i= i+sprintf(login_str+i, "ice-name: %s\r\n",name);
  else
   i= i+sprintf(login_str+i, "ice-name: %s\r\n","unknown");
  /* URL */
  if (info_url!=NULL && strlen(info_url)>0)
    i=i+sprintf(login_str+i, "ice-url: %s\r\n",info_url);
  /* genre */
  if (genre!=NULL&& strlen(genre)>0)
   i = i+sprintf(login_str+i, "ice-genre: %s\r\n",genre);
  /* audio info ??? */
   i = i+sprintf(login_str+i, "ice-audio-info: %s\r\n", "none"); 
  /* Public or private? */
  i=i+sprintf(login_str+i, "ice-public: %d\r\n",publicflag);
  /* Description string */
  if (description!=NULL && strlen(description)>0)
    i=i+ sprintf(login_str+i, "ice-description: %s\r\n",description); 

  i= i+ sprintf(login_str+i, "User-Agent: m3w\r\n");
  i= i+ sprintf(login_str+i, "Content-Type: audio/mpeg\r\n");

  login_str[i++]='\r';
  login_str[i++]='\n';
  login_str[i++]=0; 
  if (i>=1000) errormsg("login_str overrun in icy header",i);
  login_size = i;
}

static int write_bytes(char *str, int length)
/* return the number of bytes successfully sent, or -1 on error */
{ int i, offset=0;
/*use to get maximum packet size for send 
int getsockopt (
  SOCKET s,         
  int level,        
  int optname,      
  char FAR* optval, 
  int FAR*  optlen  
);
 

*/
  do { 
    i = send(connection,str+offset,length,0);
    if (i == SOCKET_ERROR )
	{ error_code = WSAGetLastError();
	  message(0,"no broadcast");
	  return -1;
	}
    length = length - i;
    offset = offset + i;
  } while (length > 0);        
  return offset;
}

static int bc_login(void)
/* return 1 if login string send otherwise return 0; */
{ 
  if (logintype == UDP_LOGIN)
	  return 1;
  else if (logintype == ICY_LOGIN)
   icy_login();
  else  if (logintype == HTTP_LOGIN)
	http_login();
  else /* if (logintype == X_AUDIO_LOGIN)  or otherwise*/
	x_audio_login();
  message(0,"Loging in");
  if (write_bytes(login_str,login_size)> 0)
    return 1;
  else
	return 0;
}

static char buffer[1000];

static int get_OK(void)
/* return 1 if an OK was received */  
{ int i, offset;
  if (logintype == UDP_LOGIN)
	  return 1;
  i = recv(connection, buffer, 1000, MSG_PEEK);
  if (i == SOCKET_ERROR || i <= 0)
  { error_code = WSAGetLastError();
	errormsg("Unable to get OK",error_code);
	return 0;
  }
  buffer[i] = 0;
  while (i>0 && isspace(buffer[i-1]))
		{ i= i-1; buffer[i]='\0';}
  for (offset=0; offset < i-1; offset++)
    if (buffer[offset] == 'O' && buffer[offset+1] == 'K')
	{ message(0,buffer);
      return 1;       
	}
  errormsg(buffer,0);
  return 0;


}






static int bc_connect(void)
/*  
   we have a server and try to connect.
   On success we return 1 otherwise 0.
*/
{ SOCKADDR_IN server_sin;
  unsigned long server_ip;

  server_ip = inet_addr(server);
  if (server_ip == INADDR_NONE)
  { PHOSTENT phe;
	message(0,"Resolving server name");
    phe = gethostbyname(server);
	if (phe==NULL)
	{ errormsg("Unable to resolve host name",0);
      server_ip = 0;
      return 0;
	}
    message(0,"Resolved");
    memcpy(&server_ip,phe->h_addr,sizeof(server_ip));
  }
  server_sin.sin_addr.s_addr = server_ip;
  if (logintype == UDP_LOGIN)
    connection = socket( AF_INET, SOCK_DGRAM, 0);
  else
    connection = socket( AF_INET, SOCK_STREAM, 0);

  if (connection == INVALID_SOCKET)
  { errormsg("Unable to create socket",0);
    return 0;
  }
  message(0,"Connecting to server");
  server_sin.sin_family = AF_INET;
  if (logintype == ICY_LOGIN)
    server_sin.sin_port = htons((unsigned short)(port+1));
  else
    server_sin.sin_port = htons((unsigned short)port);
  error_code = connect(connection, (PSOCKADDR) &server_sin, sizeof( server_sin));
  if (error_code!=0)
    return 0;
  message(0,"Connected");
  return 1;
}



/* the broadcast thread */
/* mp3 buffers can be in two queues: empty and full */

static DWORD BroadcastThread(LPDWORD lpdwParam) 
{ mp3_buffer data;
  int i;
  send_size = 0;
  send_time = current_time();	

  if (bc_connect() && bc_login()  && get_OK())
  {   PostMessage(hMainWindow,WM_SENDDISPLAY,0,BC_SENDING);
      sending = 1;
      while (1)
	  { 
        if (is_empty(&bc_full))
			wait_nonempty(&bc_full);
        data = dequeue(&bc_full);  
		if (data == NULL)
		{ PostMessage(hMainWindow,WM_SENDDISPLAY,0,BC_IDLE);
          goto thread_exit;
		}
		else
		{ i =	write_bytes(data->buffer+data->offset,data->size-data->offset);
		  if (i<0)
		  { enqueue(&bc_empty,data);
		    message(0,"Unable to broadcast data");
            goto thread_exit;
		  }
		  send_size = send_size + i;
          if (send_size > 10000) rc_tries = 0;

#ifdef DEBUG
	      bc_test(data,data->size-data->offset);
#endif
           enqueue(&bc_empty,data);
		}
      }
  }

thread_exit:
  sending = 0;
  if (connection != INVALID_SOCKET && closesocket(connection) != 0)
  { error_code = WSAGetLastError();
	errormsg("Unable to close connection",error_code);
  }
  connection = INVALID_SOCKET;
  while (!is_empty(&bc_full))
  { data = dequeue(&bc_full);
    if (data != NULL)
	  enqueue(&bc_empty, data);
  }
  message(0,"Connection closed");
  if (broadcasting && autoreconnect && rc_tries >= 0 && rc_tries < reconnectcount)
  {
    PostMessage(hMainWindow,WM_SENDDISPLAY,0,BC_RECONNECT);
	if (rc_tries == 0)
      Sleep(initialrcdelay*1000);
    else 
	  Sleep(reconnectdelay*1000);
	rc_tries++;
    PostMessage(hMainWindow,WM_SENDDISPLAY,0,BC_IDLE);
    PostMessage(hMainDialog,WM_COMMAND,IDC_SEND,0);
  }
  else
    PostMessage(hMainWindow,WM_SENDDISPLAY,0,BC_IDLE);
  ExitThread(0);
  return 0;

}


/* called to update the main dialog */
void display_send_status(LPARAM lparam)
{ 
  if (lparam == BC_UPDATE) 
  { long int delta=0;
  	if (!sending) return;
    SendDlgItemMessage(hMainDialog,IDC_OUTPROGRES, PBM_SETPOS,queue_size(&bc_full), 0);
    SetDlgItemNumber(hMainDialog,IDC_SENDBYTES,send_size,FALSE);
    delta = current_time()-send_time;
    if (delta > 0)
      SetDlgItemNumber(hMainDialog,IDC_SENDBITRATE,(long int)(send_size*8.0)/delta ,FALSE);
	SendDlgItemMessage(hMainDialog,IDC_OUTPROGRES, PBM_SETPOS,queue_size(&bc_full), 0);
  }
  else if (lparam == BC_IDLE)
  { broadcasting = 0;
    SetDlgItemText(hMainDialog,IDC_SEND_INDICATOR,"");
  }
  else  if (lparam == BC_SENDING)
  { SetDlgItemText(hMainDialog,IDC_SEND_INDICATOR,"SENDING");
  }
  else  if (lparam == BC_RECONNECT)
  { broadcasting = 0;
    SetDlgItemText(hMainDialog,IDC_SEND_INDICATOR,"RECONNECT");
  }
}


/* called by the encoder */
mp3_buffer get_empty_mp3buffer(void)
{ mp3_buffer n;
  
  while (!is_empty(&bc_empty))
  {  n = check_mp3buffer(dequeue(&bc_empty));
     if (n != NULL)
       return n;
  }
  n = new_mp3buffer();
  return n;
}

/* the interaction with the broadcast thread, called from the main thread*/

void bc_init(void)
/* called at program start */
{  WSADATA wsadata;
   if(WSAStartup(MAKEWORD(1,1), &wsadata) != 0)
     fatal_error("Unable to initialize Winsock dll");
   srand(current_time());
   create_queue(&bc_empty);
   create_queue(&bc_full);
}

void bc_exit(void)
/* called at program end */
{ stop_bc();
  WSACleanup();
}


/* the broadcast start and stop button call start_bc and stop_bc
   the first sets broadcast to 1 the later to 0. 
   The broadcast variable reflects whether a broadcast is running.
   
   The broadcast tread will handle the sending of buffers
   once it is redy it sets the variable sending to 1
   if it is not longer ready to send buffers, it sets the variable sending to 0
   While sending == 1 bufferes are enqueued to bc_full.
   Enqueueing a NULL buffer means the end of the mp3 stream.
   It will cause the sending thread to end.
*/


void start_bc(void)
/* called to initiate broadcasting after pressing the send button*/
{ DWORD dwThreadId;
  if (broadcasting)
      return;
  broadcasting = 1;
  if (server == NULL || server[0]==0)
  { errormsg("No Server given",0);
    broadcasting = 0;
    return;
  }
  hThread = CreateThread(
        NULL,                        /* no security attributes        */
        0,                           /* use default stack size        */
        (LPTHREAD_START_ROUTINE) BroadcastThread, /* thread function       */
        NULL,                        /* argument to thread function   */
        0,                           /* use default creation flags    */
        &dwThreadId);                /* returns the thread identifier */
   if (hThread == NULL)
   { errormsg("Unable to create broadcast thread",0);
     broadcasting = 0;
     return;
   }
}

void stop_bc(void)
/* stop button pressed */
{ if (!broadcasting)
    return;
  broadcasting = 0;
  enqueue(&bc_full, NULL);
} 


void bc_write(mp3_buffer data)
/* called from the encoder each time a buffer is ready */
{ if (broadcasting && sending)
    enqueue(&bc_full, data);
  else
  { if (data != NULL)
      enqueue(&bc_empty, data);
  }
}



