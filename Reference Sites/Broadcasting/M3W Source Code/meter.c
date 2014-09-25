/* meter.c $Revision: 1.5 $ $Date: 2011/06/09 02:15:43 $
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


#include <windows.h>
#include <commctrl.h>
#include "math.h"
#include "resource.h"
#include "param.h"
#include "autogain.h"
#include "main.h"
#include "sound.h"
#include "volumecompression.h"
#include "autogain.h"
#include "meter.h"

#pragma warning( disable : 4996 )

/* Layout of meter:
   there is a border around the display. The border is zero in attached mode 
   there is 
   a scale after the boder, 
   a distance between scale and bar
   a bar
   a distance between scale and bar
   a scale
   a distance between scale and text
   a scale text
   in stereo mode this sequence repeats in reverse order
*/

#define MIN_BORDER 5
#define MIN_SCALE 3
#define MIN_TEXT 15
#define MIN_TEXT_DESC 3
#define MIN_BAR 5
#define MIN_SCALE_BAR 1
#define MIN_SCALE_AND_BAR (2*MIN_SCALE+MIN_BAR+2*MIN_SCALE_BAR)
#define MIN_TOTAL_MONO (MIN_SCALE_AND_BAR+MIN_TEXT)
#define MIN_TOTAL_STEREO (MIN_TOTAL_MONO+MIN_SCALE_AND_BAR)
#define GROW_SCALE 0.1

static int min_width, min_height; /* minimum client size of meter */
static int min_win_width, min_win_height; /* minimum client size of meter */
static int meter_width, meter_height; /* actual client size of meter */
static int hborder, vborder; /* border around meter inside client area */
static int mid_scale_width, mid_scale_height; /* total scale dimensions */
static int bar_width, bar_height; /* total bar dimensions */
static int bar_y[2]; /* position of meter bars */
static int bar_begin_dB, bar_silence, bar_low, bar_high; /* bar signal levels */
static int bar_end[2], bar_level[2]={0,0}, bar_clip[2]={0,0}; /* actual bar lenght */
static int scale_text_y, mid_scale_y; /* position of scale */
static int scale_height, scale_text_height; /* scale dimensions */
static int scale_bar_dist, zero_width; /* scale dimensions */
static int scale_step_dB; /* dB between scale lines */
static double meter_gain = 1.0; /* actual gain level */
static int scale_end; /* actual scale end for gain */
static int clip_width,bar_clip_dist; /* clipping layout */

static int meter_mono = 0;
static int timerOn = 0;

static int old_bar_end[2] = {0,0}, old_scale_end = 0;
static int old_clip_volume[2] = {0,0};

/* colors, pens and brushes */
#define silence_color RGB(0,0,128)
#define low_color RGB(0,0,255)
#define mid_color RGB(160,160,255)
#define high_color RGB(255,255,0)
#define clip_color RGB(255,0,0)
static HBRUSH hsilence, hlow, hmid, hhigh, hback, hclip;
static LOGFONT lscale_font;
static HFONT hscale_font=0;
/* the window handle */
static HWND hMeter;
static HINSTANCE hInstance;
static HWND hParent;
static HDC hscaleDC, hmeterDC;
static int meter_detached;

/* 
 *
 * Painting routines 
 *
 *
 */
static int dB_to_x(int dB)
{ return (int)floor(0.5+(bar_width-1.0)*(bar_begin_dB-dB)/(double)bar_begin_dB);
}

static void paint_scale_lines(HDC hdc, int dx, int dy)
{ int dB;
  for (dB=0; dB>=bar_begin_dB;dB=dB+scale_step_dB)
  { int x;
    x = dx+dB_to_x(dB);
    MoveToEx(hdc,x,dy,NULL);
	LineTo(hdc,x,dy+scale_height);
  }
}


