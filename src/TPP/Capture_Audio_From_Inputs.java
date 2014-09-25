// The Function Code
package TPP;

// Captures audio from their sources and puts them into a memory location.
public class Capture_Audio_From_Inputs {
	
	public static int[] Capture_Audio_From_Inputs_Function(int Audio_Input1, int Audio_Input2, int[] Audio_Format)
	{ 
	int Audio_Stream1 = 0;
	int Audio_Stream2 = 0;
	System.out.println("Audio is being captured.");
	
	
	int[] Audio_Streams = new int[2];
	Audio_Streams[0] = Audio_Stream1;
	Audio_Streams[1] = Audio_Stream2;
	return Audio_Streams;
	}

}
