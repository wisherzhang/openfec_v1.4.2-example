#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <redun.h>
#include <redun_config.h>
#define VERBOSITY	1		/* Define the verbosity level:
                             *	0 : no trace
                             *	1 : main traces
                             *	2 : full traces with packet dumps */

/********************* OpenFec_Audio_Decode ******************/

typedef struct RedunManageDecPrivStu {
    char k;                             //源数据包个数
    char n;                             //源数据与冗余包个数总和
    int fillLength;                     //数据包固定长度
    int headFillLength;
    of_codec_id_t codecId;              //解码器类型，OpenFec支持多种解码类型

    of_session_t *ses;                  //OpenFec句柄
    of_parameters_t	*params;            //参数
    void *my_params;

    int recvdNum;                       //当前分组接收到的数据包个数，必需等于k
    void *src_symbols_tab[REDUN_MAX_INPUT_NUM];
    void *src_head_symbols_tab[REDUN_MAX_INPUT_NUM];
    void *recvd_symbols_tab[REDUN_MAX_INPUT_NUM];
    void *recvd_head_symbols_tab[REDUN_MAX_INPUT_NUM];

} RedunManageDecPrivStu;


int redunOpenFecDecOpen(RedunContext *oObj, void *config) {
    int status                      = -1;
    RedunManageDecPrivStu *pObj     = (RedunManageDecPrivStu *)oObj->priv;
    RedunManageDecParmas *params    = (RedunManageDecParmas *)config;

    memset(pObj, 0x0, sizeof(*pObj));

    pObj->recvdNum      = 0;
    pObj->k             = params->k;
    pObj->n             = params->n;
    pObj->fillLength    = params->fillLength;
    pObj->headFillLength= params->headFillLength;
    pObj->codecId       = params->codecId;

    switch (pObj->codecId) {
        case OF_CODEC_REED_SOLOMON_GF_2_M_STABLE:
            {
                pObj->my_params     = (of_rs_2_m_parameters_t *) malloc (sizeof(of_rs_2_m_parameters_t ));
                if (NULL != pObj->my_params) {
                    ((of_rs_2_m_parameters_t *)(pObj->my_params))->m = pObj->n;
                }
                break;
            }
        case OF_CODEC_REED_SOLOMON_GF_2_8_STABLE:
            {
                pObj->my_params     = (of_rs_parameters_t *) malloc (sizeof(of_rs_parameters_t));
                break;
            }
        default:
            printf ("codecId :%d invalid\n", pObj->codecId);
            break;
    }

    if (NULL == pObj->my_params) {
        printf ("malloc codeId:%d pObj->my_params failure\n", pObj->codecId);
        goto ERR0;
    }

    memset(pObj->my_params, 0x0, sizeof(*pObj->my_params));

    pObj->params                            = (of_parameters_t *)pObj->my_params;
    pObj->params->nb_source_symbols         = pObj->k;
    pObj->params->nb_repair_symbols         = pObj->n - pObj->k;
    pObj->params->encoding_symbol_length    = pObj->fillLength;
    pObj->params->encoding_head_length      = pObj->headFillLength;

    status = of_create_codec_instance(&pObj->ses, pObj->codecId, OF_DECODER, VERBOSITY);
    if (OF_STATUS_OK != status) {
        printf ("of_create_codec_instance failure\n");
        goto ERR1;
    }

    status = of_set_fec_parameters(pObj->ses, pObj->params);
    if (OF_STATUS_OK != status) {
        printf ("of_set_fec_parameters failure\n");
        goto ERR2;
    }

    //设置内部申请内存接口
    if (params->requestsrcnew || params->requestrepairnew) {
        of_set_callback_functions_new(pObj->ses, 
                params->requestsrcnew, params->requestrepairnew, params->priv);
    }
    else if (params->requestsrc || params->requestrepair) {
        of_set_callback_functions(pObj->ses, 
                params->requestsrc, params->requestrepair, params->priv);
    }

    memset(pObj->src_symbols_tab, 0x0, sizeof(pObj->src_symbols_tab));
    memset(pObj->recvd_symbols_tab, 0x0, sizeof(pObj->recvd_symbols_tab));
    memset(pObj->recvd_head_symbols_tab, 0x0, sizeof(pObj->recvd_head_symbols_tab));

    return 0;
ERR2:
    if (pObj->ses) of_release_codec_instance(pObj->ses);
ERR1:
    if (pObj->my_params) free(pObj->my_params);
ERR0:
    return -1;
}

