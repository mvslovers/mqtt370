#include "mqtc370.h"

void mqtc_set_context(MQTC *mqtc, void *ctx)
{
    mqtc->ctx = ctx;
}