static void paint_mid_scale(void)
{ int dB;

  PatBlt(hscaleDC,0,0,mid_scale_width,mid_scale_height,PATCOPY);  /* fill background */

  SetTextAlign(hscaleDC,TA_TOP|TA_RIGHT|TA_NOUPDATECP);
  ExtTextOut(hscaleDC,mid_scale_width,scale_text_y,ETO_OPAQUE,NULL,"dB",2,NULL);
  SetTextAlign(hscaleDC,TA_TOP|TA_CENTER|TA_NOUPDATECP); // TA_LEFT
  for (dB=0; dB>=bar_begin_dB;dB=dB+scale_step_dB)
  { int x;
    int numlen;
    char num[4];
	numlen = 0;
	if (dB!=0 && !log_scale) break;
	if (dB<0) num[numlen++] = '-';
	if (dB>-10) 
	  num[numlen++]=-dB+'0';
	else 
	{ num[numlen++]=-dB/10+'0';
	  num[numlen++]=-dB%10+'0';
	}
	num[numlen] = 0;
    x = dB_to_x(dB);
	if (x<1.5*zero_width)
		  SetTextAlign(hscaleDC,TA_TOP|TA_LEFT|TA_NOUPDATECP);
    ExtTextOut(hscaleDC,x,scale_text_y,ETO_OPAQUE,NULL,num,numlen,NULL);
  }

  paint_scale_lines(hscaleDC,0,0);
  if (!meter_mono)
  {	paint_scale_lines(hscaleDC,0,mid_scale_height-scale_height);
  }
}

static paint_scale_bitmap(HDC hdc, int e, int max_width)
{ int n;
  if (e>0)
    BitBlt(hdc,hborder,mid_scale_y,e,mid_scale_height,hscaleDC,mid_scale_width-e,0,SRCCOPY);
  n = max_width-e;
  if (n>0)
  { SelectObject(hdc, hback); /*background */
    PatBlt(hdc,hborder+e,mid_scale_y,n,mid_scale_height,PATCOPY);  
  }
}


static void paint_clip(HDC hdc, int y, int c) 
{ if (c)
    SelectObject(hdc, hclip); /* Background */
  else 
	SelectObject(hdc, hback); /* Background */
  PatBlt(hdc,hborder+bar_width+bar_clip_dist,y,clip_width,bar_height,PATCOPY);
}

static void paint_bar_from_e_to(HDC hdc, int y, int b,int e, int w)
{ int n;
  n = min(e,bar_silence);
  if (n>b)
  { SelectObject(hdc, hsilence); 
    PatBlt(hdc,hborder+b,y,n-b,bar_height,PATCOPY);
    b=n;
  }
  n = min(e,bar_low);
  if (n>b)
  { SelectObject(hdc, hlow); 
	PatBlt(hdc,hborder+b,y,n-b,bar_height,PATCOPY);
	b=n;
  }
  n = min(e,bar_high);
  if (n>b)
  { SelectObject(hdc, hmid); 
	PatBlt(hdc,hborder+b,y,n-b,bar_height,PATCOPY);
	b=n;
  }
  n = e;
  if (n>b)
  { SelectObject(hdc, hhigh); 
	PatBlt(hdc,hborder+b,y,n-b,bar_height,PATCOPY);
  	b=n;
  }
  if (b<w)
  { SelectObject(hdc, hback); /* Background */
	PatBlt(hdc,hborder+b,y,w-b,bar_height,PATCOPY);
  }
}

/* 
 *
 * Calculating dynamic values
 *
 *
 */


static int calculate_scale_end(double gain)
{  int e;
    if (log_scale) 
      e = (int)floor(0.5+bar_width*(-factor_to_dB(gain)-bar_begin_dB)/(-bar_begin_dB));
    else
      e = (int)floor(0.5+bar_width/gain); 
	if (e<0) e=0;
	if (e>bar_width) e = bar_width;
	return e+(mid_scale_width-bar_width);
}


static int calculate_bar_end(int level)
{ int e;
  if (log_scale) 
	e = (int)floor(0.5+bar_width*(level_to_dB(level)-bar_begin_dB)/(-bar_begin_dB));
  else
    e = (int)floor(0.5+bar_width*level/(double)0x7FFF);
  if (e<0) e = 0;
  return e;
}

/* 
 *
 * Formating routines
 *
 *
 */
static void set_scale_font(void)
{ SIZE size;
  TEXTMETRIC tm;
  lscale_font.lfHeight=-(scale_text_height-MIN_TEXT_DESC); /* account for a minimum descent */
  lscale_font.lfWidth=0;
  lscale_font.lfWeight = FW_NORMAL;
  strncpy(lscale_font.lfFaceName,"Arial",sizeof(lscale_font.lfFaceName));
  if (hscale_font!=0) 
	  DeleteObject(hscale_font);
  hscale_font = CreateFontIndirect(&lscale_font);  
  SelectObject(hscaleDC,hscale_font);
  GetTextMetrics(hscaleDC,&tm);
  scale_text_height=tm.tmHeight;
  scale_text_y=scale_height;
  GetTextExtentPoint32(hscaleDC,"dB",2,&size);
  bar_width -= size.cx;
  GetTextExtentPoint32(hscaleDC," ",1,&size);
  bar_width -= size.cx;
  GetTextExtentPoint32(hscaleDC,"0",1,&size);
  zero_width = size.cx;
  bar_width -= size.cx/2;
  clip_width = meter_width-bar_width-3*hborder;
  bar_clip_dist = hborder;
}


