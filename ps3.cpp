/** ************************************************************************************************
 *	PS3 Controller
 *  Part of the Wilson project (www.iambobot.com)
 *  Fernando R
 *
 ** ************************************************************************************************
*/

#define RUMBLE_EFFECT_ENABLED

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
using namespace std;

#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <linux/joystick.h>

#include "kbd.h"
#include "webclient.h"
#include "../WEBlib.d/WEBlib.h"
#include "joystick.h"
#include "effects.h"
#include "ps3.h"

#define MESSAGE_QUEUE_KEY	1234
#define KBHIT_PERIOD 10000 // 10 msecond
char prompt[]= "PS3> ";

char *version(char *str, size_t max_sz);
bool isNumber(char *s)
{
	unsigned int i=0;
	if(s[i]!='\0' && (s[i]=='-' || s[i]=='+')) i++;
	for (; i<strlen(s) && isdigit(s[i]); i++);
	return !(i<strlen(s));
}

/** 
* Joystick effects 
**/
#ifdef RUMBLE_EFFECT_ENABLED
JoystickEffects je;
#endif

/* ----------------------------------------------------------------------------------------------------- */
/* -----------------------------------------------  MAIN  ---------------------------------------------- */
void SHELL_usage()
{
    fprintf(stdout, "\nUsage:\n");
	fprintf(stdout, "ps3 [options] \n");
	fprintf(stdout, "    -?       - this help\n"); 
	fprintf(stdout, "    -h       - this help\n"); 
	fprintf(stdout, "    --help   - this help\n"); 
	fprintf(stdout, "    --info   - control module versions\n"); 
	fprintf(stdout, "options:\n");
	fprintf(stdout, "    scan - scan bluetooth devices and exit, e.g. \"car scan\"\n");
}

// n= 1 -> argument[0]== command
// n==2 first parameter
int CLI_Interpreter(string argument[], unsigned int n)
{
	string command= argument[0];
	if (command=="usage" || command=="help" || command=="?")
	{
		SHELL_usage(); 
	}
	else if (command=="test") 
	{
		string str= "Action=joystick&x=11&y=12"; 
		webClient_SendMessage((char*)str.c_str(), webClient_callback);
	}
	else if (command=="rumble") 
	{
		int r= (n>1 && isNumber((char*)argument[1].c_str()) ) ? atoi(argument[1].c_str()) : 0;
		je.play(r);
	}		
	else {
		return -1;
	}
	return 0;
}


