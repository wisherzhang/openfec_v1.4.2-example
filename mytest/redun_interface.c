#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "redun_interface.h"

/**************** server **********************/
/**************** server **********************/
/**************** server **********************/
/**************** server **********************/
/**************** server **********************/
/**************** server **********************/
/**************** server **********************/
/**************** server **********************/
static int RedunServerAutoRecycle(RedunServerObj *oObj) {
    int ret             = 0;
    RedunBuf *redunBuf    = NULL;
    RedunServerObj *pObj = (RedunServerObj *)oObj;

    while (1) {
        ret = QueueGet(&pObj->hndlRes, (long long *)&redunBuf, TIMEOUT_NONE);
        if (ret) break;

        redunBuf->refId--;
        if (redunBuf->refId <= 0) {
            redunBuf->refId = 0;

            QueuePut((QueHndl *)redunBuf->handle, (long long)redunBuf, TIMEOUT_NONE);

            if (pObj->release) {
                pObj->release(pObj->priv, redunBuf->priv);
            }
        }
    }

    return 0;
}

static void *RedunServerTransformFromRedun(void *oObj, 
        void *redunBuf, void *repair, void *headRepair, int isSrc) {
    int ret                 = -1;
    void *data              = NULL;
    RedunServerObj *pObj    = (RedunServerObj *)oObj;

    if (pObj->transformFromRedun) {
        data = pObj->transformFromRedun(pObj->priv, redunBuf, repair, headRepair, isSrc);
    }

    return data;
}

static void *RedunServerTransformToRedun(void *oObj, void **redunBuff, 
        void *data, int isSrc) {
    int ret                 = -1;
    RedunBuf *redunBuf      = NULL;
    RedunServerObj *pObj    = (RedunServerObj *)oObj;

    if (pObj->transformToRedun) {
        redunBuf = pObj->transformToRedun(pObj->priv, redunBuff, data, isSrc);
        if (redunBuf) {
            redunBuf->handleRes = (void *)&pObj->hndlRes;
        }
    }

    return redunBuf;
}

int RedunServerSend(void *oObj, void *data) {
    RedunServerObj *pObj    = (RedunServerObj *)oObj;

    if (pObj->ssend) {
        return pObj->ssend(pObj->priv, data);
    }

    return -1;
}

unsigned RedunServerGetSeqId(void *oObj, void *data) {
    RedunServerObj *pObj    = (RedunServerObj *)oObj;

    if (pObj->getSeqId) {
        return pObj->getSeqId(pObj->priv, data);
    }

    return 0x0;
}

int RedunServerReleaseRedunBuf(void *oObj, void *data) {
    RedunBuf *redunBuf      = (RedunBuf *)data;

    RedunServerRecyleRedunBuf(redunBuf->priv);

    return 0;
}


static int RedunServerMemInit(RedunServerObj *pObj) {
    int ret     = -1;
    int index   = 0;

    ret = QueueCreate(&pObj->hndlE, 1000);
    if (ret) {
        printf ("[%s %d]QueueCreate pObj->hndlE failure\n", __func__, __LINE__);
        goto ERR0;
    }

    ret = QueueCreate(&pObj->hndlRes, 1000);
    if (ret) {
        printf ("[%s %d]QueueCreate pObj->hndlRes failure\n", __func__, __LINE__);
        goto ERR1;
    }

    memset(&pObj->redunBuf[0], 0x0, sizeof(pObj->redunBuf));

    for (index = 0; index < REDUN_MAX_INPUT_NUM; index++) {
        pObj->redunBuf[index].handle = (void *)&pObj->hndlE;
        ret = QueuePut(&pObj->hndlE, (long long)&pObj->redunBuf[index], TIMEOUT_NONE);
        if (ret) {
            printf ("[%s %d] QueuePut hndlE index:%d failure\n", __func__, __LINE__, index);
        }
    }

    return 0;
ERR1:
    QueueDelete(&pObj->hndlE);
ERR0:
    return -1;
}

