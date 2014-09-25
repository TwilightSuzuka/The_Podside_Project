/* volumecompression.c $Revision: 1.5 $ $Date: 2011/06/08 02:56:16 $
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

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <math.h>

#include "main.h"
#include "param.h"
#include "sound.h"
#include "volumecompression.h"
#include "resource.h"

// Parameters controlling the behavior of the volume compression
static const double scale = (1.0/32768.0);			// Inverse of desired full-scale value
static double compression_factor = -17.0/20.0;  // (-20-target_dB)/20
static double noise_rejection = 0.0;			// Amount of attenuation to use during noise segments
static double noise_floor = 2.367600721920000e+005;  // (1e-3 * 32768)^2 * fs/100
static double af=0.99999546485261;  // 1-dt/(falltime)  = 1 - 1/(22050*10)
static double ar=0.00136054421769;  // dt/(risetime)    = 1/(22050*(1/30))
static double hpf=2.849517146113191e-004;  // dt*(2*pi*f) = (2*pi*1.0)/(22050)
static double tau=220500.0;  // falltime/dt = 10*22050
static int history_count=0;  // delay*fs = 0.1*22050
static int noise_blocks=20;  // 2*delay/sps*fs = 2*delay*100 = 2*0.1*100 = 20
static int peak_hold_samples=0; // Hold peak value for 3*rise_time

// Internal state of the volume compression
static double gain=1.0;	// Current gain value
#define TOP_PEAK 0x7FFF
static double vc_peak=TOP_PEAK;		// Start out with a maximum peak so we start where no cmpression left off.
static double peak_hold=TOP_PEAK;
static int peak_delay=0;			// Don't decay peak until this value is 0
static double peak_hold_u=0;              // u value belonging to current peak_hold
static int signal_present=1;		// Positive when signal was recently present
static int history_ptr=0;
static double *history_buffer = NULL;
static double mean[2]={0.0, 0.0};	// high-pass-filter state

void set_volume_compression_parameters(int target_dB, int vc_risetime, int vc_falltime, int vc_noisefloor, int vc_noiseattenuation)
{


	// Get the current sample rate, and abort if we don't have a reasonable setting
	double fs=get_samples_per_sec();

	if((fs < 1000) || (fs >= 200000))
		return;

	// Force sane limits on the parameters
	if(target_dB>0) target_dB=0;
	if(target_dB<-40) target_dB=-40;
	if(vc_risetime<1) vc_risetime=1;
	if(vc_risetime>500) vc_risetime=500;
	if(vc_falltime<1) vc_falltime=1;
	if(vc_falltime>600) vc_falltime=600;
	if(vc_noisefloor<20) vc_noisefloor=20;
	if(vc_noisefloor>100) vc_noisefloor=100;
	if(vc_noiseattenuation>0) vc_noiseattenuation=0;
	if(vc_noiseattenuation<-60) vc_noiseattenuation=-60;

	// Set a 1Hz corner on the high pass filter
	hpf = (2*3.1415927*1.0)/fs;

	// Set the volume compression values to these parameter settings
	compression_factor = (-20-target_dB)/20.0;
	tau = fs*vc_falltime;
	af = exp(-1.0/tau);
	ar = 1.0-exp(-1.0/(fs*0.001*vc_risetime));
	noise_floor = pow(0.1, 0.05*vc_noisefloor)*32768.0;
	noise_floor *= noise_floor * 0.01*fs;	
	// Peak hold samples is the number of samples to hold a peak value
	// before we allow it to decay.  Set to 3*risetime so that the
	// peak is held long enough for the rise time to be reached
	peak_hold_samples = (int)floor(3*0.001*vc_risetime*fs+0.5);

	// Noise blocks is the number of 1/100 sec intervals that must be below
	// the noise floor before switching into no-signal-present mode
	noise_blocks = (6*vc_risetime+9)/10;
	if(noise_blocks < 3)
		noise_blocks = 3;
	// 300 blocks is 3 seconds
	if(noise_blocks > 300)
		noise_blocks = 300;
	signal_present=noise_blocks;
}

void set_volumecompression(int on)
{   vc_enable = on;
	if(vc_enable) {
		int new_count;
		double fs;
		int stereo;
		stereo=get_stereo_flag();
		fs=get_samples_per_sec();
		new_count = (int)floor(0.003*vc_risetime*fs+0.5);
	    // Stereo mode requires buffering double the number of samples
	    if(stereo)
		  new_count *= 2;
	    if(new_count < 2)
		  new_count = 2;
	    if(new_count > 100000)
		  new_count = 100000;
	    if((history_buffer != NULL) && (new_count != history_count)) {
		  free(history_buffer);
		  history_buffer = NULL;
	    }
		reset_volumecompression();
		// Only reset the buffer if length changes, or buffer not previously allocated
		if(history_buffer == NULL) {
			history_ptr = 0;
			history_count = new_count;
			history_buffer = (double *)malloc(history_count * sizeof(double));
			memset(history_buffer, 0, history_count*sizeof(double));
		}
	} else 
		stop_volumecompression();

}

void reset_volumecompression(void)
{	// Reset our state so we start up fresh
	mean[0] = 0;
	mean[1] = 0;
	gain = 1.0;
	vc_peak = TOP_PEAK;
	peak_hold = TOP_PEAK;
	peak_hold_u = pow(peak_hold*scale, compression_factor);
	peak_delay = 0;
	signal_present = noise_blocks;
}

void stop_volumecompression()
{   vc_enable = 0;
}


static void update_peak(double in)
{ if(-vc_peak<= in)
  { if (in <= vc_peak)
      return; // no new peak
    else
	  vc_peak = in;
  }
  else
	vc_peak = -in;
  // new peak
  if(vc_peak > peak_hold) {
	  // New, higher peak  
	  //removed from condition:(peak_delay == 0) || drops peak_hold disregarding falltime to vc_peak
	  peak_hold = vc_peak;
         peak_hold_u = pow(peak_hold*scale, compression_factor);
	  peak_delay = peak_hold_samples; // keep as long as peak is in buffer
  } else {
	  // New peak, but lower than the held peak
	  // Extend the hold time to envelope this peak
	  int t2 = (int)floor(tau*log(peak_hold/vc_peak)+0.5); // time for peak_hold to fall to vc_peak
	  if (t2<=0) peak_delay = peak_hold_samples; // maximum
	  else peak_delay = peak_hold_samples-t2; // just enough
  }
}

void update_gain(void)
{ 
  if(signal_present > 0)     /* Apply volume compression */
  { double u;
	if(peak_hold > 0)
	  u = pow(peak_hold*scale, compression_factor);
	else
	  u = gain;
	/* Limit the amount of gain to 60dB */
	if(u > 1000)
	  u = 1000;
	gain = gain + ar*(u-gain);
	if(peak_delay>0) peak_delay--;
	else peak_hold *= af;
    vc_peak *= af;
  } 
  else /* Apply noise gating */
  {	gain = gain + ar*(noise_rejection-gain);
  } 
}


