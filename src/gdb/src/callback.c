#include "callback.h"
#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>

// linker symbols (check sad.ld file)
extern const Callback_Registration __start_sad_callbacks;
extern const Callback_Registration __stop_sad_callbacks;

void gdb_callbacks_deinit(Callback *cbks) {
    for (int i = 0; i < NUM_CBKS; i++) {
        if (cbks[i].handler_data != NULL)
            free(cbks[i].handler_data);
    }
}

cbks_ret gdb_callbacks_init(Callback *cbks, PKT_Buffer *output) {
    // Assert all callbacks are defined
    SAD_ASSERT(((&__start_sad_callbacks + NUM_CBKS) == &__stop_sad_callbacks), "Some callbacks aren't defined");

    for (const Callback_Registration *reg = &__start_sad_callbacks; reg < &__stop_sad_callbacks; ++reg) {
        // save user registered callbacks
        cbks[reg->type].fun = reg->cbk.fun;
        cbks[reg->type].user_data = reg->cbk.user_data;
        // CURRENTLY NOT THREAD SAFE
        // allocate all the handlers, initializing them all with
        // the output buffer (so that user callbacks can directly write on output buffer)
        // we're assuming that everything in the stub works as single thread
        switch (reg->type) {
#define X(name, ...)                                                                                                   \
    case name:                                                                                                         \
        cbks[reg->type].handler_data = malloc(sizeof(name##_t));                                                       \
        if (cbks[reg->type].handler_data != NULL) {                                                                    \
            name##_t *handler = cbks[reg->type].handler_data;                                                          \
            handler->output = output;                                                                                  \
        }                                                                                                              \
        break;
            CALLBACKS_GENERATOR
#undef X
        default:
            break;
        }
        // allocation failed)
        if (cbks[reg->type].handler_data == NULL) {
            gdb_callbacks_deinit(cbks);
            return CBKS_OOM;
        }
    };
    return CBKS_OK;
}

void gdb_callbacks_dispatch(Callback *cbks, callbk_type type) {
    Callback cbk = cbks[type];
    cbk.fun(cbk.user_data, cbk.handler_data);
}