static int RedunServerMemExit(RedunServerObj *pObj) {
    QueueDelete(&pObj->hndlE);
    QueueDelete(&pObj->hndlRes);

    return 0;
}

/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/****Server*******************************************/
/*** starting ********** interface *******************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
int RedunServerRecyleRedunBuf(void *data) {
    serverClientBuf *pBuf = (serverClientBuf *)data;

    RedunBuf *redunBuf = (RedunBuf *)pBuf->priv;

    return QueuePut((QueHndl *)redunBuf->handleRes, (long long)redunBuf, TIMEOUT_NONE);
}


/*外部调用初始化接口，初始化完成后直接调用initInst接口，完成实际的初始化动作*/
int RedunServerInit(RedunServerObj *pObj, 
        int k, int n, int fillLength, 
        int headFillLength,
        int codec_id, void *priv, 
        transformto toRedun, 
        transformfrom fromRedun, 
        release rls, 
        ssend sendd, 
        getSeqId getseqid) {
    int ret                 = -1;
    int index               = 0;

    printf ("%s %d  k:%d n:%d codecId:%d fillLength:%d headFillLength:%d " 
            "priv:%p toRedun:%p fromRedun:%p rls:%p sendd:%p \n", 
            __func__, __LINE__, k, n, codec_id, fillLength, headFillLength, 
            priv, toRedun, fromRedun, rls, sendd);

    pObj->k                     = k;
    pObj->n                     = n;
    pObj->priv                  = priv;
    pObj->codec_id              = codec_id;
    pObj->fillLength            = fillLength;
    pObj->headFillLength        = headFillLength;
    pObj->transformToRedun      = toRedun;
    pObj->transformFromRedun    = fromRedun;
    pObj->ssend                 = sendd;
    pObj->release               = rls;
    pObj->getSeqId              = getseqid;

    pObj->redunObj = RedunManageGetByName("OpenFec_Audio_Encode");
    if (NULL == pObj->redunObj) {
        printf ("[%s %d] RedunManageGetByName %s failure\n", 
                __func__, __LINE__, "OpenFec_Audio_Encode");
        goto ERR0;
    }

    pObj->redunContext = RedunContextInstance(pObj->redunObj);
    if (NULL == pObj->redunContext) {
        printf ("[%s %d] RedunContextInstance failure\n", __func__, __LINE__);
        goto ERR0;
    }

    pObj->config.k              = pObj->k;
    pObj->config.n              = pObj->n;
    pObj->config.fillLength     = pObj->fillLength;
    pObj->config.headFillLength = pObj->headFillLength;
    pObj->config.codecId        = pObj->codec_id;
    pObj->config.priv           = pObj;

    ret = RedunContextInit(pObj->redunContext, (void *)&pObj->config);
    if (ret) {
        printf ("[%s %d] RedunContextInit %s failure\n",
                __func__, __LINE__, "OpenFec_Audio_Encode");
        goto ERR1;
    }

    pObj->preRedunObj = PreRedunManageGetByName("Pre_OpenFec_Audio_Encode");
    if (NULL == pObj->preRedunObj) {
        printf ("[%s %d] PreRedunManageGetByName %s failure\n", 
                __func__, __LINE__, "PreRedunManageGetByName");
        goto ERR2;
    }

    pObj->preRedunContext = PreRedunContextInstance(pObj->preRedunObj);
    if (NULL == pObj->preRedunContext) {
        printf ("[%s %d] PreRedunContextInstance failure\n", __func__, __LINE__);
        goto ERR2;
    }

    pObj->configg.k                         = pObj->k;
    pObj->configg.n                         = pObj->n;
    pObj->configg.priv                      = pObj;
    pObj->configg.fillLength                = pObj->fillLength;
    pObj->configg.headFillLength            = pObj->headFillLength;
    pObj->configg.transformFromRedun        = RedunServerTransformFromRedun;
    pObj->configg.transformToRedun          = RedunServerTransformToRedun;
    pObj->configg.ssend                     = RedunServerSend;
    pObj->configg.release                   = RedunServerReleaseRedunBuf;
    pObj->configg.getSeqId                  = RedunServerGetSeqId;

    ret = PreRedunContextInit(pObj->preRedunContext, (void *)&pObj->configg);
    if (ret) {
        printf ("[%s %d] PreRedunContextInit failure\n", __func__, __LINE__);
        goto ERR3;
    }

    ret = RedunServerMemInit(pObj);
    if (ret) {
        goto ERR4;
    }

    return 0;
ERR4:
    PreRedunContextExit(pObj->preRedunContext);
ERR3:
    PreRedunContextUninstance(pObj->preRedunContext);
ERR2:
    RedunContextExit(pObj->redunContext);
ERR1:
    RedunContextUninstance(pObj->redunContext);
ERR0:
    return -1;
}

