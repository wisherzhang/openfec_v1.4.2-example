#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <queue.h>
#include <preredun.h>
#include <redun.h>
#include <preredun_config.h>

#include <redun_interface.h>

/********************* OpenFec_Audio_Decode ******************/

//冗余预处理结构
typedef struct PreRedunManageDecPrivStu {
    //源数据包个数
    char k;           
    //源数据与冗余包个数总和
    char n;                             
    //数据大小
    char fillLength;
    //头数据大小
    char headFillLength;
    //切换临时内容标志           
    char flag;              
    //是否是第一个接收到的包
    char isFirstPkt;              
    //上一个包的算法序列号[0-n]
    char sei;                   
    //当前包的算法序列号[0-n]
    char curSei;
    //当前接收到一组数据包的个数[< n]
    char recvdNum;                      

    //如果丢包量大于分组总大小(n)，置位，
    //表示当前接收到的是下一个分组的数据，
    //前一个分组数据无法恢复了，直接将未
    //发送的包发送出去并开始下一个分组的处理
    char isDecChange;                   
    //当前的序列号
    unsigned seqId; 
    //上一次接收包的RTP序列号
    unsigned lastRecvPktSeq;            
    //是否丢包，第一个位表示对应序列是否丢包 1 << esi
    unsigned isDrop; 
    //分组内丢包个数
    unsigned dropNum;   
    //临时存储接收数据帧的首地址
    void *recvdValidTab[REDUN_MAX_NUM];           
    //存储下的数据帧是否已经发送，是否发送由前面的数据是否有丢帧决定
    unsigned char recvdIsSend[REDUN_MAX_NUM];  
    //源数据指针存储表
    void *srcSymbolsTab[REDUN_MAX_NUM];

    //头部分使用内存块
    unsigned char repairHeadSymbolsTab[REDUN_ALT_NUM][REDUN_MAX_NUM][REDUN_MAX_HEAD_LENGTH];
                                    
    //获取序列号接口
    unsigned (*getSeqId)(void *oObj, void *data);

    //转换内部数据结构到外部数据结构
    void *(*transformFromRedun)(void *oObj, void *data, void *repair, void *headRepair, int isSrc);

    //转换外部数据结构到内部数据结构
    void *(*transformToRedun)(void *oObj, void **redunBuf, void *data, int isSrc);

    //发送数据接口
    int (*ssend)(void *oObj, void *data);

    //释放数据接口
    int (*release)(void *oObj, void *redunBuf);

    //内部结构
    QueHndl hndlE;
    RedunBuf redunBuf[REDUN_BUFFER_NUM];

    //指向回调接口对接的外部句柄
    void *priv;

    FILE *fp;
} PreRedunManageDecPrivStu ;

/*
 *  分组管理组件初始化接口
 */
int preRedunOpenFecDecInit(PreRedunContext *oObj, void *config) {
    int status                          = -1;
    int index                           = 0;
    PreRedunManageDecPrivStu *pObj      = (PreRedunManageDecPrivStu *)oObj->priv;
    PreRedunManageDecParmas *params     = (PreRedunManageDecParmas *)config;

    memset(pObj, 0x0, sizeof(*pObj));

    printf ("%s %d input check k:%d n:%d priv:%p "
            "fillLength:%d headFillLength:%d "
            "transformFromRedun:%p "
            "transformToRedun:%p "
            "ssend:%p release:%p\n", 
            __func__, __LINE__, 
            params->k, 
            params->n, 
            params->priv, 
            params->fillLength, 
            params->headFillLength,
            params->transformFromRedun, 
            params->transformToRedun, 
            params->ssend, 
            params->release);

    if (params->fillLength % 2 != 0 
            || params->headFillLength % 2 != 0) {
        printf ("[%s %d] fillLength:%d headFillLength:%d not match[len mod 2 == 0]\n", 
                __func__, __LINE__, params->fillLength, params->headFillLength);
        goto ERR0;
    }

    if (NULL == params->transformFromRedun 
            || NULL == params->transformToRedun 
            || NULL == params->ssend 
            || NULL == params->release) {
        printf ("[%s %d] input callback invalid[%d-%d-%d-%d]\n",
                __func__, __LINE__, 
                NULL == params->transformFromRedun, 
                NULL == params->transformToRedun, 
                NULL == params->ssend, 
                NULL == params->release);
        goto ERR0;
    }

    pObj->k                             = params->k;
    pObj->n                             = params->n;
    pObj->fillLength                    = params->fillLength;
    pObj->headFillLength                = params->headFillLength;
    pObj->priv                          = params->priv;
    pObj->isFirstPkt                    = 1;

    pObj->transformFromRedun            = params->transformFromRedun;
    pObj->transformToRedun              = params->transformToRedun;
    pObj->ssend                         = params->ssend;
    pObj->release                       = params->release;
    pObj->getSeqId                      = params->getSeqId;

    //内部数据管理结构相关的初始化
    status = QueueCreate(&pObj->hndlE, 1000);
    if (status) {
        printf ("[%s %d]QueueCreate pObj->hndlE failure\n", __func__, __LINE__);
        goto ERR0;
    }

    for (index = 0; index < REDUN_BUFFER_NUM; index++) {
        pObj->redunBuf[index].handle = (void *)&pObj->hndlE;
        status = QueuePut(&pObj->hndlE, (long long)&pObj->redunBuf[index], TIMEOUT_NONE);
        if (status) {
            printf ("[%s %d] QueuePut hndlE index:%d failure\n", __func__, __LINE__, index);
        }
    }

    /*test*/
    pObj->fp = fopen("./srcModify.pcm", "w+");

    return 0;
ERR0:
    return -1;
}