static double high_pass(short int in, int k)
{ double d;
  d = in - mean[k];
  mean[k] += hpf*d;
  return d;
}

static short int round_to_sample(double out)
{ if(out > 32767) return 32767;
  else if(out < -32767) return -32767;
  return (short int)floor(out+0.5);
}

void  volume_compression(LPWAVEHDR p, int stereo, int freq)
{ 
	int i,j;
	double rms,left, right,out;
	int sps = freq/100; /* samples per slot = 1/100 s */
	int n = p->dwBytesRecorded/2;
	short int *ptr = (short int *)p->lpData;

	// Don't do anything until parameters are set
	if(!vc_enable || (history_buffer == NULL))
		return;

	// In stereo mode, must process data in pairs
	if(stereo && ((sps%1)==1))
		sps++;

	i=0;
	while(i<n) {
		/* Compute RMS on 10ms intervals, for noise gating */
		rms=0;
		for(j=0; (j<sps) && (i<n); j++,i++) {
			// Apply a high-pass-filter to remove any DC offset
			left = high_pass(ptr[i],0);
			rms += left*left;
            update_peak(left);
			if(stereo) // Handle the odd stereo sample
			{   right = high_pass(ptr[i+1],1);
				rms += right*right;
				update_peak(right);
			}
			update_gain();
			out = gain*history_buffer[history_ptr];
			history_buffer[history_ptr] = left;
			ptr[i] =round_to_sample(out);
			if(++history_ptr >= history_count) history_ptr = 0;
			if(stereo) {// Handle the odd stereo sample
				i++;
				out = gain*history_buffer[history_ptr];
				history_buffer[history_ptr] = right;
				ptr[i] =round_to_sample(out);
				if(++history_ptr >= history_count) history_ptr = 0;
			}
#if 0
			/* Log the values for debugging purposes */
			{
				static FILE *fp=NULL;
				if(fp == NULL)
					fp=fopen("c:\\temp\\vc.log","w");
				if(fp != NULL)
					fprintf(fp,"%d\t%g\t%g\t%g\t%g\t%g\t%g\n", ptr[i], in, mean, vc_peak, u, gain, out);
			}
#endif      


		}
		// each 10 ms update signal present
		// Stereo mode includes double the samples, so we need to scale down
		if(stereo) rms *= 0.5;
		if(rms > noise_floor) signal_present = noise_blocks;
		else if(signal_present > 0) 
		{ signal_present--;
		  if (signal_present==0)
		  {	 if(vc_noiseattenuation < 0) /* set noise rejection as a difference to current gain */
		       noise_rejection = gain*pow(10.0, 0.05*vc_noiseattenuation);
		     else
			  noise_rejection = gain;
		  }
		}
	}
}

double volume_compression_gain(void)
{
	// Display current gain level, in dB, range 0dB to 60dB
	//double p=log(gain)*4743.653861001919;
	// Display the current peak hold detection, range -60dB to 0dB

	return gain;
#if 0
	double p=(log(peak_hold)-3.48945242941704)*4743.653861001919;
	if(p<0)
		return 0;
	if(p>32767)
		return 32768;
	return (int)floor(p+0.5);
#endif
}
