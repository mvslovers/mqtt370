#include "mqtb370.h"

int mqtb_topic_match(const char *topic, const char *sub)
{
    int     matched = 0;    /* false, no match */
    int     pos     = 0;

    if (!topic) goto quit;  /* no match */
    if (!sub) goto quit;    /* no match */
    
    /* special case for broker topics */
    if (sub[0] == '$' && topic[0] != '$') goto quit;    /* no match */
    if (sub[0] != '$' && topic[0] == '$') goto quit;    /* no match */

    while (*sub) {
        if (*sub == *topic) {
			if (!topic[1]) {
                /* end of topic string */
                if (sub[1] == '/' && sub[2] == '#' && sub[3] == 0) {
					matched = 1;
                    goto quit;
				}
			}

            /* look at next character */
            pos++;
            sub++;
            topic++;

            if (!*sub && !*topic) {
                /* end of sub and topic strings */
                matched = 1;
                goto quit;
            }

            if (!*topic && *sub == '+' && sub[1] == 0) {
                /* end of topic string and sub is "+" */
                if (pos > 0 && sub[-1] != '/') {
                    goto quit;  /* no match */
                }

				matched = 1;
                goto quit;
			}
            continue;
        }
        
        /* *sub != *topic */

        /* check for match any single topic name element */
        if (*sub == '+') {
            /* Check for bad "+foo" or "a/+foo" subscription */
            if (pos > 0 && sub[-1] != '/') goto quit;  /* no match */

            /* Check for bad "foo+" or "foo+/a" subscription */
            if (sub[1] != 0 && sub[1] != '/') goto quit; /* no match */

			pos++;
			sub++;

            while (*topic != 0 && *topic != '/') {
#if 0 /* we don't allow topics with these characters, so no need to check */
                /* check for invalid topic character */
                if (*topic == '+' || *topic == '#') goto quit; /* no match */
#endif
                topic++;
                if (*topic == 0 && *sub == 0) {
                    matched = 1;
                    goto quit;
                }
            }
            continue;
        }
        
        
        if (*sub == '#') {
            /* Check for bad "foo#" subscription */
            if(pos > 0 && sub[-1] != '/') goto quit;    /* no match *.

            /* Check for # not the final character of the sub, e.g. "#foo" */
            if (sub[1] != 0) goto quit; /* no match */

            // while (*topic) {
            if (*topic) {
#if 0 /* we don't allow topics with these characters, so no need to check */
                if( *topic == '+' || *topic == '#') goto quit;  /* no match */
#endif
				// topic++;
                matched = 1;
                goto quit;
            }
            continue;
		}

        /* Check for e.g. foo/bar matching foo/+/# */
        if (!*topic && pos > 0 && sub[-1] == '+' && sub[0] == '/' && sub[1] == '#') {
            matched = 1;
            goto quit;
        }

#if 0 /* we don't allow subscriptions with invalid patterns */
        /* There is no match at this point, but is the sub invalid? */
        while (*sub) {
            if(*sub == '#' && sub[1] != 0) goto quit; /* no match */
            pos++;
            sub++;
        }
#endif

        /* Valid input, but no match */
        goto quit;
    }

quit:
    return matched;
}