/*
 *  分组管理组件退出接口
 */
int preRedunOpenFecDecExit(PreRedunContext *oObj) {
    PreRedunManageDecPrivStu *pObj      = (PreRedunManageDecPrivStu *)oObj->priv;

    QueueDelete(&pObj->hndlE);

    return 0;
}

static void *PreRedunContextOpenFecDecGetSeqId(PreRedunManageDecPrivStu *pObj, void *data) {
    if (pObj->getSeqId) {
        pObj->seqId = pObj->getSeqId(pObj->priv, data);
    }

    printf ("pObj->seqId:%d\n", pObj->seqId);
}

/*
 *  对接收到的数据包进行内部数据结构转换处理
 *  参数1  ---->略
 *  参数2  ---->外部接收数据结构
 *  返回值 ---->转换后的内部结构指针
 */
static RedunBuf *PreRedunContextOpenFecDecTransform(PreRedunManageDecPrivStu *pObj, void *data) {
    int ret             = -1;
    RedunBuf *redunBuf  = NULL;

    ret = QueueGet(&pObj->hndlE, (long long *)&redunBuf, TIMEOUT_NONE);
    if (ret) {
        printf ("%s %d QueueGet failure\n", __func__, __LINE__);
        goto ERR0;
    }

    //将收到的数据包将换成冗余器内部数据结构,
    if (pObj->transformToRedun) {
        redunBuf = pObj->transformToRedun(pObj->priv, (void **)&redunBuf, 
                data, pObj->curSei < pObj->k ? 1 : 0);
    }

    return redunBuf;
ERR0:
    return NULL;
}

/*
 *  清可能的无效分组信息
 *  参数1  ---->略
 *  返回值 ---->略
 */
static int PreRedunContextOpenFecDecCalcSei(PreRedunManageDecPrivStu  *pObj);
static int PreRedunContextOpenFecDecClearFrame(PreRedunManageDecPrivStu  *pObj) {
    int index           = 0;
    int num             = 0;
    void *data          = NULL;
    RedunBuf *redunBuf  = NULL;

    if (pObj->isDecChange) {
        /*清分组*/
        for (index = 0; index < pObj->k; index++) {
            if (pObj->recvdValidTab[index]) {
                redunBuf = (RedunBuf *)pObj->recvdValidTab[index];
                printf ("isDecChange valid, so drop sei:%d\n", redunBuf->sei);
                //接收到有效数据减少一个单位
                pObj->recvdNum--;
                //如果数据包不没有发送过，清分组的时候，要把未发送的再发送出去
                if (!pObj->recvdIsSend[index]) {
                    redunBuf->sei -= pObj->k;
                    if (pObj->transformToRedun) {
                        redunBuf = pObj->transformToRedun(pObj->priv, (void **)&redunBuf, redunBuf->priv, 1);
                    }

                    if (pObj->transformFromRedun) {
                        data = pObj->transformFromRedun(pObj->priv, pObj->recvdValidTab[index], NULL, NULL, 1);
                        if (data) {
                            if (pObj->ssend) {
                                //没有发送过的数据，清掉等于直接释放
                                redunBuf            = pObj->recvdValidTab[index];
                                redunBuf->refId     = REDUN_REF_JUST_SEND_NUM;
                                pObj->ssend(pObj->priv, data);
                            }
                        }
                    }
                }
                pObj->recvdValidTab[index] = NULL;
            }
            pObj->recvdIsSend[index] = 0;
        }

        //清分组的时候，将可能存在的下一个分组的数据存储在当前分组的位置，
        //便于下一个周期的处理
        if (pObj->recvdNum) {
            int refNum = 0;
            int lastIndex = 0;
            for (index = pObj->k; index < pObj->n; index++) {
                redunBuf                                = (RedunBuf *)pObj->recvdValidTab[index];
                pObj->recvdValidTab[index - pObj->k]    = pObj->recvdValidTab[index];
                pObj->recvdIsSend[index - pObj->k]      = pObj->recvdIsSend[index];
                //清掉冗余部分的存储状态
                pObj->recvdValidTab[index]              = NULL;
                pObj->recvdIsSend[index]                = 0;
                if (pObj->recvdValidTab[index - pObj->k]) {
                    redunBuf->sei -= pObj->k;
                    if (pObj->transformToRedun) {
                        redunBuf = pObj->transformToRedun(pObj->priv, (void **)&redunBuf, redunBuf->priv, 1);
                    }
                    //有效数据包的引用计数
                    if (refNum == index - pObj->k) {
                        //连续的数据包可以发送了
                        if (pObj->transformFromRedun) {
                            data = pObj->transformFromRedun(pObj->priv, redunBuf, NULL, NULL, 1);
                            if (data) {
                                if (pObj->ssend) {
                                    //数据提前发送，需要临时存储
                                    redunBuf            = pObj->recvdValidTab[index - pObj->k];
                                    redunBuf->refId = REDUN_REF_RESTORE_NUM;
                                    pObj->ssend(pObj->priv, data);
                                }
                            }
                        }
                    }
                    refNum++;
                    lastIndex = index;
                }
            }

            //计算丢包量
            for (index = 0; index < lastIndex; index++) {
                if (NULL == pObj->recvdValidTab[index]) {
                    pObj->dropNum++;
                    pObj->isDrop |= (1 << index);
                }
            }

            pObj->isDecChange = 0;
        }
        else {
            printf ("recv src done ............, so clear\n");
            pObj->isDrop        = 0;
            pObj->dropNum       = 0;
            pObj->recvdNum      = 0;

            pObj->sei           = 0;
            /* pObj->isFirstPkt    = 1; */
            pObj->isDecChange   = 0;

            PreRedunContextOpenFecDecCalcSei(pObj);
        }
    }

    return 0;
}

