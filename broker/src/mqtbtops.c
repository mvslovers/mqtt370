#include "mqtb370.h"

int mqtb_topic_sub_valid(const char *sub)
{
    int         is_valid    = 0;    /* not valid */
    int         prev        = 0;
    int         len;

    if (!sub)   goto quit;

    for(len=0; sub[len]; len++) {
        int c = (int)sub[len];
        int next = (int)sub[len+1];
        
        if (c == '+') {
            if (prev && prev != '/') goto quit;     /* invalid */
            if (next && next != '/') goto quit;     /* invalid */
        }
        else if(c == '#') {
            if (prev && prev != '/') goto quit;     /* invalid */
            if (next) goto quit;                    /* invalid */
		}
        prev = c;
    }
    
    if (len > MAX_TOPIC) goto quit;
    
    is_valid = len;

quit:
    return is_valid;
}