int redunOpenFecDecClose(RedunContext *oObj) {
    RedunManageDecPrivStu *pObj     = (RedunManageDecPrivStu *)oObj->priv;

    if (pObj->my_params) free(pObj->my_params);
    if (pObj->ses) of_release_codec_instance(pObj->ses);

    return 0;
}

int redunOpenFecDecReset(RedunContext *oObj) {
    int index                       = 0;
    RedunManageDecPrivStu *pObj     = (RedunManageDecPrivStu *)oObj->priv;

    pObj->recvdNum = 0;

    for (index = 0; index < pObj->n; index++) {
        pObj->recvd_symbols_tab[index]  = NULL;
        pObj->recvd_head_symbols_tab[index]  = NULL;
        pObj->src_symbols_tab[index]    = NULL;
    }

    if (pObj->ses) of_reset_decoding_statu(pObj->ses);

    return 0;
}

int redunOpenFecDecAdd(RedunContext *oObj, RedunBufList *pDataList) {
    int index                       = 0;
    RedunBuf *pBuf                  = NULL;
    RedunManageDecPrivStu *pObj     = (RedunManageDecPrivStu *)oObj->priv;

    for (index = 0; index < pDataList->numBufs; index++) {
        pBuf = pDataList->redunBuf[index];
        if (pBuf) {
            //检查openfec序列的有效性
            if (pBuf->sei >= pObj->n) {
                printf ("input data openfec sei:%d invalid, "
                        "range to [0 - %d]\n", pBuf->sei, pObj->n);
                goto ERR0;
            }

            if (pBuf->sei < pObj->k) {
                if (NULL == pBuf->data) {
                    printf ("input data openfec data:0x%p invalid\n", pBuf->data);
                    goto ERR0;
                }
            }
            else {
                if (NULL == pBuf->dataRepair) {
                    printf ("input data openfec dataRepair:0x%p invalid\n", pBuf->dataRepair);
                    goto ERR0;
                }
            }

            pObj->recvdNum++;
            pObj->recvd_symbols_tab[pBuf->sei]      = pBuf->sei < pObj->k ? pBuf->data : pBuf->dataRepair;
            pObj->recvd_head_symbols_tab[pBuf->sei] = pBuf->sei < pObj->k ? pBuf->dataHead : pBuf->dataHeadRepair;
            printf ("%s %d redun sei:%d %s, rcvd:%d recvdSybTab:%p recvdHeadSybTab:%p \n",
                    __func__, __LINE__, 
                    pBuf->sei, pBuf->sei >= pObj->k ? "repair" : "src", 
                    pObj->recvdNum, pObj->recvd_symbols_tab[pBuf->sei], 
                    pObj->recvd_head_symbols_tab[pBuf->sei]);
        }
    }

    return 0;
ERR0:
    return -1;
}

