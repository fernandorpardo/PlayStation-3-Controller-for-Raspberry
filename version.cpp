// 
// 01.00.00 - 	first release


#define VERSION_MAIN  "01" 		// major version. N-1 is not compatible with N
#define VERSION_MAJOR "00" 		// release
#define VERSION_MINOR "00" 		// changes

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
using namespace std;
#include <string.h>

const char *months_list[] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};

int monthsnumber(const char *m)
{
	int i;
	int n= sizeof(months_list)/sizeof(months_list[0]);
	for(i=0; i<n && !(months_list[i][0]==tolower(m[0]) && months_list[i][1]==tolower(m[1]) && months_list[i][2]==tolower(m[2])) ; i++);
	if(i<n) return i+1;
	return 0;
}
char *version(char *str, size_t max_sz)
{
	char day[16];
	char month[16];
	char year[16];
	char hour[16];
	char min[16];
	char sec[16];
	char time[32];
	// date
	sscanf(__DATE__, "%s %s %s", month, day, year);
	// time
	snprintf(time, sizeof(time),"%s",__TIME__);
	for(int i=0; i<(int)strlen(time); i++) if(time[i]==(char)':') time[i]=' ';
	sscanf(time, "%s %s %s", hour, min, sec);
	// generate version string
	snprintf(str, max_sz, "%s.%s.%s-%04d.%02d.%02d-%02d%02d%02d", VERSION_MAIN, VERSION_MAJOR, VERSION_MINOR, atoi(year), monthsnumber(month), atoi(day), atoi(hour), atoi(min), atoi(sec));
	return str;
}

/* END OF FILE */