/*
 *  通过数据包的序列号计算内部分组序列号
 *  参数1  ---->略
 *  参数2  ---->转换后的数据包地址
 *  返回值 ---->略
 */
static int PreRedunContextOpenFecDecCalcSei(PreRedunManageDecPrivStu  *pObj) {
    int index                       = 0;
    unsigned dropPack               = 1;

    printf ("SSSSSSSS isFirstPkt:%d pObj->sei:%d\n", pObj->isFirstPkt, pObj->sei);
    if (pObj->isFirstPkt) {
        pObj->sei               = pObj->seqId % pObj->k;
        dropPack                = pObj->sei + 1;
        pObj->isFirstPkt        = 0;
        pObj->lastRecvPktSeq    = pObj->seqId;
        printf ("###seqId:%d sei:%d dropPack:%d\n", pObj->seqId, pObj->sei, dropPack);
    }
    else {
        if ((pObj->seqId != ((pObj->lastRecvPktSeq + 1) & 0x0000FFFF)) 
                && (pObj->seqId != (pObj->lastRecvPktSeq & 0x0000FFFF))) {
            //计算丢包情况
            dropPack = pObj->seqId - pObj->lastRecvPktSeq;
        }

        if (dropPack - 1 > pObj->n - pObj->k) {

        }

        //通过丢包及分组量计算出分组序列号
        pObj->sei = (pObj->sei + dropPack % pObj->n) % pObj->n;
        printf ("seqId:%d lastSeqId:%d sei:%d dropPack:%d\n", 
                pObj->seqId, pObj->lastRecvPktSeq, pObj->sei, dropPack);
    }

    if (dropPack - 1 > 0) {
        pObj->isDrop |= (1 << pObj->sei);
        pObj->dropNum += dropPack - 1;//统计一个分组内的丢包个数
    }

    //分组内丢包大于冗余包个数，说明分组无法恢复
    if (pObj->dropNum > pObj->n - pObj->k) {
        pObj->isDecChange = 1;
        printf ("now isDecChange:%d\n", pObj->isDecChange);
    }

    pObj->lastRecvPktSeq    = pObj->seqId;  //记录当前序列号用于下一次
    pObj->curSei            = pObj->sei;    //重!!!计算分组序列

    printf ("[%s %d]seqId:%d isDrop:0x%08x dropNum:%u sei:%d isDecChange:%d\n",
            __func__, __LINE__,
            pObj->seqId, pObj->isDrop, pObj->dropNum, pObj->curSei, pObj->isDecChange);

    return 0;
}

/*
 *  将收到的转换后的数据包存储起来,通过计算没有丢包的情况下，数据包直接发送并标记
 *  参数1  ---->略
 *  参数2  ---->转换后的数据包地址
 *  返回值 ---->略
 */
static int PreRedunContextOpenFecDecAddFrame(PreRedunManageDecPrivStu *pObj, RedunBuf *redunBuf) {
    int index           = 0;
    void *data          = NULL;

    redunBuf->sei = pObj->curSei;

    if (NULL == pObj->recvdValidTab[pObj->curSei]) {
        pObj->recvdValidTab[pObj->curSei] = (void *)redunBuf;//将数据包临时存储下来
        if (!pObj->isDrop) {
            if (pObj->transformFromRedun) {
                data = pObj->transformFromRedun(pObj->priv, pObj->recvdValidTab[pObj->curSei], NULL, NULL, 1);
                if (data) {
                    if (pObj->ssend) {
                        redunBuf->refId = REDUN_REF_RESTORE_NUM;
                        pObj->ssend(pObj->priv, data);
                    }
                }
            }
            pObj->recvdIsSend[pObj->curSei] = 1;    //没有丢包，那么数据可以发送
        }
        else {
            pObj->recvdIsSend[pObj->curSei] = 0;    //有丢包，那么数据不能发送，必需解码后才能再发送
        }
        pObj->recvdNum++;//统计分组有效成员个数

        if (!pObj->isDrop && pObj->recvdNum == pObj->k) {
            printf ("recv src sok.......................................\n");
            //没有丢包，且接收到的包刚好等于源分组数据大小，说明当前分组都处理完成
            for (index = 0; index < pObj->k; index++) {
                if (pObj->fp) {
                    fwrite(((RedunBuf *)pObj->recvdValidTab[index])->data, 1, 1024, pObj->fp);
                    printf ("src wirite index:%d ./.................\n", redunBuf->sei);
                }
                pObj->release(pObj->priv, pObj->recvdValidTab[index]);
                pObj->recvdIsSend[index]    = 0;
                pObj->recvdValidTab[index]  = NULL;
            }
            pObj->isDrop        = 0;
            pObj->dropNum       = 0;
            pObj->recvdNum      = 0;

            /* if (!pObj->recvdNum) { */
            pObj->sei           = 0;
            /* pObj->isFirstPkt    = 1; */
            /* } */
        }
    }
    else {
        printf ("[%s %d] why sei:%d pObj->recvdValidTab:%p != NULL\n",
                __func__, __LINE__, pObj->curSei, pObj->recvdValidTab[pObj->curSei]);
    }

    return 0;
}