int RedunServerExit(RedunServerObj *pObj) {

    RedunServerMemExit(pObj);

    RedunContextExit(pObj->redunContext);

    RedunContextUninstance(pObj->redunContext);

    PreRedunContextExit(pObj->preRedunContext);

    PreRedunContextUninstance(pObj->preRedunContext);

    return 0;
}

/*要保证数据接口的无关联性，这样才能做到通用*/
int RedunServerRun(RedunServerObj *pObj, void *data) {
    int ret                     = -1;
    RedunBufList redunBufList   = {.numBufs = 0};

    //数据发送前需要做一定的字段填充，音频视频都有可能
    ret = PreRedunContextProcessFrame(pObj->preRedunContext, data, &redunBufList);
    if (ret) {
        RedunServerAutoRecycle(pObj);
        return -1;
    }

    RedunContextAddFrameList(pObj->redunContext, &redunBufList);

    RedunContextProcessFrame(pObj->redunContext);

    RedunContextReset(pObj->redunContext);

    PreRedunContextProcessAfter(pObj->preRedunContext);

    /*归还本端及前一级的数据包*/
    RedunServerAutoRecycle(pObj);

    return 0;
ERR0:
    return -1;
}

int RedunServerCommand(RedunServerObj *pObj, unsigned cmd, void *value) {
    return PreRedunContextCommand(pObj->preRedunContext, cmd, value);
}
/*** ended ********** interface *******************/

/**************** client **********************/
/**************** client **********************/
/**************** client **********************/
/**************** client **********************/
/**************** client **********************/

static int RedunClientAutoRecycle(void *oObj) {
    int ret                 = 0;
    RedunBuf *redunBuf      = NULL;
    RedunClientObj *pObj    = (RedunClientObj *)oObj;

    while (1) {
        ret = QueueGet(&pObj->hndlRes, (long long *)&redunBuf, TIMEOUT_NONE);
        if (ret) break;

        redunBuf->refId--;
        if (redunBuf->refId <= 0) {
            redunBuf->refId = 0;

    printf ("%s %d seqId:%d sei:%d data:%p ref:%d\n", __func__, __LINE__, 0, redunBuf->sei, redunBuf, redunBuf->refId);
            QueuePut((QueHndl *)redunBuf->handle, (long long)redunBuf, TIMEOUT_NONE);

            pObj->release(pObj->priv, redunBuf->priv);
        }
    }

    return 0;
}

static void *RedunClientTransformToRedun(void *oObj, void **redunBuff,
        void *data, int isSrc) {
    int ret                 = -1;
    RedunBuf *redunBuf      = (RedunBuf *)(*((RedunBuf **)redunBuff));
    RedunClientObj *pObj    = (RedunClientObj *)oObj;

    if (pObj->transformToRedun) {
        redunBuf = pObj->transformToRedun(pObj->priv, (void **)&redunBuf, data, isSrc);
        if (redunBuf) {
            redunBuf->handleRes = (void *)&pObj->hndlRes;
        }
    }

ERR0:
    return redunBuf;
}