/*
 * int redunOpenFecDecAddSrc(RedunContext *oObj, RedunBuf *data) {
 *     RedunBuf *pBuf                  = data;
 *     RedunManageDecPrivStu *pObj     = (RedunManageDecPrivStu *)oObj->priv;
 * 
 *     //检查openfec序列的有效性
 *     if (pBuf->sei >= pObj->k) {
 *         printf ("input data openfec sei:%d invalid, range to [0 - %d]\n", pObj->k);
 *         goto ERR0;
 *     }
 * 
 *     if (NULL == pBuf->data) {
 *         printf ("input data openfec data:0x%p invalid\n", pBuf->data);
 *         goto ERR0;
 *     }
 * 
 *     pObj->recvdNum++;
 *     pObj->recvd_symbols_tab[pBuf->sei] = pBuf->data;
 * 
 *     return 0;
 * ERR0:
 *     return -1;
 * }
 * 
 * int redunOpenFecDecAddRepair(RedunContext *oObj, RedunBuf *data) {
 *     RedunBuf *pBuf                  = data;
 *     RedunManageDecPrivStu *pObj     = (RedunManageDecPrivStu *)oObj->priv;
 * 
 *     //检查openfec序列的有效性
 *     if (pBuf->sei < pObj->k
 *             || pBuf->sei >= pObj->n) {
 *         printf ("input data openfec sei:%d invalid, range to [%d - %d]\n", pObj->k, pObj->n);
 *         goto ERR0;
 *     }
 * 
 *     if (NULL == pBuf->data) {
 *         printf ("input data openfec data:0x%p invalid\n", pBuf->data);
 *         goto ERR0;
 *     }
 * 
 *     pObj->recvdNum++;
 *     pObj->recvd_symbols_tab[pBuf->sei] = pBuf->data;
 * 
 *     return 0;
 * ERR0:
 *     return -1;
 * }
 */

int redunOpenFecDecProcess(RedunContext *oObj) {
    int index                       = 0;
    int status                      = -1;
    RedunManageDecPrivStu *pObj     = (RedunManageDecPrivStu *)oObj->priv;

    //冗余器接收到的源数据及冗余数据必需大于等于K
    if (pObj->recvdNum >= pObj->k) {
        for (index = 0; index < pObj->n; index++) {
            if (pObj->recvd_symbols_tab[index]) {
                //将接收到的源数据及冗余数据添加到冗余器中
                status = of_decode_with_new_symbol(pObj->ses, 
                        pObj->recvd_symbols_tab[index], 
                        pObj->recvd_head_symbols_tab[index], 
                        index);
                if (OF_STATUS_OK != status) {
                    printf ("of_decode_with_new_symbol sei:%d failure\n", index);
                }
            }
        }

        //调用冗余解码完成接口，检测是否完成解码
        status = of_is_decoding_complete(pObj->ses);
        if (true != status) {
            printf ("of_is_decoding_complete failure\n");
            goto ERR0;
        }

        //从冗余器中获取恢复后的数据指针
        status = of_get_source_symbols_tab_new(pObj->ses, 
                pObj->src_symbols_tab, pObj->src_head_symbols_tab);
        if (OF_STATUS_OK != status) {
            printf ("of_get_source_symbols_tab_new failure\n");
            goto ERR0;
        }
    }
    else {
        printf ("input recvdNum:%d must >= pObj->k:%d\n", pObj->recvdNum, pObj->k);
        goto ERR0;
    }

    return 0;
ERR0:
    return -1;
}

static int redunOpenFecDecCommand(RedunContext *oObj, unsigned cmd, void *value) {
    int index                       = 0;
    int status                      = -1;
    RedunManageDecPrivStu *pObj     = (RedunManageDecPrivStu *)oObj->priv;

    switch (cmd) {
        case REDUN_COMMAND_ISSAVE_FILE:
            {
                break;
            }
        default:
            break;
    }

    return 0;
}

RedunManageStu redunOpenFecAudDec = {
    .name           = "OpenFec_Audio_Decode",
    .privSize       = sizeof(RedunManageDecPrivStu),
    .oopen          = redunOpenFecDecOpen,
    .cclose         = redunOpenFecDecClose,
    .reset          = redunOpenFecDecReset,
    .add            = redunOpenFecDecAdd,
    .process        = redunOpenFecDecProcess,
    .command        = redunOpenFecDecCommand,
};


/********************* OpenFec_Audio_Encode ******************/

