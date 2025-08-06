#ifndef CALLBACK_H
#define CALLBACK_H

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
#define REGISTER_CALLBACK_SECTION(cb_fun, cb_type)                                         \
    static const Callback_Registration __cb_reg_##cb_fun                                   \
    __attribute__((used, section(".sad_callbacks"))) = {                                   \
        .type = cb_type,                                                                   \
        .cbk.fun = cb_fun,                                                                 \
    };

#define DEFINE_CALLBACK(name, type)                                                        \
    void name##_impl(type##_t * handler_data);                                             \
    inline static void name(void *raw_input) {                                             \
        name##_impl((type##_t *)raw_input);                                                \
    };                                                                                     \
    REGISTER_CALLBACK_SECTION(name, type)                                                  \
    void name##_impl(type##_t * handler_data)

// clang-format on

// HANDLERS
#define HANDL_FIELD(type, name) type name;
#define HANDL_GENERATOR(name, ...)                                                                                     \
    typedef struct {                                                                                                   \
        Buffer *output;                                                                                                \
        __VA_ARGS__                                                                                                    \
    } name##_t;

// SAD_EXTEND START "Add new CALLBACK here"
#define CALLBACKS_GENERATOR                                                                                            \
    X(READ_REGS_CBK)                                                                                                   \
    X(WRITE_REGS_CBK, HANDL_FIELD(uint32_t, regs[NUM_REGS]))                                                           \
    X(READ_MEM_CBK, HANDL_FIELD(uint64_t, addr) HANDL_FIELD(size_t, length))                                           \
    X(WRITE_MEM_CBK, HANDL_FIELD(uint64_t, addr) HANDL_FIELD(size_t, length) HANDL_FIELD(byte *, data))                \
// SAD_EXTEND END

// generate handlers
#define X(name, ...) HANDL_GENERATOR(name, __VA_ARGS__)
CALLBACKS_GENERATOR
#undef X

typedef void (*Callback_Fun)(void *);

typedef enum {
#define X(name, ...) name,
    CALLBACKS_GENERATOR
#undef X
        NUM_CBKS
} callbk_type;

typedef struct {
    Callback_Fun fun;
    void *handler_data;
} Callback;

typedef struct {
    Callback cbk;
    callbk_type type;
    // alignment reasons
    // if the SAD ASSERT on the callbacks triggers
    // maybe there is something wrong with paddings of these sctruct
    byte padding[8];
} Callback_Registration;

typedef enum {
    CBKS_OOM,
    CBKS_OK
} cbks_ret;

// output buffer is used from all the handlers
cbks_ret sad_callbacks_init(Callback *cbks);

void sad_callbacks_deinit(Callback *cbks);

void sad_callbacks_dispatch(Callback *cbks, callbk_type type);

void sad_callbacks_reset(Callback *cbks);
#endif
