#include "mqttutil.h"

void 
util_free_char(char **pstr)
{
	char 	*str;
	
	if (!pstr) goto quit;
	str = *pstr;
	if (!str) goto quit;
	
	free(str);
	*pstr = NULL;

quit:
	return;
}
