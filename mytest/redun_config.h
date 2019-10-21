#ifndef __RUDUN_OPENFEC_H__
#define __RUDUN_OPENFEC_H__ 

#include <of_openfec_api.h>

typedef struct RedunManageDecParmas {
    char k;
    char n;
    int fillLength;
    int headFillLength;
    int codecId;

    void *priv;

    void *(*requestsrc)(void *oObj, unsigned size, unsigned esi);       //申请源数据内存回调
    void *(*requestrepair)(void *oObj, unsigned size, unsigned esi);    //申请冗余数据内存回调
    void *(*requestsrcnew)(void *oObj,
            void **data, void **head, 
            unsigned size, unsigned head_size,
            unsigned esi);    //申请冗余数据内存回调
    void *(*requestrepairnew)(void *oObj,
            void **data, void **head, 
            unsigned size, unsigned head_size,
            unsigned esi);    //申请冗余数据内存回调
    int (*release)(void *oObj, RedunBuf *redunBuf);
} RedunManageDecParmas;

typedef struct RedunManageEncParmas {
    char k;
    char n;
    int fillLength;
    int headFillLength;
    of_codec_id_t codecId;

    void *priv;
} RedunManageEncParmas;

#endif /*__RUDUN_OPENFEC_H__*/