/*
 *  统计分组打包是否完成，原则上是收到任意4个包即可进行分组恢复
 *  参数1  ---->略
 *  返回值 ---->略
 */
int PreRedunContextOpenFecDecIsComplete(PreRedunManageDecPrivStu *pObj) {
    return pObj->recvdNum == pObj->k ? 0 : -1;
}

/*
 *  分组统计完成，将收到的分组数据通过参数2返回到上层处理
 *  参数1  ---->略
 *  参数2  ---->返回分组数据集合
 *  返回值 ---->略
 */
static int PreRedunContextOpenFecDecCompleted(PreRedunManageDecPrivStu *pObj, 
        RedunBufList *redunBufList) {
    int index               = 0;
    RedunBuf *redunBuf      = NULL;
    redunBufList->numBufs   = 0;

    for (index = 0; index < pObj->n; index++) {
        if (pObj->recvdValidTab[index]) {
            redunBuf            = (RedunBuf *)pObj->recvdValidTab[index];
            if (index < pObj->k) {
                /* redunBuf->dataHead          = pObj->repairHeadSymbolsTab[!pObj->flag][index]; */
            }
            else {
                /* redunBuf->dataHeadRepair    = pObj->repairHeadSymbolsTab[!pObj->flag][index]; */
            }
            redunBufList->redunBuf[redunBufList->numBufs++] = redunBuf;
            printf ("%s %d****** combine  index:%d %s\n", 
                    __func__, __LINE__, index, index < pObj->k ? "src" : "repair");
        }

        //任意分组数据达到K即可
        if (redunBufList->numBufs == pObj->k) {
            break;
        }
    }

    return redunBufList->numBufs == pObj->k ? 0 : -1;
}

/*
 *  分组处理接口，主要是进行收到的数据包分组处理
 *  参数1  ---->略
 *  参数2  ---->当次收到的数据包指针，外部结构
 *  参数3  ---->分组统计完成后, 返回分组数据集合
 *  返回值 ---->分组未完成返回-1，分组完成返回0
 */
int preRedunOpenFecDecProcess(PreRedunContext *oObj, void *data, RedunBufList *redunBufList) {
    RedunBuf *redunBuf              = NULL;
    PreRedunManageDecPrivStu *pObj  = (PreRedunManageDecPrivStu *)oObj->priv;

    PreRedunContextOpenFecDecGetSeqId(pObj, data);

    PreRedunContextOpenFecDecCalcSei(pObj);

    PreRedunContextOpenFecDecClearFrame(pObj);

    redunBuf = PreRedunContextOpenFecDecTransform(pObj, data);
    if (NULL == redunBuf) {
        return -1;
    }

    PreRedunContextOpenFecDecAddFrame(pObj, redunBuf);

     if (!PreRedunContextOpenFecDecIsComplete(pObj)) {
         return PreRedunContextOpenFecDecCompleted(pObj, redunBufList);
     }

     return -1;
}

/*
 *  算法内部有个申请内存接口，因为本模块与之无关联性，无法与之对接，
 *  固提供接口在算法内部申请内存时调用，将申请的内存关联到本模块
 *  参数1  ---->略
 *  参数2  ---->将所有分组序列需要申请内存内存传递集合
 *  返回值 ---->分组未完成返回-1，分组完成返回0
 */
int preRedunOpenFecDecAdd(PreRedunContext *oObj, RedunBufList *redunBufList) {
    int index                       = 0;
    RedunBuf *redunBuf              = NULL;
    PreRedunManageDecPrivStu *pObj  = (PreRedunManageDecPrivStu *)oObj->priv;

    for (index = 0; index < redunBufList->numBufs; index++) {
        redunBuf = redunBufList->redunBuf[index];
        pObj->srcSymbolsTab[redunBuf->sei] = redunBuf;
    }

    return -1;
}

/*
 *  分组处理后，进行一些相关的数据恢复发送及复位处理
 *  参数1  ---->略
 *  返回值 ---->略
 */
