/** ************************************************************************************************
 *	Keyboard 
 *  Part of the Wilson project (www.iambobot.com)
 *  Fernando R
 *
 ** ************************************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
using namespace std;

#include "kbd.h"
/* ----------------------------------------------------------------------------------------------------- */
/* -------------------------------------------  KEYBOARD  ---------------------------------------------- */
// restore values
struct termios term_flags;
int term_ctrl;
int termios_init()
{
	// get the original state
	tcgetattr(STDIN_FILENO, &term_flags);
	term_ctrl = fcntl(STDIN_FILENO, F_GETFL, 0);
	return 0;
}
int termios_restore()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &term_flags);
	fcntl(STDIN_FILENO, F_SETFL, term_ctrl);
	return 0;
}
int kbhit(void)
{
	struct termios newtio, oldtio;
	int oldf;
    if (tcgetattr(STDIN_FILENO, &oldtio) < 0) // get the original state
        return -1;
    newtio = oldtio;
	/* echo off, canonical mode off */
    newtio.c_lflag &= ~(ECHO | ICANON );  
	tcsetattr(STDIN_FILENO, TCSANOW, &newtio);
 	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
	int ch = getchar();
 	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
 	tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
	return 0;
}

/* ----------------------------------------------------------------------------------------------------- */
/* ------------------------------------------  CLIbuff ----------------------------------------- */
CLIbuff::CLIbuff(char *bf, string p)
{
	command_b_read= 0;
	command_b_write= 0;
	command_b_length= 0;
	command_b_last= 0;
	prompt= p;
	command_buffer= bf;
}
void CLIbuff::Up()
{
	command_b_read --;
	if(command_b_read<0) command_b_read= command_b_length - 1;
}
void CLIbuff::Down()
{
	command_b_read ++;
	if(command_b_read>=COMMNAD_QUEUE_LENGTH || 	(command_b_length<COMMNAD_QUEUE_LENGTH && command_b_read>=command_b_length) )
		command_b_read= 0;	
}
int CLIbuff::Last()
{
	int ic=0;
	if(command_b_length > 0)
	{
		// Delete last
		ic= command_queue[command_b_last].length();
		printf("\r");
		printf(prompt.c_str());
		for (int i=0; i<ic; i++) printf(" ");						
		printf("\r");
		// Print prompt
		printf(prompt.c_str());
		// take b_read
		strcpy(command_buffer, command_queue[command_b_read].c_str());
		printf("%s", command_buffer);
		command_b_last= command_b_read;
		ic= command_queue[command_b_read].length();
	}
	return ic;
}
void CLIbuff::Store()
{
	string command_line(command_buffer);
	command_queue[command_b_write]= command_line;
	command_b_read= command_b_write;
	command_b_last= command_b_read;  
	command_b_write = (command_b_write + 1 ) % COMMNAD_QUEUE_LENGTH; 
	if(command_b_length < COMMNAD_QUEUE_LENGTH) command_b_length++;
}
// ------------------------------------------------------------------------------------------------
// ---------------- PARSER ------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
unsigned int Parser(string inputline, string argument[])
{
	int ls= inputline.length();
	int i=0;
	unsigned int argn=0;
	char *cstr = new char[inputline.length() + 1];
	strcpy(cstr, inputline.c_str());
	argument[argn]= "";
	while(i<ls && cstr[i]!=0 && cstr[i]==' ') i++;
	while(i<ls && cstr[i]!=0)
	{
		if(cstr[i]==' ' || cstr[i]=='\n') 
		{
			// END OF WORD
			// Remove spaces
			while(i<ls && cstr[i]!=0 && (cstr[i]==' ' || cstr[i]=='\n')) i++;
			//start new word
			if(i<ls && cstr[i]!=0) argument[++argn]="";
		}
		else
			argument[argn] += cstr[i++];
	}
	// done
	delete [] cstr;	
	return ((i==0 || argument[0]=="")? 0 : argn+1);
}


/* END OF FILE */