typedef struct RedunManageEncPrivStu {
    char k;                                         //源数据包个数
    char n;                                         //源数据与冗余包个数总和
    int fillLength;                                 //数据包固定长度
    int headFillLength;
    of_codec_id_t codecId;                          //解码器类型，OpenFec支持多种解码类型

    of_session_t *ses;                              //OpenFec句柄
    of_parameters_t	*params;                        //参数
    void *my_params;

    int recvdNum;                                   //当前分组接收到的数据包个数，必需等于k

    void *enc_symbols_tab[REDUN_MAX_INPUT_NUM];
    void *enc_head_symbols_tab[REDUN_MAX_INPUT_NUM];
} RedunManageEncPrivStu;

int redunOpenFecEncOpen(RedunContext *oObj, void *config) {
    int status                      = -1;
    RedunManageEncPrivStu *pObj     = (RedunManageEncPrivStu *)oObj->priv;
    RedunManageEncParmas *params    = (RedunManageEncParmas *)config;

    memset(pObj, 0x0, sizeof(*pObj));

    pObj->recvdNum      = 0;
    pObj->k             = params->k;
    pObj->n             = params->n;
    pObj->fillLength    = params->fillLength;
    pObj->headFillLength= params->headFillLength;
    pObj->codecId       = params->codecId;

    switch (pObj->codecId) {
        case OF_CODEC_REED_SOLOMON_GF_2_M_STABLE:
            {
                pObj->my_params     = (of_rs_2_m_parameters_t *) malloc (sizeof(of_rs_2_m_parameters_t ));
                if (NULL != pObj->my_params) {
                    ((of_rs_2_m_parameters_t *)(pObj->my_params))->m = pObj->n;
                }
                break;
            }
        case OF_CODEC_REED_SOLOMON_GF_2_8_STABLE:
            {
                pObj->my_params     = (of_rs_parameters_t *) malloc (sizeof(of_rs_parameters_t));
                break;
            }
        default:
            printf ("codecId :%d invalid\n", pObj->codecId);
            break;
    }

    if (NULL == pObj->my_params) {
        printf ("malloc codeId:%d pObj->my_params failure\n", pObj->codecId);
        goto ERR0;
    }

    memset(pObj->my_params, 0x0, sizeof(*pObj->my_params));

    pObj->params                            = (of_parameters_t *)pObj->my_params;
    pObj->params->nb_source_symbols         = pObj->k;
    pObj->params->nb_repair_symbols         = pObj->n - pObj->k;
    pObj->params->encoding_symbol_length    = pObj->fillLength;
    pObj->params->encoding_head_length      = pObj->headFillLength;

    status = of_create_codec_instance(&pObj->ses, pObj->codecId, OF_ENCODER, VERBOSITY);
    if (OF_STATUS_OK != status) {
        printf ("of_create_codec_instance failure\n");
        goto ERR1;
    }

    status = of_set_fec_parameters(pObj->ses, pObj->params);
    if (OF_STATUS_OK != status) {
        printf ("of_set_fec_parameters failure\n");
        goto ERR2;
    }

    memset(pObj->enc_symbols_tab, 0x0, sizeof(pObj->enc_symbols_tab));
    memset(pObj->enc_head_symbols_tab, 0x0, sizeof(pObj->enc_head_symbols_tab));

    return 0;
ERR2:
    if (pObj->ses) of_release_codec_instance(pObj->ses);
ERR1:
    if (pObj->my_params) free(pObj->my_params);
ERR0:
    return -1;
}

int redunOpenFecEncClose(RedunContext *oObj) {
    RedunManageEncPrivStu *pObj     = (RedunManageEncPrivStu *)oObj->priv;

    if (pObj->my_params) free(pObj->my_params);
    if (pObj->ses) of_release_codec_instance(pObj->ses);

    return 0;
}

int redunOpenFecEncReset(RedunContext *oObj) {
    int index                       = 0;
    RedunManageEncPrivStu *pObj     = (RedunManageEncPrivStu *)oObj->priv;

    pObj->recvdNum = 0;

    for (index = 0; index < pObj->n; index++) {
        pObj->enc_symbols_tab[index]  = NULL;
        pObj->enc_head_symbols_tab[index]  = NULL;
    }

    /* if (pObj->ses) of_reset_encoding_statu(pObj->ses); */

    return 0;
}