int preRedunOpenFecDecProcessAfter(PreRedunContext *oObj) {
    int esi                         = 0;
    void *data                      = NULL;
    RedunBuf *redunBuf              = NULL;
    PreRedunManageDecPrivStu *pObj  = (PreRedunManageDecPrivStu *)oObj->priv;

    pObj->flag = !pObj->flag;

    for (esi = 0; esi < pObj->k; esi++) {
        if (pObj->recvdValidTab[esi]) {
            //没有发送过的，开始发送
            RedunBuf *redunBuf = (RedunBuf *)pObj->recvdValidTab[esi];
            if (!pObj->recvdIsSend[esi]) {
                if (pObj->transformFromRedun) {
                    data = pObj->transformFromRedun(pObj->priv, pObj->recvdValidTab[esi], NULL, NULL, 1);
                    if (data) {
                        if (pObj->ssend) {
                            redunBuf->refId = REDUN_REF_JUST_SEND_NUM;
                            pObj->ssend(pObj->priv, data);
                        }
                    }
                }

                pObj->recvdValidTab[esi]    = NULL;
                pObj->srcSymbolsTab[esi]    = NULL;
            }
            else {
                pObj->release(pObj->priv, pObj->recvdValidTab[esi]);
            }

            pObj->recvdIsSend[esi] = 0;
            pObj->dropNum = (pObj->isDrop & (1 << esi)) ? pObj->dropNum - 1 : pObj->dropNum;
            pObj->isDrop &= ~(1 << esi);
            pObj->recvdNum--;

            if (pObj->fp) {
                fwrite(redunBuf->data, 1, 1024, pObj->fp);
                printf ("wirite index:%d ./.................\n", redunBuf->sei);
            }
        }
        else {
            //这里有丢包，已经恢复，但无法获取到相应的数据
            if (pObj->transformFromRedun) {
                if (pObj->srcSymbolsTab[esi]) {
                    RedunBuf *redunBuf = pObj->srcSymbolsTab[esi];
                    data = pObj->transformFromRedun(pObj->priv, pObj->srcSymbolsTab[esi], NULL, NULL, 1);
                    if (data) {
                        if (pObj->ssend) {
                            redunBuf->refId = REDUN_REF_JUST_SEND_NUM;
                            pObj->ssend(pObj->priv, data);
                        }
                    }

                    if (pObj->fp) {
                        fwrite(redunBuf->data, 1, 1024, pObj->fp);
                        printf ("wirite index:%d ./.................\n", redunBuf->sei);
                    }
                }
            }

            pObj->recvdIsSend[esi]      = 0;
            pObj->recvdValidTab[esi]    = NULL;
            pObj->srcSymbolsTab[esi]    = NULL;
        }
    }

    //说明当前恢复使用的冗余包
    if (pObj->recvdNum > 0) {
        int refNum      = 0;
        int index       = 0;
        int jIndex      = 0;
        int lastIndex   = 0;

        for (index = pObj->k; index < pObj->n; index++) {
            redunBuf                                = (RedunBuf *)pObj->recvdValidTab[index];
            pObj->recvdValidTab[index - pObj->k]    = pObj->recvdValidTab[index];
            pObj->recvdIsSend[index - pObj->k]      = pObj->recvdIsSend[index];
            //清掉冗余部分的存储状态
            pObj->recvdValidTab[index]              = NULL;
            pObj->recvdIsSend[index]                = 0;
            if (pObj->recvdValidTab[index - pObj->k]) {
                redunBuf->sei -= pObj->k;
                if (pObj->transformToRedun) {
                    redunBuf = pObj->transformToRedun(pObj->priv, (void **)&redunBuf, redunBuf->priv, 1);
                }
                //有效数据包的引用计数
                if (refNum == index - pObj->k) {
                    //连续的数据包可以发送了
                    if (pObj->transformFromRedun) {
                        data = pObj->transformFromRedun(pObj->priv, redunBuf, NULL, NULL, 1);
                        if (data) {
                            if (pObj->ssend) {
                                redunBuf->refId = REDUN_REF_RESTORE_NUM;
                                pObj->ssend(pObj->priv, data);
                            }
                        }
                    }
                    pObj->recvdIsSend[index - pObj->k] = 1;
                }
                refNum++;
                lastIndex = index;
            }
        }

        //计算丢包量
        pObj->isDrop    = 0;
        pObj->dropNum   = 0;
        for (jIndex = 0; jIndex < lastIndex - pObj->k; jIndex++) {
            if (NULL == pObj->recvdValidTab[jIndex]) {
                pObj->dropNum++;
                pObj->isDrop |= (1 << jIndex);
            }
        }

        pObj->sei -= pObj->k;
    }

    return 0;
}

static int preRedunOpenFecDecCommand(PreRedunContext *oObj, unsigned cmd, void *value) {
    int index                       = 0;
    int status                      = -1;
    PreRedunManageDecPrivStu *pObj     = (PreRedunManageDecPrivStu *)oObj->priv;

    switch (cmd) {
        case PREREDUN_COMMAND_ISSAVE_FILE:
            {
                break;
            }
        default:
            break;
    }

    return 0;
}

PreRedunManageStu preRedunOpenFecAudDec = {
    .name           = "Pre_OpenFec_Audio_Decode",
    .privSize       = sizeof(PreRedunManageDecPrivStu),
    .iinit          = preRedunOpenFecDecInit,
    .eexit          = preRedunOpenFecDecExit,
    .process        = preRedunOpenFecDecProcess,
    .add            = preRedunOpenFecDecAdd,
    .processafter   = preRedunOpenFecDecProcessAfter,
    .command        = preRedunOpenFecDecCommand,
};

/********************* OpenFec_Audio_Encode ******************/

