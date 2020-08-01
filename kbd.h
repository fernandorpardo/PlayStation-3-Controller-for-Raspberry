#ifndef KBD_HEADER_FILLE_H
#define KBD_HEADER_FILLE_H
#endif

int termios_init();
int termios_restore();
int kbhit(void);

unsigned int Parser(string , string []);

#define COMMNAD_QUEUE_LENGTH 5
class CLIbuff
{
	public:
		CLIbuff(char *command_buffer, string prompt);
		void Up();
		void Down();
		int Last();
		void Store();
	private:
		string command_queue[COMMNAD_QUEUE_LENGTH];
		int command_b_read;
		int command_b_write;
		int command_b_length;
		int command_b_last;
		char *command_buffer;
		string prompt;
};

/* END OF FILE */