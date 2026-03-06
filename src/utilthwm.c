#include "mqttutil.h"

unsigned util_task_hwm(const CTHDTASK *task, unsigned *stk, unsigned *bytes)
{
    unsigned    hwm     = 0;
    
    if (!task) goto quit;

    if (stk)    *stk    = (unsigned) task->stack;
    if (bytes)  *bytes  = (unsigned) task->stacksize;

    hwm = util_stack_hwm((void*)task->stack, task->stacksize);

quit:
    return hwm;
}
