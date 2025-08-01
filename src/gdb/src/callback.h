#ifndef _GDB_CALLBACK_H
#define _GDB_CALLBACK_H

// every callback type should be associated (if needed) to a type
// called in the same way but with _t like "REGS_CBK_t"
// defined as a global "singleton" struct used inside the callback
// as "handler_data" and passed from the builder when calling the callback
// inside the associated builder function.
// In practice the "type" used in "DECLARE_CALLBACK"
// is both used to define the type of the callback and the name of the handler to use.
// every callback also has an "user_data" variable,
// during registration of the callback the actual data should be passed,
// the data passed must have the same type defined during callback definition.
// user data MUST be a global variable

// clang-format off
#include "buffer.h"
#include <stdint.h>
#include "defs.h"
#define REGISTER_CALLBACK_SECTION(cb_fun, cb_arg, cb_type)                                 \
    static const Callback_Registration __cb_reg_##cb_fun                                   \
    __attribute__((used, section(".sad_callbacks"))) = {                                   \
        .type = cb_type,                                                                   \
        .cbk.fun = cb_fun,                                                                 \
        .cbk.user_data = (void *) cb_arg                                                         \
    };

#define DEFINE_CALLBACK(name, user_dt, type)                                                                         \
    void name##_impl(__typeof__(user_dt) user_data, type##_t * handler_data);                                        \
    inline static void name(void *user_data, void *raw_input) {                                                      \
        name##_impl((__typeof__(user_dt)) user_data, (type##_t *)raw_input);                                         \
    };                                                                                                               \
    REGISTER_CALLBACK_SECTION(name, user_dt, type)                                                                   \
    void name##_impl(__typeof__(user_dt) user_data, type##_t * handler_data)

// clang-format on

// HANDLERS
#define HANDL_FIELD(type, name) type name;
#define HANDL_GENERATOR(name, ...)                                                                                     \
    typedef struct {                                                                                                   \
        __VA_ARGS__                                                                                                    \
        PKT_Buffer *output;                                                                                            \
    } name##_t;

// SAD_EXTEND START "Add new CALLBACK here"
#define CALLBACKS_GENERATOR                                                                                            \
    X(READ_REGS_CBK)                                                                                                   \
    X(WRITE_REGS_CBK, HANDL_FIELD(uint32_t, regs[NUM_REGS]))                                                           \
    X(READ_MEM_CBK, HANDL_FIELD(uint64_t, addr) HANDL_FIELD(size_t, length))                                           \
    X(WRITE_MEM_CBK, HANDL_FIELD(uint64_t, addr) HANDL_FIELD(size_t, length) HANDL_FIELD(unsigned char *, data))       \
// SAD_EXTEND END

// generate handlers
#define X(name, ...) HANDL_GENERATOR(name, __VA_ARGS__)
CALLBACKS_GENERATOR
#undef X

typedef void (*Callback_Fun)(void *, void *);

typedef enum {
#define X(name, ...) name,
    CALLBACKS_GENERATOR
#undef X
        NUM_CBKS
} callbk_type;

typedef struct {
    Callback_Fun fun;
    void *user_data;
    void *handler_data;
} Callback;

typedef struct {
    Callback cbk;
    callbk_type type;
} Callback_Registration;

typedef enum {
    CBKS_OOM,
    CBKS_OK
} cbks_ret;

// output buffer is used from all the handlers
cbks_ret gdb_callbacks_init(Callback *cbks, PKT_Buffer *output);

void gdb_callbacks_deinit(Callback *cbks);

void gdb_callbacks_dispatch(Callback *cbks, callbk_type type);

#endif