int redunOpenFecEncAdd(RedunContext *oObj, RedunBufList *pDataList) {
    int index                       = 0;
    RedunBuf *pBuf                  = NULL;
    RedunManageEncPrivStu *pObj     = (RedunManageEncPrivStu *)oObj->priv;

    for (index = 0; index < pDataList->numBufs; index++) {
        pBuf = pDataList->redunBuf[index];
        if (pBuf) {
            //检查openfec序列的有效性
            if (pBuf->sei >= pObj->n) {
                printf ("input data openfec sei:%d invalid, "
                        "range to [0 - %d]\n", pBuf->sei, pObj->n);
                goto ERR0;
            }

            if (pBuf->sei < pObj->k) {
                if (NULL == pBuf->data) {
                    printf ("input data openfec data:0x%p invalid\n", pBuf->data);
                    goto ERR0;
                }
            }
            else {
                if (NULL == pBuf->dataRepair) {
                    printf ("input data openfec dataRepair:0x%p invalid\n", pBuf->dataRepair);
                    goto ERR0;
                }
            }

            pObj->recvdNum++;
            pObj->enc_symbols_tab[pBuf->sei]        = pBuf->sei < pObj->k ? pBuf->data : pBuf->dataRepair;
            pObj->enc_head_symbols_tab[pBuf->sei]   = pBuf->sei < pObj->k ? pBuf->dataHead : pBuf->dataHeadRepair;
            printf ("%s %d redun sei:%d %s, rcvd:%d\n",
                    __func__, __LINE__, 
                    pBuf->sei, pBuf->sei >= pObj->k ? "repair" : "src", 
                    pObj->recvdNum);
        }
    }

    return 0;
ERR0:
    return -1;
}

int redunOpenFecEncProcess(RedunContext *oObj) {
    int index                       = 0;
    int status                      = -1;
    RedunManageEncPrivStu *pObj     = (RedunManageEncPrivStu *)oObj->priv;

    //冗余器接收到的源数据及冗余数据必需等于n
    if (pObj->recvdNum == pObj->n) {
        for (index = pObj->k; index < pObj->n; index++) {
            if (pObj->enc_symbols_tab[index] 
                    && pObj->enc_head_symbols_tab[index]) {
                //将接收到的源数据及冗余数据添加到冗余器中
                printf ("fillLength:%d headFillLength:%d\n", pObj->fillLength, pObj->headFillLength);
                status = of_build_repair_symbol(pObj->ses, 
                        pObj->enc_symbols_tab,
                        pObj->enc_head_symbols_tab,
                        index, 
                        pObj->fillLength, 
                        pObj->headFillLength);
                if (OF_STATUS_OK != status) {
                    printf ("of_build_repair_symbol sei:%d failure\n", index);
                    goto ERR0;
                }
            }
        }
    }
    else {
        printf ("[%s %d]input recvdNum:%d must == pObj->n:%d\n",
                __func__, __LINE__, pObj->recvdNum, pObj->n);
        goto ERR0;
    }

    return 0;
ERR0:
    return -1;
}

static int redunOpenFecEncCommand(RedunContext *oObj, unsigned cmd, void *value) {
    int index                       = 0;
    int status                      = -1;
    RedunManageEncPrivStu *pObj     = (RedunManageEncPrivStu *)oObj->priv;

    switch (cmd) {
        case REDUN_COMMAND_ISSAVE_FILE:
            {
                break;
            }
        default:
            break;
    }

    return 0;
}

RedunManageStu redunOpenFecAudEnc = {
    .name           = "OpenFec_Audio_Encode",
    .privSize       = sizeof(RedunManageEncPrivStu),
    .oopen          = redunOpenFecEncOpen,
    .cclose         = redunOpenFecEncClose,
    .reset          = redunOpenFecEncReset,
    .add            = redunOpenFecEncAdd,
    .process        = redunOpenFecEncProcess,
    .command        = redunOpenFecEncCommand,
};
