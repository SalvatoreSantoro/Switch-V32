#ifndef _GDB_CALLBACK_H
#define _GDB_CALLBACK_H

// every callback type should be associated (if needed) to a type
// called in the same way but with _t like "REGS_CBK_t"
// defined as a global "singleton" struct used inside the callback
// as "handler_data" and passed from the builder when calling the callback
// inside the associated builder function.
// In practice the "type" used in "DECLARE_CALLBACK" and "DEFINE_CALLBACK"
// is both used to define the type of the callback and the name of the handler to use.
// every callback also has an "user_data" variable,
// during the definition/declaration the correct type should be passed
// during registration of the callback the actual data should be passed,
// the data passed must have the same type defined during callback declaration/definition.
// assume that the callback could be called in every
// moment, so please make sure that "user_data" is always valid
// so no stack allocated except for main function
// never free if on heap
// could became a dangling pointer otherwise


#define DECLARE_CALLBACK(name, user_dt, type)                                                                          \
    void name##_impl(user_dt *user_data, type##_t *handler_data);                                                      \
    inline static void name(void *user_data, void *raw_input) {                                                        \
        return name##_impl((user_dt *) user_data, (type##_t *) raw_input);                                             \
    }

#define DEFINE_CALLBACK(name, user_dt, type) void name##_impl(user_dt *user_data, type##_t *handler_data)

#define EMIT_CALLBACK(name, user_dt, type) (DECLARE_CALLBACK(name, user_dt, type) DEFINE_CALLBACK(name, user_dt, type))


typedef enum {
    NOACK_CBK = 0,
    REGS_CBK,
    REG_CBK,
    NUM_CBKS
} callbk_type;

typedef void (*Callback_Fun)(void *);

typedef struct {
    Callback_Fun fun;
    void *arg;
} Callback;

#endif
