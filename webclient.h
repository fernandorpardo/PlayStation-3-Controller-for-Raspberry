#ifndef WEBCLIENT_HEADER_FILLE_H
#define WEBCLIENT_HEADER_FILLE_H
#endif

void web_client(char*, unsigned int, char*);
extern int webclient_message_queue_id;

// http://www.dsm.fordham.edu/cgi-bin/man-cgi.pl?topic=msgrcv&ampsect=2
#define SZ_PROCESS_MESSAGE 512
struct pmessbuf {
	long mtype; // message type, must be > 0		
	// message data
	void (*callback) (int, char *);
	char data[SZ_PROCESS_MESSAGE];
};

int webClient_SendMessage(char *str, void (*callback) (int, char *)= NULL);
void webClient_callback(int result, char *xmlcode);

/* END OF FILE */