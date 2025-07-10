#ifndef _GDB_PKT_DATA_H
#define _GDB_PKT_DATA_H

#include <stddef.h>

typedef struct {
    char *param1;
    char *param2; // Empty string for key-only/value params
} Data_Param;

typedef struct {
    size_t params_sz;
    size_t params_filled;
    char *command;
    Data_Param *params;
} PKT_Data;

typedef enum {
    DATA_OK,
    DATA_OOM,
} data_ret;

PKT_Data *gdb_pkt_data_create(size_t params_sz);

void gdb_pkt_data_destroy(PKT_Data *pkt_data);

void gdb_pkt_data_reset(PKT_Data *pkt_data);

data_ret gdb_pkt_data_append_par(PKT_Data *pkt_data, char *param1, char *param2);

void gdb_pkt_data_print(PKT_Data *pkt_data);

#endif