static void *RedunClientTransformFromRedun(void *oObj, 
        void *redunBuff, void *repair, void *headRepair, int isSrc) {
    int ret             = -1;
    void *data          = NULL;
    RedunBuf *redunBuf  = redunBuff;
    RedunClientObj *pObj = (RedunClientObj *)oObj;

    if (pObj->transformFromRedun) {
        data = pObj->transformFromRedun(pObj->priv, redunBuf, repair, headRepair, isSrc);
    }

ERR0:
    return data;
}

static int RedunClientSend(void *oObj, void *data) {
    RedunClientObj *pObj = (RedunClientObj *)oObj;

    if (pObj->ssend) {
        return pObj->ssend(pObj->priv, data);
    }

    return -1;
}

static unsigned RedunClientGetSeqId(void *oObj, void *data) {
    RedunClientObj *pObj = (RedunClientObj *)oObj;

    if (pObj->getSeqId) {
        return pObj->getSeqId(pObj->priv, data);
    }

    return 0x0;
}

static void *RedunClientRequestRepairMem(void *data, 
        void **payLoad, void **head, 
        unsigned size, unsigned headSize, unsigned esi) {
    //该接口暂不确定是否有使用的可行性，暂不处理

/*
 *     int ret                 = -1;
 *     RedunBuf *redunBuf      = NULL;
 *     RedunClientObj *pObj    = (RedunClientObj *)data; 
 * 
 *     if (NULL == payLoad || NULL == head) {
 *         printf ("[%s %d]request size:%d headSize:%d\n", __func__, __LINE__, size, headSize);
 *         return NULL;
 *     }
 * 
 *     ret = QueueGet(&pObj->hndlE, (long long *)&redunBuf, TIMEOUT_NONE);
 *     if (ret) {
 *         printf ("%s %d QueueGet failure\n", __func__, __LINE__);
 *         return NULL;
 *     }
 * 
 *     //关于头部分的内容还需要再确定一下
 *     void *pBuf = (void *)pObj->requestsrcnew(pObj, size, esi);
 *     if (pBuf) {
 *         memset(&pObj->headTmp[esi][0], 0x0, headSize);
 *         RedunClientTransformToRedun(pObj, redunBuf, pBuf);
 *         redunBuf->dataHead          = &pObj->headTmp[esi][0];
 *         redunBuf->sei               = esi;
 *         redunBuf->handleRes         = &pObj->hndlRes;
 *         *payLoad                    = redunBuf->data;
 *         *head                       = redunBuf->dataHead;
 * 
 *         PreRedunContextAddRecoverFrame(pObj->preRedunContext, redunBuf);
 * 
 *         return (void *)redunBuf->data;
 *     }
 *     else {
 *         QueuePut(&pObj->hndlE, (long long)redunBuf, TIMEOUT_NONE);
 *     }
 */

    return NULL;
}

static void *RedunClientRequestSrcMem(void *data, 
        void **payLoad, void **head, 
        unsigned size, unsigned headSize, unsigned esi) {

    int ret                 = -1;
    RedunBuf *redunBuf      = NULL;
    RedunClientObj *pObj    = (RedunClientObj *)data; 

    if (NULL == payLoad || NULL == head) {
        printf ("request size:%d headSize:%d\n", size, headSize);
        return NULL;
    }

    ret = QueueGet(&pObj->hndlE, (long long *)&redunBuf, TIMEOUT_NONE);
    if (ret) {
        printf ("%s %d QueueGet failure\n", __func__, __LINE__);
        return NULL;
    }

    void *pBuf = (void *)pObj->requestsrcnew(pObj, size, esi);
    if (pBuf) {
        redunBuf = RedunClientTransformToRedun(pObj, (void **)&redunBuf, pBuf, 1);
        memset(redunBuf->dataHead, 0x0, headSize);
        redunBuf->sei               = esi;
        redunBuf->handleRes         = &pObj->hndlRes;
        *payLoad                    = redunBuf->data;
        *head                       = redunBuf->dataHead;

        /*
         * printf ("############sei:%d recvd_symbols_tab:%p recvd_head_symbols_tab:%p\n", 
         *         esi, redunBuf->data, redunBuf->dataHead);
         */

        PreRedunContextAddRecoverFrame(pObj->preRedunContext, redunBuf);

        return (void *)redunBuf->data;
    }
    else {
        QueuePut(&pObj->hndlE, (long long)redunBuf, TIMEOUT_NONE);
    }

    return NULL;
}