static void set_heights(void)
{ int glue_height;
  /* set minimum values */
  bar_height = MIN_BAR; 
  scale_height = MIN_SCALE;
  scale_text_height = MIN_TEXT;
  /* grow values to fill window */
  if (meter_mono)
	glue_height = meter_height-MIN_TOTAL_MONO;
  else 
    glue_height = meter_height-MIN_TOTAL_STEREO;
  glue_height = glue_height-2*vborder;
  if (glue_height > 0)
  { int d = (int)(GROW_SCALE*glue_height);
    scale_height += d;
	scale_text_height += d;
  }
  set_scale_font();
  if (meter_mono) 
  { mid_scale_height= scale_text_height+scale_height;
    bar_height = 
		meter_height
		-vborder*2
		-scale_height
		-MIN_SCALE_BAR*2
		-mid_scale_height;
  }
  else
  { mid_scale_height= scale_text_height+2*scale_height;
    bar_height = 
		(meter_height
		-vborder*2
		-scale_height*2
		-MIN_SCALE_BAR*2*2
		-mid_scale_height)/2;
   }
}



void set_scale_bitmap(void)
{ HBITMAP hbitmap, oldbitmap;
  hbitmap = CreateCompatibleBitmap (hmeterDC,mid_scale_width,mid_scale_height);
  oldbitmap = SelectObject ( hscaleDC, hbitmap );
  DeleteObject(oldbitmap);
}

void set_meter_scaling(void)
{ int steps, dB_per_step;
  bar_begin_dB = -60; 
  if (bar_width > 2* min_width)
  { int d;
	d = (int)((-30)*(bar_width-2.0*min_width)/(10.0*min_width));
	if (d>0) d=0;
	if (d<-30) d = -30;
    bar_begin_dB += d;
  }
  steps = bar_width/(2*MIN_BORDER+4*zero_width); /* something like the length of "000-40" */
  if (steps <1) steps = 1;
  dB_per_step = -bar_begin_dB/steps;
  if (dB_per_step <= 3)
    scale_step_dB = -3;
  else if (dB_per_step <= 6)
    scale_step_dB = -6;
  else 
    scale_step_dB = (int)ceil(dB_per_step/10.0)*(-10);
}

void meter_format(void)
/* from meter_height, meter_width, vborder and hborder derive layout */
{ meter_mono = (wavefmtex.nChannels == 1);
  mid_scale_width=meter_width-2*hborder;
  bar_width = meter_width-2*hborder;
  set_heights();
  set_scale_bitmap();

  bar_y[0]=vborder+scale_height+MIN_SCALE_BAR;
  mid_scale_y= bar_y[0]+bar_height+MIN_SCALE_BAR;
  bar_y[1]= mid_scale_y+mid_scale_height+MIN_SCALE_BAR;
  
  set_meter_scaling();
  paint_mid_scale();

  if (log_scale)
  { bar_silence = bar_width*(ag_silence_dB -bar_begin_dB)/(-bar_begin_dB);
	bar_low = bar_width*(ag_low_dB -bar_begin_dB)/(-bar_begin_dB);  
	bar_high = bar_width*(ag_high_dB -bar_begin_dB)/(-bar_begin_dB);  
  }
  else
  { bar_silence = (int)(bar_width*dB_to_factor(ag_silence_dB)); 
	bar_low = (int)(bar_width*dB_to_factor(ag_low_dB));  
	bar_high = (int)(bar_width*dB_to_factor(ag_high_dB)); 
  }
  /* recalculate dynamic values based on old level and gain */
  scale_end=calculate_scale_end(meter_gain);
  bar_end[0]=calculate_bar_end(bar_level[0]);
  if (!meter_mono)
	    bar_end[1]=calculate_bar_end(bar_level[1]);
  InvalidateRect(hMeter,NULL,TRUE);
  UpdateWindow(hMeter); 
}




/* 
 *
 * Redisplaing the window content
 *
 *
 */
