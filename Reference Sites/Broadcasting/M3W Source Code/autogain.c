/* autogain.c $Revision: 1.6 $ $Date: 2011/06/05 19:06:38 $
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
#include <stdio.h>
#include <math.h>
#include "main.h"
#include "param.h"
#include "sound.h"
#include "autogain.h"

static HMIXER hmx;
static DWORD Mux_ID = 0;
static DWORD wavin_LineID = 0;
static DWORD wavin_Controls = 0;
static DWORD Mux_Items = 0;

static int peak[2]= {0,0};
static int clip[2]= {0,0};
static double rms[2]= {0,0};

#define MAX_SLOTS 50
static int slot_index = 0;
static int slot_peaks[MAX_SLOTS];
static int hi_slots = 0;
static int lo_slots = 0;

static void init_slots(void)
{ int i;
  slot_index = 0;
  hi_slots = 0;
  lo_slots = 0;
  for (i = 0; i< MAX_SLOTS; i++)
    slot_peaks[i] = 0;
}

static int ag_high_level, ag_low_level, ag_silence_level;

void set_ag_levels(void)
{ int i;
  ag_high_level = dB_to_level(ag_high_dB);
  ag_low_level = dB_to_level(ag_low_dB);
  ag_silence_level = dB_to_level(ag_silence_dB);
  hi_slots=lo_slots=0;
  for (i=0;i<MAX_SLOTS;i++)
    if (slot_peaks[i] > ag_high_level) 
	  hi_slots++; 
    else if (slot_peaks[i] != 0 && slot_peaks[i] < ag_low_level)
      lo_slots++;  
}

static void update_Mixer_Volume(int percent);

static void auto_gain(int peak)
{ if (!ag_on|| ag_disable)
    return;
  if (peak < ag_silence_level)
    return;
  if (slot_peaks[slot_index] > ag_high_level) hi_slots--;
  else if (slot_peaks[slot_index] != 0 && slot_peaks[slot_index] < ag_low_level) lo_slots--;
  slot_peaks[slot_index] = peak;
  if (slot_peaks[slot_index] > ag_high_level) 
    hi_slots++; 
  else if (slot_peaks[slot_index] != 0 && slot_peaks[slot_index] < ag_low_level)
    lo_slots++;  
  slot_index++;
  if (slot_index >= MAX_SLOTS) slot_index = 0;
  if (hi_slots <= (MAX_SLOTS/10) && lo_slots > (2*MAX_SLOTS/3)) 
  { update_Mixer_Volume(ag_step);
    init_slots();
  }
  else if (hi_slots >(MAX_SLOTS/5) && lo_slots < (MAX_SLOTS/3)) 
  { update_Mixer_Volume(-ag_step);
    init_slots();
  }
}


void  check_volume(LPWAVEHDR p, int stereo, int freq)
{ int n,i,m,sp,j;
  short int k;
  short int *pData=(short int *)p->lpData;
  int sps;
  int last;
  int first;
  int rms_count;
  sps = freq/10; /* samples per slot = 1/10 s */
  n = p->dwBytesRecorded/2;
  j = 1; /* increment, you can be lazy and check only every 17 or so sample */
  if (stereo) 
  { j = j*2;
    sps = sps*2;
  }
  first = 0;
  peak[0] = peak[1] = 0;
  rms[0] = rms[1] = 0.0;
  do 
  {
    i = first;
    last = first + sps;
    if (last > n) last = n;
    m = 0;
	rms_count=0;
    while (i < last)
	{ k = pData[i];
	  k = k ^ (k>>15); /* k = k<0?-k:k; */
      if (k > m) m = k;
	  i = i + j; 
	  rms_count++;
	  rms[0] += (double)k * (double)k;
	}
    sp = m;
    if (m > peak[0]) peak[0] = m;
    rms[0] = sqrt(rms[0]/rms_count);
    if (stereo)
	{  
	   i = first+1;
	   last = first+1+sps;
       if (last > n) last = n;
       m = 0;
	   rms_count=0;
	   while (i < last)
	   {  k = pData[i];
	      k = k ^ (k>>15); /* k = k<0?-k:k; */
	      if (k > m) m = k;
		  i = i + j; 
		  rms_count++;
	      rms[1] += (double)k * (double)k;
	   }
       if (sp <m ) sp = m;
	   if (peak[1] <m) peak[1] =m;
	   rms[1] = sqrt(rms[1]/rms_count);
	}
	auto_gain(sp);
    first = first + sps;
  } while (first < n );
}