static int RedunClientReleaseRedunBuf(void *oObj, void *data) {
    RedunBuf *redunBuf      = (RedunBuf *)data;

    RedunClientRecyleRedunBuf(redunBuf->priv);

    return 0;
}


static int RedunClientMemInit(RedunClientObj *pObj) {
    int ret     = -1;
    int index   = 0;

    ret = QueueCreate(&pObj->hndlE, 1000);
    if (ret) {
        printf ("[%s %d]QueueCreate pObj->hndlE failure\n", __func__, __LINE__);
        goto ERR0;
    }

    ret = QueueCreate(&pObj->hndlRes, 1000);
    if (ret) {
        printf ("[%s %d]QueueCreate pObj->hndlRes failure\n", __func__, __LINE__);
        goto ERR1;
    }

    memset(&pObj->redunBuf[0], 0x0, sizeof(pObj->redunBuf));

    for (index = 0; index < REDUN_MAX_INPUT_NUM; index++) {
        pObj->redunBuf[index].handle = (void *)&pObj->hndlE;
        ret = QueuePut(&pObj->hndlE, (long long)&pObj->redunBuf[index], TIMEOUT_NONE);
        if (ret) {
            printf ("[%s %d] QueuePut hndlE index:%d failure\n", __func__, __LINE__, index);
        }
    }

    return 0;
ERR1:
    QueueDelete(&pObj->hndlE);
ERR0:
    return -1;
}

static int RedunClientMemExit(RedunClientObj *pObj) {
    QueueDelete(&pObj->hndlE);
    QueueDelete(&pObj->hndlRes);

    return 0;
}