static void paint_scale(HDC hdc)
{ 
  paint_scale_lines(hdc,hborder,bar_y[0]-MIN_SCALE_BAR-scale_height);
  if (!meter_mono)
  {	paint_scale_lines(hdc,hborder,bar_y[1]+bar_height+MIN_SCALE_BAR);
  }
  paint_scale_bitmap(hdc,scale_end,mid_scale_width);
}

static void paint_bar(HDC hdc, int channel)
{ paint_bar_from_e_to(hdc,bar_y[channel],0,bar_end[channel],bar_width);
  paint_clip(hdc,bar_y[channel],bar_clip[channel]);
}

static void paint_meter(HDC hdc)
{ paint_bar(hdc,0);
  if (!meter_mono)
	paint_bar(hdc,1);
  paint_scale(hdc);
}

/* 
 *
 * Updating the window content based on dynamic values
 *
 *
 */

static void update_scale(HDC hdc, double gain)
{ int e;
  if (vc_enable & out_scale)
  { meter_gain = gain;
    e = calculate_scale_end(meter_gain);
  }
  else
  { meter_gain = 1.0;
    e = mid_scale_width;
  }
  if (scale_end != e) 
  { paint_scale_bitmap(hdc,e,max(scale_end,e));
    scale_end = e;
  }
}

static void update_bar(HDC hdc, int channel, int c, int level)
{ 
  int b = bar_end[channel];
  int e;
  bar_level[channel] = level;
  e = calculate_bar_end(level);
  if (b<e)
    paint_bar_from_e_to(hdc,bar_y[channel],b,e,e);
  else if (e<b)
    paint_bar_from_e_to(hdc,bar_y[channel],e,e,b);
  bar_end[channel]=e;

  if (c !=bar_clip[channel])
  {	paint_clip(hdc,bar_y[channel],c);
    bar_clip[channel]=c;
  }
}


static void update_meter(void)
{ update_bar(hmeterDC,0, clip_volume(0),peak_volume(0));
  if (!meter_mono) 
    update_bar(hmeterDC,1, clip_volume(1),peak_volume(1));
  update_scale(hmeterDC,volume_compression_gain());
}

static void zero_meter(void)
{ update_bar(hmeterDC,0, 0, 0);
  if (!meter_mono) 
    update_bar(hmeterDC,1, 0, 0);
  update_scale(hmeterDC,1.0);
}


static LONG CALLBACK
MeterWndProc( HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{ 
  switch( msg )
  { case WM_CREATE:
      hMeter = hWnd;
      hmeterDC=GetDC(hMeter);
	  hscaleDC = CreateCompatibleDC (hmeterDC);
	  SelectObject(hscaleDC, hback); /* Background */
      SetBkColor(hscaleDC,GetSysColor(COLOR_BTNFACE));
	  if (timerOn) SetTimer(hMeter,1,100,NULL);
      break;
	case WM_CLOSE:
	  if (meter_detached)
	  { PostMessage(hMainWindow,WM_COMMAND,ID_OPTIONS_DETACH,0);
	    return 0;
	  }
	  break;
	case WM_LBUTTONDBLCLK:
     if (!meter_detached)
	  { PostMessage(hMainWindow,WM_COMMAND,ID_OPTIONS_DETACH,0);
	    return 0;
	  }
	  break;
	case WM_DESTROY:
	  if (!meter_detached)
	  { InvalidateRect(hMainDialog,NULL,TRUE);
	  }
	  KillTimer(hMeter,1);
	  DeleteDC(hscaleDC);
	  break;
	case WM_GETMINMAXINFO:
	  { MINMAXINFO *p;
        p =((MINMAXINFO*)lparam);
		p->ptMinTrackSize.x = min_win_width;
		p->ptMinTrackSize.y = min_win_height;
	  }
	  return 0;
    case WM_SIZE:
      meter_width =LOWORD(lparam);
	  meter_height = HIWORD(lparam);
	  meter_format();
	  break;
	case WM_TIMER: 
      update_meter();
      return 0;
    case WM_PAINT:
    { PAINTSTRUCT ps;
      HDC hdc;
      hdc = BeginPaint (hWnd, &ps);
      paint_meter(hdc);
      EndPaint (hWnd, &ps);
    }
    return 0;     
  }
  return DefWindowProc(hWnd, msg, wparam, lparam);
}

static BOOL RegisterMeterClass(HINSTANCE hinstance) 
{ 
    WNDCLASSEX wcx; 
 
    // Fill in the window class structure with parameters 
    // that describe the main window. 
 
    wcx.cbSize = sizeof(wcx);          // size of structure 
    wcx.style = CS_OWNDC|CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW; // redraw if size changes 
    wcx.lpfnWndProc = MeterWndProc;     // points to window procedure 
    wcx.cbClsExtra = 0;                // no extra class memory 
    wcx.cbWndExtra = 0;                // no extra window memory 
    wcx.hInstance = hinstance;         // handle to instance 
    wcx.hIcon = hm3wIcon;              // predefined app. icon 
    wcx.hCursor = LoadCursor(NULL,IDC_ARROW);  // predefined arrow 
	hback = GetSysColorBrush(COLOR_BTNFACE);
    wcx.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1); // background brush 
    wcx.lpszMenuName =  NULL;          // name of menu resource 
    wcx.lpszClassName = "MeterClass";  // name of window class 
    wcx.hIconSm = NULL; 
 
    // Register the window class. 
 
    return RegisterClassEx(&wcx); 
} 