//冗余预处理结构
typedef struct PreRedunManageEncPrivStu {
    //源数据包个数
    char k; 
    //源数据与冗余包个数总和
    char n;                             
    //源数据大小
    char fillLength;  
    //头数据大小
    char headFillLength;
    //前后分组轮流切换冗余内存
    char flag;              
    //是否是第一个接收到的包
    char isFirstPkt;            
    //上一个包的算法序列号[0-n]
    char sei;             
    //当前接收到一组数据包的个数[< n]
    char recvdNum;                  
    //如果丢包量大于分组总大小(n)，置位，
    //表示当前接收到的是下一个分组的数据，
    //前一个分组数据无法恢复了，直接将未
    //发送的包发送出去并开始下一个分组的处理
    char isEncChange;           
    //当前的序列号
    unsigned seqId; 
    //上一次接收包的RTP序列号
    unsigned lastRecvPktSeq;        
    //是否丢包，第一个位表示对应序列是否丢包 1 << esi
    unsigned isDrop;    
    //分组内丢包个数
    unsigned dropNum;              
    //临时存储接收数据帧的首地址
    void *recvdValidTab[REDUN_MAX_NUM];           
    //存储下的数据帧是否已经发送，是否发送由前面的数据是否有丢帧决定
    unsigned char recvdIsSend[REDUN_MAX_NUM];  
    unsigned char repairSymbolsTab[REDUN_ALT_NUM][REDUN_MAX_NUM][REDUN_MAX_LENGTH];
    unsigned char repairHeadSymbolsTab[REDUN_ALT_NUM][REDUN_MAX_NUM][REDUN_MAX_HEAD_LENGTH];

    //获取序列号接口
    unsigned (*getSeqId)(void *oObj, void *data);
                                    
    //转换内部数据结构到外部数据结构
    void *(*transformFromRedun)(void *oObj, void *data, void *repair, void *headRepair, int isSrc);

    //转换外部数据结构到内部数据结构
    void *(*transformToRedun)(void *oObj, void **redunBuf, void *data, int isSrc);

    //发送数据接口
    int (*ssend)(void *oObj, void *data);

    /* void *(*request)(void *oObj, unsigned size, unsigned esi);       //申请源数据内存回调,音频模块不需要该接口，视频需要提供 */

    //释放数据接口
    int (*release)(void *oObj, void *redunBuf);

    //内部结构
    QueHndl hndlE;
    RedunBuf redunBuf[REDUN_BUFFER_NUM];

    //指向回调接口对接的外部句柄
    void *priv;
} PreRedunManageEncPrivStu ;

/*
 *  分组管理组件初始化接口
 */
int preRedunOpenFecEncInit(PreRedunContext *oObj, void *config) {
    int ret                             = -1;
    int index                           = 0;
    PreRedunManageEncPrivStu *pObj      = (PreRedunManageEncPrivStu *)oObj->priv;
    PreRedunManageEncParmas *params     = (PreRedunManageEncParmas *)config;

    memset(pObj, 0x0, sizeof(*pObj));

    printf ("%s %d input check k:%d n:%d priv:%p "
            "fillLength:%d headFillLength:%d "
            "transformFromRedun:%p "
            "transformToRedun:%p "
            "ssend:%p release:%p "
            "getSeqId:%p\n", 
            __func__, __LINE__, 
            params->k, 
            params->n, 
            params->priv, 
            params->fillLength, 
            params->headFillLength,
            params->transformFromRedun, 
            params->transformToRedun, 
            params->ssend, 
            params->release, 
            params->getSeqId);

    if (params->fillLength % 2 != 0 
            || params->headFillLength % 2 != 0) {
        printf ("[%s %d] fillLength:%d headFillLength:%d not match[len mod 2 == 0]\n", 
                __func__, __LINE__, params->fillLength, params->headFillLength);
        goto ERR0;
    }

    if (NULL == params->transformFromRedun 
            || NULL == params->transformToRedun 
            || NULL == params->ssend 
            || NULL == params->release
            || NULL == params->getSeqId) {
        printf ("[%s %d]input callback invalid[%d-%d-%d-%d-%d]\n",
                __func__, __LINE__, 
                NULL == params->transformFromRedun, 
                NULL == params->transformToRedun, 
                NULL == params->ssend, 
                NULL == params->release, 
                NULL == params->getSeqId);
        goto ERR0;
    }

    pObj->k                             = params->k;
    pObj->n                             = params->n;
    pObj->fillLength                    = params->fillLength;
    pObj->headFillLength                = params->headFillLength;
    pObj->priv                          = params->priv;

    pObj->transformFromRedun            = params->transformFromRedun;
    pObj->transformToRedun              = params->transformToRedun;
    pObj->ssend                         = params->ssend;
    pObj->release                       = params->release;
    pObj->getSeqId                      = params->getSeqId;

    ret = QueueCreate(&pObj->hndlE, 1000);
    if (ret) {
        printf ("[%s %d]QueueCreate pObj->hndlE failure\n", __func__, __LINE__);
        goto ERR0;
    }

    for (index = 0; index < REDUN_BUFFER_NUM; index++) {
        pObj->redunBuf[index].handle = (void *)&pObj->hndlE;
        ret = QueuePut(&pObj->hndlE, (long long)&pObj->redunBuf[index], TIMEOUT_NONE);
        if (ret) {
            printf ("[%s %d] QueuePut hndlE index:%d failure\n", __func__, __LINE__, index);
        }
    }

    return 0;
ERR0:
    return -1;
}

