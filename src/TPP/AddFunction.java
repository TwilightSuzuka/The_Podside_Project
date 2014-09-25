/* The Main file code.

package TPP;

public class Main {

	public static void main(String[] args) 
	{
	int answer = AddFunction.addFunction(3, 5);
	System.out.println("Answer = " + answer);
	}
	
}

**********************************************

// The Function Code
package TPP;

public class AddFunction {
	
	public static int addFunction(int a, int b)
	{
	answer = a + b;
	return answer;
	}

}

*/

/*
// Main file code.
package TPP;

public class Main {

	public static void main(String[] args) 
	{
	int[] answer; //= 0;
	answer = new int[2];
	answer = AddFunction.addFunction(3, 5, answer);
	System.out.println("Answer = " + answer[0] + " and " + answer[1]);
	}
}
 */

// The Function Code
package TPP;

public class AddFunction {
	
	public static int[] addFunction(int a, int b, int[] answer)
	{
	answer[0] = a + b;
	answer[1] = a + b + 1;
	return answer;
	}

}