int peak_volume(int channel)
{ 	// Display peak on log scale from -60dB to 0dB
	return peak[channel];
}

int rms_volume(int channel)
{ 
	return (int)(rms[channel]); 
}

int clip_volume(int channel)
{ 
	return peak[channel]>=0x7FFF; 
}


void init_Mixer_device(HWND hwnd, HWAVEIN hwavein)
/* tries to find and open a corresponding Mixing device
   if successful it sets wavin_LinID and wavin_Controls
   if unsuccessful,sets wavin_Controls to zero
*/
{ int errno; 
  MIXERLINE mxl;
  mxl.cbStruct = sizeof(mxl);
  mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN ;

  /* get Number of devices and needed to load dll */
  if (mixerGetNumDevs()==0)
  	 message(0,"No Mixer found");
  else  if (hwavein == 0)
     message(0,"No soundcard open");
  else if ((errno =mixerOpen(&hmx,(UINT)hwavein,(UINT)hwnd,0,MIXER_OBJECTF_HWAVEIN | CALLBACK_WINDOW))
	 != MMSYSERR_NOERROR)
     message(0,"Unable to open Mixer");
  else if ((errno = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl,MIXER_GETLINEINFOF_COMPONENTTYPE))
	 != MMSYSERR_NOERROR) /* find the destination that belongs to the wavein line*/
  { message(0,"Unable to get Mixer Line Info");
    mixerClose(hmx);
  }
  else if (mxl.cControls == 0)
  { message(0,"No controls available for this input line");
    mixerClose(hmx);
  }
  else /* success */
  { wavin_LineID = mxl.dwLineID;
    wavin_Controls = mxl.cControls;
	ag_disable=0;
	display_autogain();
	return;
  }
  /* no luck here */
   wavin_LineID = wavin_Controls =ag_on = Mux_ID = Mux_Items =0; 
   ag_disable=1;
   display_autogain();
}

void init_Mixer_data(void)
{ int i;
  int errno; 
  MIXERLINECONTROLS mxlc;
  LPMIXERCONTROL mux_mxctrl;
  if (wavin_Controls == 0 ) 
	return;
/* computes MuxID and Mux_Items  from hwavein */
  /* get destination controls */
  mxlc.cbStruct = sizeof(mxlc);
  mxlc.cbmxctrl = sizeof(MIXERCONTROL);
  mxlc.dwLineID = wavin_LineID;
  mxlc.cControls = wavin_Controls;
  mxlc.pamxctrl  = mux_mxctrl = malloc(mxlc.cbmxctrl*mxlc.cControls);
  if (mux_mxctrl==NULL)
  { errormsg("Out of memory",0); return; }
  errno = mixerGetLineControls((HMIXEROBJ)hmx,&mxlc,MIXER_GETLINECONTROLSF_ALL);
  if (MMSYSERR_NOERROR != errno) 
  { errormsg("Unable to get Mixer Line Controls",errno); 
    free(mux_mxctrl); 
	close_Mixer_data();
	return; 
  }
  for (i = 0; i < (int)wavin_Controls; i++)
  {  /* skip disabled controls */
	  if ( mux_mxctrl[i].fdwControl & MIXERCONTROL_CONTROLF_DISABLED)
	    continue;
	  if ((mux_mxctrl[i].dwControlType & MIXERCONTROL_CT_UNITS_MASK )
			   != MIXERCONTROL_CT_UNITS_BOOLEAN)
		continue;
	  if ((mux_mxctrl[i].dwControlType & MIXERCONTROL_CT_CLASS_MASK )
			   != MIXERCONTROL_CT_CLASS_LIST)
		continue;
      Mux_ID =   mux_mxctrl[i].dwControlID;
	  Mux_Items = mux_mxctrl[i].cMultipleItems;
	  break; /* take the first */
  }		  
  free(mux_mxctrl);
}


void close_Mixer_data(void)
{ mixerClose(hmx);
  wavin_LineID = wavin_Controls = Mux_ID = Mux_Items =0; 
}


void change_Mixer_Control(HMIXER hMixer,DWORD dwControlID)
{ if (hMixer != hmx)
    return;
  if (dwControlID != Mux_ID)
	return;
  set_Mixer_data();
  
}

void change_Mixer_Line(HMIXER hMixer,DWORD dwLineID)
{ if (hMixer != hmx)
    return;
  if (dwLineID != wavin_LineID)
	return;
  set_Mixer_data();
}

static int num_vol = 0;
static DWORD *vol_ID = NULL;
static DWORD *vol_min = NULL;
static DWORD *vol_max = NULL;