static BOOL CreateMeter(void) 
{ 

	if (meter_detached)
	{ RECT rc;
	  hborder = vborder = MIN_BORDER;
	  min_win_height = min_height+2*vborder;
	  min_win_width = min_width+2*hborder; 
	  rc.top=0;
	  rc.bottom=min_win_height;
	  rc.left=0;
	  rc.right=min_win_width;
      AdjustWindowRect(&rc,WS_OVERLAPPEDWINDOW,FALSE);
	  min_win_width = rc.right-rc.left;
	  min_win_height = rc.bottom-rc.top;
	  hMeter = CreateWindow( 
        "MeterClass",        // name of window class 
        "Volume",            // title-bar string 
        WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT ,       // default horizontal position 
        CW_USEDEFAULT ,       // default vertical position 
        min_win_width+2*min_width,       // default width 
        min_win_height+min_height,       // default height 
        (HWND) hParent,         // owner window 
        (HMENU) NULL,        // use class menu 
        hInstance,           // handle to application instance 
        (LPVOID) NULL);      // no window-creation data 
	}
	else
	{ hborder = MIN_BORDER;
	  vborder = 0;
	  min_win_width = min_width;
	  min_win_height = min_height+2*vborder;
      hMeter = CreateWindow( 
        "MeterClass",        // name of window class 
        "Volume",            // title-bar string 
        WS_CHILD, // child window 
        3,       // default horizontal position 
        14,       // default vertical position 
        min_win_width,       // default width 
        min_win_height,       // default height 
        (HWND) hParent,         // owner window 
        (HMENU) NULL,        // use class menu 
        hInstance,           // handle to application instance 
        (LPVOID) NULL);      // no window-creation data 
	}
    if (!hMeter) 
        return FALSE; 

    ShowWindow(hMeter, SW_SHOW); 
    UpdateWindow(hMeter); 
    return TRUE; 
 
} 


void meter_create(HINSTANCE inst, HWND wnd)
{  
   if (!RegisterMeterClass(hInstance)) 
        return; 
 
   hInstance = inst;
   hParent = wnd;
   hsilence=CreateSolidBrush(silence_color);
   hlow = CreateSolidBrush(low_color);
   hmid = CreateSolidBrush(mid_color);
   hhigh = CreateSolidBrush(high_color); 
   hclip = CreateSolidBrush(clip_color);
   GetObject(GetStockObject(ANSI_VAR_FONT), sizeof(LOGFONT), &lscale_font); 
   if (hParent)
	{ RECT rp;
	  GetClientRect(hParent,&rp);
	  min_width = rp.right-rp.left-2*(8-MIN_BORDER);
	}
	else
	  min_width = 160;
    min_height = MIN_TOTAL_STEREO;
   meter_detached=0;
   CreateMeter();
}

void meter_delete(void)
{  DeleteObject(hsilence);
   DeleteObject(hlow);
   DeleteObject(hmid);
   DeleteObject(hhigh);
   DeleteObject(hclip);
}

int meter_detach(void)
{  DestroyWindow(hMeter);
   meter_detached = !meter_detached;
   meter_detached_flag = meter_detached;
   CreateMeter();
   return meter_detached;
}



void meter_on(void)
{ timerOn=SetTimer(hMeter,1,100,NULL);
}
void meter_off(void)
{ if (timerOn)
  KillTimer(hMeter,1);  
  zero_meter();   
}

