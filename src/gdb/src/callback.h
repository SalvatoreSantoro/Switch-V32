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
#define REGISTER_CALLBACK_SECTION(cb_fun, cb_arg, cb_type)                                 \
    static const Callback_Registration __cb_reg_##cb_fun                                   \
    __attribute__((used, section(".sad_callbacks"))) = {                                   \
        .type = cb_type,                                                                   \
        .cbk.fun = cb_fun,                                                                 \
        .cbk.arg = (void *) cb_arg                                                         \
    };


#define DEFINE_CALLBACK(name, user_dt, type)                                                                         \
    void name##_impl(__typeof__(user_dt) user_data, type##_t * handler_data);                                        \
    inline static void name(void *user_data, void *raw_input) {                                                      \
        name##_impl((__typeof__(user_dt)) user_data, (type##_t *)raw_input);                                         \
    };                                                                                                               \
    REGISTER_CALLBACK_SECTION(name, user_dt, type)                                                                   \
    void name##_impl(__typeof__(user_dt) user_data, type##_t * handler_data)

// clang-format on
typedef enum {
    NOACK_CBK = 0,
    REGS_CBK,
    REG_CBK,
    NUM_CBKS
} callbk_type;

typedef struct {
    int a;
} REGS_CBK_t;

extern REGS_CBK_t gas;

typedef void (*Callback_Fun)(void *, void *);

typedef struct {
    Callback_Fun fun;
    void *arg;
} Callback;

typedef struct {
    Callback cbk;
    callbk_type type;
} Callback_Registration;


// linker symbols (check sad.ld file)
extern const Callback_Registration __start_sad_callbacks;
extern const Callback_Registration __stop_sad_callbacks;

#endif
