#include "defs.h"
#include "sad_gdb_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern SAD_Stub server;

data_ret sad_pkt_data_init(void) {
    server.pkt_data.params = malloc(sizeof(Data_Param) * DEFAULT_PAR_SIZE);

    if (server.pkt_data.params == NULL) {
        return DATA_OOM;
    }

    server.pkt_data.params_sz = DEFAULT_PAR_SIZE;
    sad_pkt_data_reset();

    return DATA_OK;
}

void sad_pkt_data_deinit(void) {
    free(server.pkt_data.params);
}

static data_ret sad_pkt_data_param_expand(void) {
    Data_Param *tmp_par;

    tmp_par = realloc(server.pkt_data.params, sizeof(Data_Param) * server.pkt_data.params_sz * 2);
    if (tmp_par == NULL)
        return DATA_OOM;

    server.pkt_data.params = tmp_par;
    server.pkt_data.params_sz *= 2;

    return DATA_OK;
}

void sad_pkt_data_reset(void) {
    if (server.pkt_data.params_filled > MAX_DATA_PARAMS_SIZE) {
        free(server.pkt_data.params);
        server.pkt_data.params = malloc(sizeof(Data_Param) * DEFAULT_PAR_SIZE);
		server.pkt_data.params_sz = DEFAULT_PAR_SIZE;
    }
    server.pkt_data.command = NULL;
    server.pkt_data.params_filled = 0;
}

data_ret sad_pkt_data_append_par(char *param1, char *param2) {
    if ((server.pkt_data.params_filled + 1) > server.pkt_data.params_sz) {
        if (sad_pkt_data_param_expand() == DATA_OOM)
            return DATA_OOM;
    }
    server.pkt_data.params[server.pkt_data.params_filled].param1 = param1;
    server.pkt_data.params[server.pkt_data.params_filled].param2 = param2;

    server.pkt_data.params_filled += 1;

    return DATA_OK;
}

Data_Param *sad_pkt_data_find(PKT_Data *pkt_data, const char *param1) {
    for (size_t i = 0; i < pkt_data->params_filled; i++) {
        if (strcmp(pkt_data->params[i].param1, param1) == 0)
            return &pkt_data->params[i];
    }
    return NULL;
}

void sad_pkt_data_print(void) {
    printf("PARAM SIZE: %ld\n", server.pkt_data.params_sz);
    printf("PARAM FILL: %ld\n", server.pkt_data.params_filled);
    printf("COMMAND %s\n", server.pkt_data.command);
    for (size_t i = 0; i < server.pkt_data.params_filled; i++) {
        printf("PARAM1: %s\n", server.pkt_data.params[i].param1);
        if (server.pkt_data.params[i].param2 != NULL)
            printf("PARAM2: %s\n", server.pkt_data.params[i].param2);
    }
}
