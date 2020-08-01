/*
* Web Client
* 
*/
//#define	WEBCLIENT_DEBUG 1
/* Includes */
#include <stdio.h>      /* Input/Output */
#include <iostream>
using namespace std;

#include <string>
#include <string.h>
#include <errno.h>      /* Errors */
#include <stdlib.h>     /* General Utilities */
#include <fstream>
#include <sstream>

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <sys/wait.h>   /* Wait for Process Termination */

#include <termios.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <signal.h>
#include <netdb.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/time.h>

#include "../WEBlib.d/WEBlib.h"
#include "webclient.h"

char htmlcode[2048];
bool trace_html= false;
int webclient_message_queue_id; 

// web server error
void wc_error(const char *msg, bool fatal=false) //  C library function void perror(const char *str) 
{
	fprintf(stderr, "[WS ERROR] ");
	fflush(stderr);
    perror(msg);
    if(fatal) exit(EXIT_FAILURE);
}

void web_client(char *servername, unsigned int serverport, char* serverpath)
{
	char CloudHost_name[128];
	char CloudHost_serverpath[128]; 
	char str[64];
	char str1[200];
	
	struct sockaddr_in servaddr;
	char **pptr;
	struct hostent *hptr;
	int r;
	unsigned int c_error= 0;
	unsigned int status_code;
	unsigned int c_total= 0;

	fprintf(stdout, "\nWEB client started to server %s:%d%s", servername, serverport, serverpath);
	fflush(stdout);
	snprintf(CloudHost_name, sizeof(CloudHost_name), "%s:%d", servername, serverport);
	snprintf(CloudHost_serverpath, sizeof(CloudHost_serverpath), serverpath); 
	
	if((hptr = gethostbyname(servername)) == NULL)
		printf("\nERROR gethostbyname\n");

	fprintf(stdout, "\nhostname: %s", hptr->h_name);
	if (hptr->h_addrtype == AF_INET && (pptr = hptr->h_addr_list) != NULL) 
	{
		fprintf(stdout, "\naddress: %s\n", inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
	} 
	else {
		fprintf(stderr, "\n[ERROR]Error call inet_ntop");
		fflush(stderr);
	}
	fflush(stdout);
	
	while(1)
	{
		// 1. wait for process message
		// a process-message means there is a network message to be sent to the cloud
		pmessbuf rbuf;
		msgrcv(webclient_message_queue_id, &rbuf, sizeof(pmessbuf)-sizeof(long), 1, 0);	
		
		// MESSAGE 
		// HTML payload - Action=xxxx&yyy
		htmlcode[0]='\0';
		// 1. copy message 
		snprintf((char *)htmlcode, sizeof(htmlcode),
			 "POST %s HTTP/1.1\r\n"
			 "Host: %s\r\n"
			 "Content-Type: application/x-www-form-urlencoded; charset=ISO-8859-1\r\n"
			 "Content-length: %d\r\n\r\n"
			 "%s", CloudHost_serverpath, CloudHost_name, strlen(rbuf.data), rbuf.data);
		
		if(trace_html)
		{		
			fprintf(stdout, "\n%s\n", htmlcode);
			fflush(stdout);
		}
		
		// 2. create socket to send IP cloud message
		// SOCKET
		int sockfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: IPv4 Internet protocols  / SOCK_STREAM: TCP sockect
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(serverport);
		inet_pton(AF_INET, str, &servaddr.sin_addr);	
		// Set time-out before connect
		struct timeval timeout;      
		timeout.tv_sec = 3; // 3 second timeout
		timeout.tv_usec = 0;
		if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
			wc_error("setsockopt failed\n");
		if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
			wc_error("setsockopt failed\n");
		r= connect(sockfd, (struct sockaddr *) & servaddr, sizeof(servaddr));
		// (END) SOCKET		

		c_total ++;		
		if(r != 0) 
		{
			// callback_timer
			if(rbuf.callback) rbuf.callback(-1, (char *) 0);
			fprintf(stdout, "-- errno= %d -- %s", errno, strerror(errno));
			fflush(stdout);
		}
		else 
		{
			ssize_t n;	
			// 3. SEND
			n= write(sockfd, htmlcode, strlen(htmlcode));
			// 4. RECEIVE response
			status_code= 0;
			n = read(sockfd, htmlcode, sizeof(htmlcode)-1);
			
			// Check if there are packets left (check it one time -  I may need to give some more tries)
			if(n>0)
			{
				htmlcode[n]='\0';
				// (DEBUG) Show buffer
				if(trace_html)
				{
					fprintf(stdout, "\n\n%s", htmlcode);
					fflush(stdout);
				}

				status_code= HTTPHeader_status_code(htmlcode);
				
				if(status_code==200 && !XML_CodeComplete(htmlcode) && (size_t)n<(sizeof(htmlcode)-1))
				{
					ssize_t m;
					sprintf(str1, "+");
					write(STDOUT_FILENO, str1, strlen(str1));
					m = read(sockfd, &htmlcode[n], sizeof(htmlcode)-n-1);
					if(m<0) {
						snprintf(str1, sizeof(str1), "-- errno (2)= %d --", errno);
						write(STDOUT_FILENO, str1, strlen(str1));
					}
					else n += m;
					htmlcode[n]='\0';
				}
			}
			else if(n==0) {
				htmlcode[0]='\0';
				fprintf(stdout,"\n--- ZERO BYTES read");
				fflush(stdout);
			}
			else // n<0 
			{
				htmlcode[0]='\0';
				fprintf(stdout, "-- errno= %d -- %s", errno, strerror(errno));
				fflush(stdout);
			}
			// (END) RECEIVE

			if(n>0 && status_code==200 && XML_CodeComplete(htmlcode))
			{
				char * xmlcode_ptr= XML_CodePtr(htmlcode);
				if(rbuf.callback) rbuf.callback(0, xmlcode_ptr);
				
			}
			// if(status_code==200 && XML_CodeComplete(htmlcode))
			else
			{
				if(status_code == 0) {
					sprintf(str1, " posible time-out");
					write(STDOUT_FILENO, str1, strlen(str1));
					c_error ++;
					double d1= c_error * 100;
					double d2= c_total;
					sprintf(str1, " %.3f%%", d1/d2); // percentage
					write(STDOUT_FILENO, str1, strlen(str1));
				}
				else if(status_code == 200) {
					sprintf(str1, "\n[ERROR] Incomplete message. IGNORED");
					write(STDOUT_FILENO, str1, strlen(str1));
				}
				else {
					char status[32];
					HTTPHeader_status_str(htmlcode, status);
					sprintf(str1, " %s", status);
					write(STDOUT_FILENO, str1, strlen(str1));
				}
				// callback with result= 1 error
				if(rbuf.callback) rbuf.callback(1, (char *) 0);
			}			
		}
		close(sockfd); 
	}
}

// Send a message to the cloud
// callback - function called when the cloud responds
int webClient_SendMessage(char *message, void (*callback) (int, char *))
{
	pmessbuf sbuf;
	sbuf.mtype = 1; // Any value > 0
	sbuf.callback= callback;
	snprintf(sbuf.data, SZ_PROCESS_MESSAGE, "%s", message);
    if(msgsnd(webclient_message_queue_id, &sbuf, sizeof(pmessbuf)-sizeof(long), IPC_NOWAIT) !=0)
	{
		wc_error("[webClient_SendMessage]");
		fflush(stdout);				return -1;
	}
	return 0;
}
void webClient_callback(int result, char *xmlcode)
{
	if(result==0 && xmlcode!=0)
	{
#ifdef WEBCLIENT_DEBUG		
		fprintf(stdout, "\nxmlcode %s\n", xmlcode);
		fflush(stdout);		
#endif		
	}
}

/* END OF FILE */