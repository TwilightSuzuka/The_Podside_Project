// The Function Code
package TPP;

// Returns an int array.
public class Select_Audio_Format {
	
	public static int[] Select_Audio_Format_Function()
	{ 
	System.out.println("Select audio sampling frequency, in Hz (44100 is suggested): ");
	System.out.println("Select the nember of audio channels, mono (1) or sterio (2) \n (mono is suggested to reduce bandwitdh usage and convinience for the listener): ");
	System.out.println("Select the bit size per sample (8, 16, 24) \n(Note: Each sample will contain X bits for mono and X+X bits for sterio.): ");
	System.out.println("PCM?");
	System.out.println("Big endian");
	System.out.println("Something?");
	
	int[] Audio_Format;
	Audio_Format = new int[5];
	return Audio_Format;
	}

}
