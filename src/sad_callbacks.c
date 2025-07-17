#include "callback.h"
#include "cpu.h"
#include <stdio.h>

DEFINE_CALLBACK(regs_dump_callback, &core, REGS_CBK) {
    printf("THIS IS PC: %d", user_data->pc);
    handler_data->a=10;
    printf("THIS IS HANDLER DATA: %d", handler_data->a);
}
