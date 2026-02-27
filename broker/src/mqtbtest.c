#include "mqtb370.h"

static const char *subs[] = {
    "#",
    "town/#",
    "town/house/#",
    "+/house/kitchen",
    "town/+/kitchen",
    "town/house/+",
    "+/+/kitchen",
    "town/+/+",
    "+/+/+",
    "town/house/kitchen",

    "bad#",
    "town/bad/#",
    "town/bad/#",
    "+/bad/kitchen",
    "town/+/bad",
    "town/bad/+",
    "+/+/bad",
    "bad/+/+",
    "+/+",
    "town/house/bad",
    NULL
};

int main(int argc, char **argv)
{
    int     rc  = 0;
    int     i;
    const char *topic = "town/house/kitchen";
    
    for(i=0; subs[i]; i++) {
        rc = mqtb_topic_match(topic, subs[i]);
        wtof("%d = mqtb_topic_match(\"%s\", \"%s\")", rc, topic, subs[i]);
    }

    rc = 0;
    return rc;
}
