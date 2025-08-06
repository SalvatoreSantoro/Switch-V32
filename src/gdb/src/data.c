#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PKT_Data *sad_pkt_data_create(size_t params_sz) {
    PKT_Data *pkt_data;

    pkt_data = malloc(sizeof(PKT_Data));

    if (pkt_data == NULL)
        return NULL;

    pkt_data->params = malloc(sizeof(Data_Param) * params_sz);
    if (pkt_data->params == NULL) {
        free(pkt_data);
        return NULL;
    }

    pkt_data->params_sz = params_sz;
    sad_pkt_data_reset(pkt_data);

    return pkt_data;
}

void sad_pkt_data_destroy(PKT_Data *pkt_data) {
    if (pkt_data->params != NULL)
        free(pkt_data->params);
    if (pkt_data != NULL)
        free(pkt_data);
    pkt_data = NULL;
}

static data_ret sad_pkt_data_param_expand(PKT_Data *pkt_data) {
    Data_Param *tmp_par;

    tmp_par = realloc(pkt_data->params, sizeof(Data_Param) * pkt_data->params_sz * 2);
    if (tmp_par == NULL)
        return DATA_OOM;

    pkt_data->params = tmp_par;
    pkt_data->params_sz *= 2;

    return DATA_OK;
}

void sad_pkt_data_reset(PKT_Data *pkt_data) {
    pkt_data->command = NULL;
    pkt_data->params_filled = 0;
}

data_ret sad_pkt_data_append_par(PKT_Data *pkt_data, char *param1, char *param2) {
    if ((pkt_data->params_filled + 1) > pkt_data->params_sz) {
        if (sad_pkt_data_param_expand(pkt_data) == DATA_OOM)
            return DATA_OOM;
    }
    pkt_data->params[pkt_data->params_filled].param1 = param1;
    pkt_data->params[pkt_data->params_filled].param2 = param2;

    pkt_data->params_filled += 1;

    return DATA_OK;
}

Data_Param *sad_pkt_data_find(PKT_Data *pkt_data, const char *param1) {
    for (size_t i = 0; i < pkt_data->params_filled; i++) {
        if (strcmp(pkt_data->params[i].param1, param1) == 0)
            return &pkt_data->params[i];
    }
    return NULL;
}

void sad_pkt_data_print(PKT_Data *pkt_data) {
    printf("PARAM SIZE: %ld\n", pkt_data->params_sz);
    printf("PARAM FILL: %ld\n", pkt_data->params_filled);
    printf("COMMAND %s\n", pkt_data->command);
    for (size_t i = 0; i < pkt_data->params_filled; i++) {
        printf("PARAM1: %s\n", pkt_data->params[i].param1);
        if (pkt_data->params[i].param2 != NULL)
            printf("PARAM2: %s\n", pkt_data->params[i].param2);
    }
}