int main(int argc, char *argv[])
{
	size_t n_numbers= 0;
	int numbers[10];
	char str[128]; // de uso general
	int focus_axis= PS3_AXIS_ANALOG_RIGHT;
	bool option_verbose= true;
	bool option_process= false;
	int rumble= 0;

	// (1) Command line options
	if (argc>1)
	for(int i=1; i<argc; i++)
	{
		if(argv[i][0]=='-') 
		{
			if(argv[i][1]!='-')
			switch(argv[i][1])
			{
				case '?':
				case 'h':
						SHELL_usage();
						exit(EXIT_FAILURE);
					break;
				// -a <focus_axis>
				case 'a':
					if( (i+1) < argc && isNumber((char*)argv[i + 1]) )
					{
						focus_axis= atoi(argv[i+1]);
						fprintf(stdout,"\noption: focus_axis %d", focus_axis); 
					}
					break;
				case 'p':
						option_process= true;
						fprintf(stdout,"\noption: process"); 
					break;					
				// -r <effect>
				case 'r':
					if( (i+1) < argc && isNumber((char*)argv[i + 1]) )
					{
						rumble= atoi(argv[i+1]);
						if(rumble<0 || rumble>=2) rumble= 0;
						fprintf(stdout,"\noption: rumble %d", rumble); 
					}
					break;					
				default:
					break;				
			}
		}
	}	
	
	if(argc > 1) 
	{
		for(int i=1; i<argc; i++)
		{	
			if(isNumber((char*)argv[i]))
			{
				if(n_numbers < (sizeof(numbers)/sizeof(size_t))) 
				numbers[n_numbers++]= atoi(argv[i]);
			}
			else if(strcmp(argv[i],"noverbose")==0 || strcmp(argv[i],"nv")==0) option_verbose= false;
		}
	}	

	fprintf(stdout,"\nPS3 version %s", version(str, sizeof(str))); 
	

#ifdef RUMBLE_EFFECT_ENABLED
	// Add rumble effects
	// a strong rumbling effect 
	je.create(0x8000, 0, 2000, 1000);
	// a weak rumbling effect 
	je.create(0, 0xc000, 5000, 0);
	je.play(rumble);
#endif
	
	// Create message queue for WEBclient process
    pid_t pid_client; 
	// message queue
    if ((webclient_message_queue_id = msgget(MESSAGE_QUEUE_KEY, IPC_CREAT | 0666 )) < 0) {
        perror("msgget");
        exit(1);
    }
	if((pid_client = fork())< 0) perror("Fork failed");	
	if (pid_client == 0)
	{
		web_client((char*)SERVER_NAME, (unsigned int) SERVER_PORT,  (char*) SERVER_PATH);
		fprintf(stdout, "\n[FATAL ERROR] ----------------  process CLIENT failed to create");
		fflush(stdout);
		exit(EXIT_FAILURE);
	}
	else
	{
		size_t axis;
		long xb= 0;
		long yb= 0;
		// joystick device
		int joystick = open(JOYSTICK_DEVICE, O_RDONLY);
		if (joystick == -1)
		{
			perror("Could not open joystick");
			kill(pid_client, SIGKILL);
			fprintf(stdout, "\nFATAL ERROR: joystick device \"%s\" failed\n", JOYSTICK_DEVICE);
			fprintf(stdout, 
				"You need to start sixad:\n"
				"sudo sixad --start\n"
				"and connect the PS3 contoller\n");
			exit(EXIT_FAILURE);
		}	
		struct joystrick_status
		{
			int x;
			int y;	
			int up;
			int down;
		} snow, sprev;
		memset((void*)&snow, 0, sizeof(struct joystrick_status));
		memset((void*)&sprev, 0, sizeof(struct joystrick_status));
		int do_move= false;
		int count_less=0;		
		
//		fprintf(stdout,"\nJoystick axis= %d buttons= %d", get_axis_count(joystick), get_button_count(joystick));
		// main process
		string command_line;
		char command_line_charbuffer[200] = {0};
		string arguments[10];
		int ic=0;
		unsigned int n=0;
		char c= 0;
		command_line_charbuffer[0]=0;
		
		// Command line
		CLIbuff* CLB = new CLIbuff(command_line_charbuffer, prompt);	
		fprintf(stdout, "\nPS3 Controller command line\n");
		fprintf(stdout, prompt);
		fflush(stdout);
		if(!option_process) termios_init();
			
		while(1)
		{
			while(option_process || !kbhit()) 
			{
				usleep(1000); // Needed. otherwise buttons values result random
				struct js_event event; 
				memset(&event, 0, sizeof(event));
				if (read(joystick, &event, sizeof(event)) == sizeof(event))
				{
					switch (event.type)
					{
						case JS_EVENT_BUTTON:
						{
							fprintf(stdout,"\nButton %u %s", event.number, event.value ? "pressed" : "released");						
							if(event.number == PS3_BUTTON_TRIANGLE) 
							{
								fprintf(stdout," TRI %s", event.value == 0 ? "up" : "down");
								snow.up= event.value;
								if(snow.up != sprev.up)
								{
									sprev.up= snow.up;
									if(snow.up != 0)
									{
										do_move= true;
										webClient_SendMessage((char*)"Action=setlight&value=1", webClient_callback);
									}
								}
							}
							if(event.number == PS3_BUTTON_X)
							{
								fprintf(stdout," X %s", event.value == 0 ? "up" : "down"); 
								snow.down= event.value;
								if(snow.down != sprev.down)
								{
									sprev.down= snow.down; 
									if(snow.down != 0)
									{
										do_move= false; 
										webClient_SendMessage((char*)"Action=setlight&value=0", webClient_callback);
										webClient_SendMessage((char*)"Action=stop", webClient_callback);
									}
								}							
							}	
							if(event.number == PS3_BUTTON_CIRCLE && event.value != 0 ) 
							{
								fprintf(stdout," CICLE"); 
#ifdef RUMBLE_EFFECT_ENABLED
								je.play(0); 
#endif
							}
							if(event.number == PS3_BUTTON_SQUARE && event.value != 0 ) 
							{
								string str= "Action=blink"; 
								webClient_SendMessage((char*)str.c_str(), webClient_callback);
							}
						}
						break;
						case JS_EVENT_AXIS:
						{
							int xe= sprev.x;
							int ye= sprev.y;							
							/**
							* 					Event 	Axis	XY
							*	ANALOG LEFT		0  		0		x
							*					1		0		y
							*	ANALOG RIGHT	2		1		x
							*					3		1		y
							*	GYROSCOPE		4		2		x	ROLL - longitudinal axis
							*					5		2		y	PITCH - lateral axis
							**/
							axis = event.number / 2;
							if (event.number % 2 == 0)	xe = event.value;
							else						ye = event.value;
							if (axis == (size_t) focus_axis)
							{
								// event's X and Y corrdinates goes from -32.000 .. +32.000
								if(option_verbose)fprintf(stdout,"\nAxis %zu at (%6d, %6d)", axis, xe, ye);
								if(count_less == 0)
								//if(1)
								{
									if(axis == PS3_AXIS_GYRO) count_less= 50; // limit event to process		
									// Calculate delta
									int dxe= xe - sprev.x;
									int dye= ye - sprev.y;
									int sxe= dxe>=0? 1 : -1;
									int sye= dye>=0? 1 : -1;
									// dismiss values that are below a threshold
									if( (sxe*dxe) > 1000 || (sye*dye) > 1000)
									{
										//if(sxe*dxe > 5) 
											sprev.x = xe;
										//if(sye*dye > 5) 
											sprev.y = ye;
										long x= xe * 255 / 32768;
										long y= ye * 255 / 32768;
										if(x != xb || y != yb)
										{
											if(option_verbose) fprintf(stdout," XY= %d %d", (int)x, (int)y);
											snprintf(str, sizeof(str),"Action=joystick&x=%d&y=%d",(int)x, (int)(-1*y)); 
											if(do_move) webClient_SendMessage(str, webClient_callback);
											xb= x;
											yb= y;
										} 
									} 
								}
								if(count_less!=0) count_less--;
							}
						}
						break;
						case 129:
						case 130: 
						break;
						default:
							fprintf(stdout,"\nUnknown event %u", event.type);
							/* Ignore init events. */ 
							break;
					}
					fflush(stdout);
				}
			}
			
			// key pressed
			switch(c = getchar()) 
			{
				case '\n': 
				{
					//putchar(c); // Echo
					string command_line(command_line_charbuffer);
					// Parse commandline
					n= Parser(command_line, arguments);
					if(n>0) 
					{
						if(CLI_Interpreter(arguments, n) < 0) 
						{
							fprintf(stdout,"\nUnknown command %s", arguments[0].c_str());
						}
						// Store command
						CLB->Store();
					}
					ic=0;
					command_line_charbuffer[0]=0;						
					fprintf(stdout, "\n%s", prompt);
					fflush(stdout);
				}
				break;
				case '\b':
				case 127:
				{
					if(ic>0) 
					{
						printf("\b \b"); // Backspace
						command_line_charbuffer[--ic]= 0;
					}
				}
				break;
				// ESCAPE
				case 27:
				{
					// "throw away" next two characters which specify escape sequence
					char c1=0;
					char c2=0;
					if(kbhit()) 
					{
						c1 = getchar();
						if(kbhit()) c2 = getchar();
					}
					switch(c2) 
					{
						// Simple ESC key
						case 0:
							printf("\nPROGRAM TERMINATED !\n");
							kill(pid_client, SIGKILL);
							close(joystick);
							//je.~JoystickEffects();
							termios_restore();
							exit(EXIT_SUCCESS);  // parent exits
						break;
						// UP
						case 65:
						// DOWN
						case 66:
							ic= CLB->Last();
							// UP
							if(c2==65)
							{
								CLB->Up();
							}
							// DOWN
							if(c2==66)
							{
								CLB->Down();								
							}							
						break;
						case 67:
							 cout << "RIGHT" << endl; // key right
							printf (prompt);
						break;
						case 68:
							cout << "LEFT" << endl; // key left
							printf (prompt);
						break;
						default: 
							printf("DEFAULT %d\n", (int)c1); 
							printf (prompt);
					}
				}
				break;			
				default: 
				{
					printf("%c", c); // Echo
					command_line_charbuffer[ic++]= c;
					command_line_charbuffer[ic]= 0;
				}
			}		
		}
	}
	return 0;
}

/* END OF FILE */