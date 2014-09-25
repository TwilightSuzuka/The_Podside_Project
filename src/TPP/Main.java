/*
 * Internet AV transmission: 
 * 		- http://www.oracle.com/technetwork/java/javase/documentation/rtptransmit-177022.html
 * 		- http://www.oracle.com/technetwork/java/javase/samplecode-138571.html#RTPSocketPlayer
 * 		- (Transmission only) http://www.oracle.com/technetwork/java/javase/documentation/toolstx-178270.html 
 * 		  (UI-Based Application for Transmission of Media over RTP)
 * 		- (Reception only) http://www.oracle.com/technetwork/java/javase/documentation/toolsrx-178271.html 
 * 		  (UI-Based Application for Reception of Media over RTP)
 * 
 * WAV to MP3 conversion: 
 * 		- http://www.oracle.com/technetwork/java/javase/features-140218.html
 * 		- MP3 to WAV converter: http://java2everyone.blogspot.com/search?updated-min=2011-01-01T00:00:00%2B08:00&updated-max=2012-01-01T00:00:00%2B08:00&max-results=1
 * 
 * 
 * 
 * Plan of action:
 * 		- Capture mic audio and store in RAM as a dynamic array (.WAV file). 
 * 			- (Later) Capture the original audio in MP3 format.
 * 		- Capture 'Windows' audio and store in RAM as a dynamic array (.WAV file).
 * 			- (Later) Capture the original audio in MP3 format.
 * 			- (Later) Experiment with adding/merging the 2 sound files together at this point.
 * 		- (Later) Convert both to MP3
 * 		- Merge/join/combine the 2 audio files.
 * 		- Save the Combined audio file to HDD (but keep it in RAM for later transmission).
 * 			- (Later) "Progressive Saving" - Save to the HDD every X minutes (while keeping the full 
 * 		  	  audio file in RAM). This will allow for the program/computer to crash and for there to 
 * 		  	  be a fully functional saved recording.
 * 		- Transmit audio stream over TCP/IP
 * 		- Stop program.
 * 			- Save all audio to HDD.
 * 			- Terminate process.
 * 
 */

// Code format:
// - Display and Select Audio I/O
// - Select Audio Format
// - Capture Audio Input Streams
// - Collate Audio Streams
// - Convert to mp3
// - Save Audio
// - Push Converted Audio to the Broadcast Server
// - Shutdown

package TPP;
import java.io.*;


public class Main {
	
	public static void main(String[] args) 
	{
		System.out.println("This is a Spare Change Inc. (Reginald Aryee's future company) 'product' called The Podside Project which is free to use and distribute for testing, research, and non-profit purposes.");
		
		int Audio_Input1 = 0;				// Microphone
		int Audio_Input2 = 0;				// PC Audio (Audio begin played through the speakers)
		int[] Audio_Format = new int[4];	// Audio formatting (Sample rate [44100Hz, 22050Hz], Number of Channels [1 or 2], Bits/Sample [8 or 16], Big Endian)
		int Audio_Stream1 = 0;				// Microphone Audio Stream (stored in RAM)
		int Audio_Stream2 = 0;				// PC Audio Stream (stored in RAM)
		int Collated_Audio_Stream = 0;		// Combined Microphone and PC Audio Stream (stored in RAM)
		int Converted_Audio_Stream = 0;		// Combined Audio Stream in MP3 Format
		String Filename = null;				// Filename for the saved MP3 file
		String WebAddress = null;			// URL for the broadcast server
		int PortNumber = 0;					// Port to use for the broadcast server
		
	// Display and Select Audio I/O
	Display_and_Select_IO.Display_and_Select_IO_Function();
	
	/********************************************************************************/
	// Select Audio Format
	Select_Audio_Format.Select_Audio_Format_Function();
	
	/********************************************************************************/
	// Get miscellaneous info (MP3 filename, broadcast server website address, broadcast server port number).
	//input();
	
	/********************************************************************************/
	// Capture Audio Input Streams
	Capture_Audio_From_Inputs.Capture_Audio_From_Inputs_Function(Audio_Input1, Audio_Input2, Audio_Format);
	
	System.out.println("Capturing audio input streams.");
	/********************************************************************************/
	// Collate Audio Streams
	Collate_Audio.Collate_Audio_Function(Audio_Stream1, Audio_Stream2, Audio_Format);
	
	System.out.println("Collating audio streams.");
	/********************************************************************************/
	// Convert to mp3
	Convert_Audio_To_MP3.Convert_Audio_To_MP3_Function(Collated_Audio_Stream, Audio_Format);
	
	System.out.println("Converting audio stream to mp3 format.");
	/********************************************************************************/
	// Save Audio
	Save_Audio.Save_Audio_Function(Converted_Audio_Stream, Filename);
	
	System.out.println("Started saving mp3 file to the hard drive.");
	/********************************************************************************/
	// Push Converted Audio to the Broadcast Server
	System.out.println("Starting connection to the broadcast server.");
	Push_To_Broadcast_Server.Push_To_Broadcast_Server_Function(Converted_Audio_Stream, WebAddress, PortNumber);
	
	System.out.println("Streaming audio to the broadcast server.");
	/********************************************************************************/
	// Shutdown
	System.out.println("Starting shutdown procedure.");
	Shutdown.Shutdown_Function();
	
	System.out.println("Shutdown finished. Goodbye.");
	
	}
	
}