/*
 *  分组管理组件退出接口
 */
int preRedunOpenFecEncExit(PreRedunContext *oObj) {
    PreRedunManageEncPrivStu *pObj      = (PreRedunManageEncPrivStu *)oObj->priv;

    QueueDelete(&pObj->hndlE);

    return 0;
}

static void *PreRedunContextOpenFecEncGetSeqId(PreRedunManageEncPrivStu *pObj, void *data) {
    if (pObj->getSeqId) {
        pObj->seqId = pObj->getSeqId(pObj->priv, data);
    }
}

/*
 *  对接收到的数据包进行内部数据结构转换处理
 *  参数1  ---->略
 *  参数2  ---->外部接收数据结构
 *  返回值 ---->转换后的内部结构指针
 */
static RedunBuf *PreRedunContextOpenFecEncTransform(PreRedunManageEncPrivStu *pObj, void *data) {
    int ret             = -1;
    RedunBuf *redunBuf  = NULL;

    ret = QueueGet(&pObj->hndlE, (long long *)&redunBuf, TIMEOUT_NONE);
    if (ret) {
        printf ("%s %d QueuGet failure\n", __func__, __LINE__);
        goto ERR0;
    }

    //将收到的数据包将换成冗余器内部数据结构,
    if (pObj->transformToRedun) {
        return pObj->transformToRedun(pObj->priv, (void **)&redunBuf, data, 1);
    }

ERR0:
    return NULL;
}

/*
 *  将收到的转换后的数据包存储起来,编码端收到的数据可以直接发送了
 *  参数1  ---->略
 *  参数2  ---->转换后的数据包地址
 *  返回值 ---->略
 */
static int PreRedunContextOpenFecEncAddFrame(PreRedunManageEncPrivStu *pObj, RedunBuf *redunBuf) {
    int index           = 0;
    void *data          = NULL;

    if (NULL == pObj->recvdValidTab[pObj->sei]) {
        pObj->recvdValidTab[pObj->sei] = (void *)redunBuf;//将数据包临时存储下来
        if (pObj->sei < pObj->k) {
            if (pObj->transformFromRedun) {
                data = pObj->transformFromRedun(pObj->priv,
                        pObj->recvdValidTab[pObj->sei], 
                        pObj->repairSymbolsTab[pObj->flag][pObj->sei],
                        pObj->repairHeadSymbolsTab[pObj->flag][pObj->sei], 1);
                if (data) {
                    if (pObj->ssend) {
                        //提前发送的数据包，引用计数为2，因为需要临时保存
                        redunBuf->refId = REDUN_REF_RESTORE_NUM;
                        pObj->ssend(pObj->priv, data);
                    }
                }
                else {
                    printf ("priv is NULL sei:%d\n", pObj->sei);
                }
            }
        }
        //统计分组有效成员个数
        pObj->recvdNum++;
        //指定源数据头部
        /* redunBuf->dataHead  = pObj->repairHeadSymbolsTab[!pObj->flag][pObj->sei]; */
        redunBuf->sei       = pObj->sei++;
    }
    else {
        printf ("[%s %d] why sei:%d pObj->recvdValidTab:%p != NULL\n",
                __func__, __LINE__, pObj->sei, pObj->recvdValidTab[pObj->sei]);
    }

    return 0;
}

/*
 *  统计分组打包是否完成，编码端收到四个包即可进行编码冗余
 *  参数1  ---->略
 *  返回值 ---->略
 */
int PreRedunContextOpenFecEncIsComplete(PreRedunManageEncPrivStu *pObj) {
    return pObj->recvdNum == pObj->k ? 0 : -1;
}

/*
 *  分组统计完成，将收到的分组数据通过参数2返回到上层处理
 *  参数1  ---->略
 *  参数2  ---->返回分组数据集合
 *  返回值 ---->略
 */