/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/****Client*******************************************/
/*** starting ********** interface *******************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
int RedunClientCommand(RedunClientObj *pObj, unsigned cmd, void *value) {
    return PreRedunContextCommand(pObj->preRedunContext, cmd, value);
}

int RedunClientRecyleRedunBuf(void *data) {
    serverClientBuf *pBuf = (serverClientBuf *)data;

    RedunBuf *redunBuf = (RedunBuf *)pBuf->priv;

    return QueuePut((QueHndl *)redunBuf->handleRes, (long long)redunBuf, TIMEOUT_NONE);
}


int RedunClientRun(RedunClientObj *pObj, void *data) {
    int status                  = -1;
    RedunBufList pDataList      = {
        .numBufs                = 0,
    };

    //分组处理
    status = PreRedunContextProcessFrame(pObj->preRedunContext, data, &pDataList);
    if (0 != status) {
        RedunClientAutoRecycle(pObj);
        return -1;
    }

    //将分组数据添加到冗余器
    RedunContextAddFrameList(pObj->redunContext, &pDataList);

    //开始处理冗余
    RedunContextProcessFrame(pObj->redunContext);

    //每次处理一个分组后需要复位冗余器
    RedunContextReset(pObj->redunContext);

    //做一次冗余后，进行数据发送等相关的处理
    PreRedunContextProcessAfter(pObj->preRedunContext);

    RedunClientAutoRecycle(pObj);

    return 0;
}

/*外部调用初始化接口，初始化完成后直接调用initInst接口，完成实际的初始化动作*/
int RedunClientInit(RedunClientObj *pObj, 
        int k, int n, int fillLength, int headFillLength,
        int codec_id, void *priv,
        request requestsrcnew, 
        request requestrepairnew,
        transformto toRedun, 
        transformfrom fromRedun, 
        release rls, 
        ssend sendd, 
        getSeqId getseqid) {
    int index                   = 0;
    int ret                     = -1;

    memset(pObj, 0x0, sizeof(*pObj));

    pObj->k                     = k;
    pObj->n                     = n;
    pObj->priv                  = priv;
    pObj->codec_id              = codec_id;
    pObj->fillLength            = fillLength;
    pObj->headFillLength        = headFillLength;
    pObj->requestsrcnew         = requestsrcnew;
    pObj->requestrepairnew      = requestrepairnew;
    pObj->transformToRedun      = toRedun;
    pObj->transformFromRedun    = fromRedun;
    pObj->release               = rls;
    pObj->ssend                 = sendd;
    pObj->getSeqId              = getseqid;

    pObj->redunObj = RedunManageGetByName("OpenFec_Audio_Decode");
    if (NULL == pObj->redunObj) {
        printf ("[%s %d] RedunManageGetByName %s failure\n", 
                __func__, __LINE__, "OpenFec_Audio_Decode");
        goto ERR0;
    }

    pObj->redunContext = RedunContextInstance(pObj->redunObj);
    if (NULL == pObj->redunContext) {
        printf ("[%s %d] RedunContextInstance failure\n", __func__, __LINE__);
        goto ERR0;
    }

    pObj->config.k                  = pObj->k;
    pObj->config.n                  = pObj->n;
    pObj->config.fillLength         = pObj->fillLength;
    pObj->config.headFillLength     = pObj->headFillLength;
    pObj->config.codecId            = pObj->codec_id;
    pObj->config.requestsrcnew      = RedunClientRequestSrcMem;
    //暂不确定是否需要，暂不添加
    pObj->config.requestrepairnew   = NULL;//RedunClientRequestRepairMem;
    pObj->config.priv               = pObj;

    ret = RedunContextInit(pObj->redunContext, (void *)&pObj->config);
    if (ret) {
        printf ("[%s %d] RedunContextInit %s failure\n",
                __func__, __LINE__, "OpenFec_Audio_Decode");
        goto ERR1;
    }

    pObj->preRedunObj = PreRedunManageGetByName("Pre_OpenFec_Audio_Decode");
    if (NULL == pObj->preRedunObj) {
        printf ("[%s %d] PreRedunManageGetByName %s failure\n", 
                __func__, __LINE__, "PreRedunManageGetByName");
        goto ERR2;
    }

    pObj->preRedunContext = PreRedunContextInstance(pObj->preRedunObj);
    if (NULL == pObj->preRedunContext) {
        printf ("[%s %d] PreRedunContextInstance failure\n", __func__, __LINE__);
        goto ERR2;
    }

    pObj->configg.k                     = pObj->k;
    pObj->configg.n                     = pObj->n;
    pObj->configg.fillLength            = pObj->fillLength;
    pObj->configg.headFillLength        = pObj->headFillLength;
    pObj->configg.priv                  = pObj;
    pObj->configg.transformToRedun      = RedunClientTransformToRedun;
    pObj->configg.transformFromRedun    = RedunClientTransformFromRedun;
    pObj->configg.ssend                 = RedunClientSend;
    pObj->configg.release               = RedunClientReleaseRedunBuf;
    pObj->configg.getSeqId              = RedunClientGetSeqId;
    ret = PreRedunContextInit(pObj->preRedunContext, (void *)&pObj->configg);
    if (ret) {
        printf ("[%s %d] PreRedunContextInit failure\n", __func__, __LINE__);
        goto ERR3;
    }

    ret = RedunClientMemInit(pObj);
    if (ret) {
        goto ERR4;
    }

    return 0;
ERR4:
    PreRedunContextExit(pObj->preRedunContext);
ERR3:
    PreRedunContextUninstance(pObj->preRedunContext);
ERR2:
    RedunContextExit(pObj->redunContext);
ERR1:
    RedunContextUninstance(pObj->redunContext);
ERR0:
    return -1;
}

int RedunClientExit(RedunClientObj *pObj) {

    RedunClientMemExit(pObj);

    RedunContextExit(pObj->redunContext);

    RedunContextUninstance(pObj->redunContext);

    PreRedunContextExit(pObj->preRedunContext);

    PreRedunContextUninstance(pObj->preRedunContext);

    return 0;
}
/*** ended ********** interface *******************/