void set_Mixer_data()
{  MIXERCONTROLDETAILS mxcd;
   int errno;
   LPMIXERCONTROLDETAILS_BOOLEAN pmxcd_b;
   LPMIXERCONTROLDETAILS_LISTTEXT pmxcd_lt;
   MIXERLINECONTROLS mxlc;
   MIXERCONTROL vol_mxctrl;

   int count, i;
   if (Mux_Items <=0) return;

   /* get all the details for the controll */
   mxcd.cbStruct       = sizeof(mxcd);
   mxcd.dwControlID    = Mux_ID;
   mxcd.cChannels      = 1;
   mxcd.cMultipleItems = Mux_Items;
   /* get booleans */
   mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
   mxcd.paDetails      = pmxcd_b = malloc(mxcd.cbDetails*Mux_Items);
   if (pmxcd_b==NULL)
   { errormsg("Out of memory",0); return; }

   errno = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);
   if (MMSYSERR_NOERROR != errno) 
   { errormsg("Unable to get Volume Control Value",errno); free(pmxcd_b); return; }
   /* get names */
   mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
   mxcd.paDetails      = pmxcd_lt = malloc(mxcd.cbDetails*Mux_Items);
   if (pmxcd_lt==NULL)
   { errormsg("Out of memory",0); free(pmxcd_b); return; }
   errno = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_GETCONTROLDETAILSF_LISTTEXT);
   if (MMSYSERR_NOERROR != errno) 
   { errormsg("Unable to get Volume Control Text",errno); free(pmxcd_b); free(pmxcd_lt); return; }
   count = 0;
   for (i=0; i< (int)Mux_Items; i++)
	 if (pmxcd_b[i].fValue) count++;

   if (vol_ID!=NULL && count > num_vol)
   { free(vol_ID);
     vol_ID = vol_min = vol_max = NULL;
   }
	 
   if (vol_ID == NULL )
   {   if (count > 0) 
		{ vol_ID  = malloc(sizeof(DWORD) * count*3);
          if (vol_ID == NULL)
		  {  errormsg("Out of memory",0); return; }
		}
       vol_min = vol_ID+count;
       vol_max = vol_min+count;
	   num_vol = count;
   }
   count = 0;
   for (i=0; i< (int)Mux_Items; i++)
	 if (pmxcd_b[i].fValue)
	 { mxlc.cbStruct = sizeof(mxlc);
	   mxlc.cbmxctrl = sizeof(MIXERCONTROL);
	   mxlc.pamxctrl  = &vol_mxctrl;
	   mxlc.dwLineID = pmxcd_lt[i].dwParam1;
	   mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
       errno = mixerGetLineControls((HMIXEROBJ)hmx,&mxlc,MIXER_GETLINECONTROLSF_ONEBYTYPE);
       if (MMSYSERR_NOERROR != errno) 
	     errormsg("Unable to get Volume Control ID",errno);
	   else
	   { vol_ID[count] = vol_mxctrl.dwControlID;
	     vol_min[count] = vol_mxctrl.Bounds.dwMinimum;
	     vol_max[count] = vol_mxctrl.Bounds.dwMaximum;
	     count++;
	   }
	 }

   free(pmxcd_lt);
   free(pmxcd_b);
}



static void update_Mixer_Volume(int percent)
{ int i;
  int errno;
  int vol;
  MIXERCONTROLDETAILS mxcd;
  MIXERCONTROLDETAILS_UNSIGNED mxcd_u;

  for (i=0; i< num_vol; i++)
  { mxcd.cbStruct       = sizeof(mxcd);
    mxcd.dwControlID    = vol_ID[i];
    mxcd.cChannels = 1;
    mxcd.cbDetails      = sizeof(mxcd_u);
    mxcd.paDetails      = &mxcd_u;
    mxcd.cMultipleItems = 0;
    errno = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);
    if (MMSYSERR_NOERROR != errno) 
	  errormsg("Unable to get Volume",errno);
    else
	{ int step;
	  vol = mxcd_u.dwValue;
	  step = (int)((vol_max[i]-vol_min[i])*(percent/100.0));
	  vol = vol + step;
	  if (vol <= (int)vol_min[i])
		  return; /* don't turn it down to zero volume, it will never come up again */
	  if (vol > (int)vol_max[i]) vol = vol_max[i];
	  mxcd_u.dwValue = vol;
      errno = mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);
      if (MMSYSERR_NOERROR != errno) errormsg("Unable to set Volume",errno);
	}
  }
}