static int PreRedunContextOpenFecEncCompleted(PreRedunManageEncPrivStu *pObj, 
        RedunBufList *redunBufList) {
    int ret                 = -1;
    int index               = 0;
    void *data              = NULL;
    RedunBuf *redunBuf      = NULL;
    redunBufList->numBufs   = 0;

    for (index = 0; index < pObj->k; index++) {
        if (pObj->recvdValidTab[index]) {
            redunBufList->redunBuf[redunBufList->numBufs++] = (RedunBuf *)pObj->recvdValidTab[index];
        }

        if (redunBufList->numBufs == pObj->k) {
            break;
        }
    }

    //冗余数据包申请
    for (index = 0; index < pObj->k; index++) {
        /*获取一个内部数据管理结构*/
        ret = QueueGet(&pObj->hndlE, (long long *)&redunBuf, TIMEOUT_NONE);
        if (ret) {
            printf ("[%s %d]QueueGet hndlE failure\n", __func__, __LINE__);
            goto ERR0;
        }

#if 0
        if (pObj->request) {
            //如果需要向外部申请内存，需要实现该接口
            data = pObj->request(pObj->priv, pObj->fillLength, pObj->sei + index);
            if (NULL != data) {
                if (pObj->transformToRedun) {
                    /* redunBuf = pObj->transformToRedun(pObj->priv, data, 0); */
                }
            }

            pObj->recvdValidTab[pObj->k + index] = (void *)redunBuf;
            /* redunBufList->redunBuf[redunBufList->numBufs]               = redunBuf; */
            /* redunBufList->redunBuf[redunBufList->numBufs]->sei          = pObj->k + index; */
            /* redunBufList->redunBuf[redunBufList->numBufs++]->data       = redunBuf->data; */
        }
        else
#endif
        {
            //使用内部内存做冗余包数据存储
            pObj->recvdValidTab[pObj->k + index]                            = (void *)redunBuf;
            redunBufList->redunBuf[redunBufList->numBufs]                   = redunBuf;
            redunBufList->redunBuf[redunBufList->numBufs]->sei              = pObj->k + index;
            redunBufList->redunBuf[redunBufList->numBufs]->data             = pObj->repairSymbolsTab[!pObj->flag][index];//invalid
            redunBufList->redunBuf[redunBufList->numBufs]->dataRepair       = pObj->repairSymbolsTab[!pObj->flag][index];//valid
            redunBufList->redunBuf[redunBufList->numBufs]->dataHead         = pObj->repairHeadSymbolsTab[!pObj->flag][index];//invalid
            redunBufList->redunBuf[redunBufList->numBufs++]->dataHeadRepair = pObj->repairHeadSymbolsTab[!pObj->flag][index];//valid
        }
    }

    return redunBufList->numBufs == pObj->n ? 0 : -1;
ERR0:
    for (index = pObj->k; index < pObj->n; index++) {
        if (pObj->recvdValidTab[index]) {
            redunBuf = (RedunBuf *)pObj->recvdValidTab[index];
            QueuePut((QueHndl *)redunBuf->handle, (long long)redunBuf, TIMEOUT_NONE);
            pObj->recvdValidTab[index] = NULL;
        }
    }

    return -1;
}


/*
 *  分组处理接口，主要是进行收到的数据包分组处理
 *  参数1  ---->略
 *  参数2  ---->当次收到的数据包指针，外部结构
 *  参数3  ---->分组统计完成后, 返回分组数据集合
 *  返回值 ---->分组未完成返回-1，分组完成返回0
 */
int preRedunOpenFecEncProcess(PreRedunContext *oObj, void *data, RedunBufList *redunBufList) {
    RedunBuf *redunBuf              = NULL;
    PreRedunManageEncPrivStu *pObj  = (PreRedunManageEncPrivStu *)oObj->priv;

    //编码端序列号是否用处不大
    PreRedunContextOpenFecEncGetSeqId(pObj, data);

    redunBuf = PreRedunContextOpenFecEncTransform(pObj, data);
    if (NULL == redunBuf) {
        return -1;
    }

    PreRedunContextOpenFecEncAddFrame(pObj, redunBuf);

     if (!PreRedunContextOpenFecEncIsComplete(pObj)) {
         return PreRedunContextOpenFecEncCompleted(pObj, redunBufList);
     }

     return -1;
}

/*
 *  分组处理后，进行一些相关的数据策略发送及复位处理
 *  参数1  ---->略
 *  返回值 ---->略
 */
int preRedunOpenFecEncProcessAfter(PreRedunContext *oObj) {
    int ret                         = -1;
    int esi                         = 0;
    void *data                      = NULL;
    RedunBuf *redunBuf              = NULL;
    PreRedunManageEncPrivStu *pObj  = (PreRedunManageEncPrivStu *)oObj->priv;
    
    for (esi = 0; esi < pObj->n; esi++) {
        if (pObj->recvdValidTab[esi]) {
            redunBuf = (RedunBuf *)pObj->recvdValidTab[esi];
            if (esi < pObj->k) {
                //数据包释放
                pObj->release(&pObj->priv, pObj->recvdValidTab[esi]);
            }
            else {
                //音频模块冗余包用的是内部数据包，不需要回调释放
                QueuePut(&pObj->hndlE, (long long)pObj->recvdValidTab[esi], TIMEOUT_NONE);
            }

            pObj->recvdValidTab[esi] = NULL;
        }
    }

    /*发送冗余数据，如果需要发送*/


    //复位部分字段
    pObj->recvdNum  = 0;
    pObj->sei       = 0;
    pObj->flag      = !pObj->flag;

    return 0;
}

static int preRedunOpenFecEncCommand(PreRedunContext *oObj, unsigned cmd, void *value) {
    int index                       = 0;
    int status                      = -1;
    PreRedunManageEncPrivStu *pObj     = (PreRedunManageEncPrivStu *)oObj->priv;

    switch (cmd) {
        case PREREDUN_COMMAND_ISSAVE_FILE:
            {
                break;
            }
        default:
            break;
    }

    return 0;
}


PreRedunManageStu preRedunOpenFecAudEnc = {
    .name           = "Pre_OpenFec_Audio_Encode",
    .privSize       = sizeof(PreRedunManageEncPrivStu),
    .iinit          = preRedunOpenFecEncInit,
    .eexit          = preRedunOpenFecEncExit,
    .process        = preRedunOpenFecEncProcess,
    .processafter   = preRedunOpenFecEncProcessAfter,
    .command        = preRedunOpenFecEncCommand,
};
