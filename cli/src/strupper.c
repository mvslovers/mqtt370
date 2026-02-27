#include "mqttcli.h"

char *strupper(char *str)
{
	char 	*p;
	
	if (!str) goto quit;
	
	for(p=str; *p; p++) {
		*p = toupper(*p);
	}

quit:	
	return str;
}
