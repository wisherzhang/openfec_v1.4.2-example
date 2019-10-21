#define OF_USE_ENCODER

#include "redun_interface.h"
#include <pthread.h>
#include <signal.h>
#include <execinfo.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>




typedef struct tagRtpHead
{
    unsigned ulSeq:16;      /* RTP序列号 */
    unsigned bit7PT:7;      /* 负载类型 */
    unsigned bit1M:1;       /* RTP结尾标志 */
    unsigned bit4CC:4;      /* CRC清单计数器 */
    unsigned bit1X:1;       /* RTP包头扩展标志位 */
    unsigned bit1P:1;       /* 协议子集标志位 */
    unsigned bit2V:2;       /* RTP版本协议号 */
    unsigned ulTimeStamp;   /* 编码时戳 */
    unsigned ulSSRC;        /* 同步源标志 */
} RtpHeadDef;

typedef struct _tagSrcRTPHead{
    unsigned seq:16;        //当前分组中正常包seq rs运算后的结果
    unsigned mark:1;        //当前分组中正常包mark位 rs运算后的结果
    unsigned payloadLen:15; //当前分组中正常包payloadLen rs运算后的结果
    unsigned timestamp;     //当前分组中正常包timestamp位 rs运算后的结果
} SrcRTPHead;

typedef struct _tagRedunRTPHead{
    unsigned    sei;         //标志当前是否为冗余包
    unsigned    version;     //当前使用FEC模块的版本信息 
    unsigned    mould;       //矩阵模型
    unsigned    bminus;      //分组起始序号与当前包序号的差值
    SrcRTPHead  srcHead;
} RedunRTPHead;

typedef struct {
    int k;
    int n;
    int fillLength;
    int headFillLength;

    unsigned timestamp;
    unsigned short seqId;

    of_codec_id_t	codec_id;
    QueHndl hndlE;
    QueHndl hndlF;

    serverClientBuf buffer[30];

    QueHndl clientHndlE;
    QueHndl clientHndlF;

    serverClientBuf bufferClient[30];

    FILE *fpIn;
    FILE *fpOut;

    RedunServerObj encode;
    RedunClientObj decode;

    pthread_mutex_t mutex;
} server_client;

unsigned sendNum = 0;
unsigned recvNum = 0;
#define POSTION_BYTE  (sizeof(RtpHeadDef))

#define _Crc32Expression 0x04C10DB7
#define _Crc32TableSize  256
extern void *OpenFecClientRequestSrc(void *oObj, unsigned size, unsigned esi);
extern void *myclienttransform(void *oObj, void *data, void *repair, void *headRepair, int isSrc);
extern void *myservertransform(void *oObj, void *data, void *repair, void *headRepair, int isSrc);
extern void *myservertransromtoredun(void *oObj, void **redunBuf, void *data, /* void *dataHead,  */int isSrc);
extern void *myclienttransromtoredun(void *oObj, void **redunBuf, void *data, /* void *dataHead,  */int isSrc);
extern int mysend(void *oObj, void *data);
extern int mysersend(void *oObj, void *data);
extern int myserverrelease(void *oObj, void *data);
extern int myclientrelease(void *oObj, void *data);
extern unsigned myservergetseqid(void *oObj, void *data);
extern unsigned myclientgetseqid(void *oObj, void *data);

static inline int _CreateCrc32Table(unsigned crcTable[], unsigned aPoly) {

    unsigned nData;
    unsigned nAccum;
    int i, j;

    for (i = 0; i < _Crc32TableSize; i++ ) {

        nData = (unsigned)( i << 24 );
        nAccum = 0;
        for (j = 0; j < 8; j++ ) {

            if ((nData ^ nAccum) & 0x80000000) nAccum = (nAccum << 1) ^ aPoly;
            else nAccum <<= 1;
            nData <<= 1;

        }
        crcTable[i] = nAccum;

    }

    return 0;

}

static inline unsigned _GetCrc32Val(unsigned char *data, unsigned dataLen) {

    unsigned crcTable[_Crc32TableSize] = {
        0
    };
    unsigned nAccum = 0;
    unsigned i = 0;
    _CreateCrc32Table(crcTable, _Crc32Expression);
    for (i = 0; i < dataLen; i++)
        nAccum = (nAccum << 8) ^ crcTable[(nAccum >> 24) ^ *data++];

    return nAccum;

}

int Simple_RedunServerBuildRTP(server_client *oObj, serverClientBuf *elem) {
    int ret                 = -1;
    RtpHeadDef *rtpHead     = (RtpHeadDef *)(elem->data);
    void *payload           = (char *)elem->data + sizeof(RtpHeadDef);

    //填充RTP字段
    memset(rtpHead, 0x0, sizeof(RtpHeadDef));
    rtpHead->ulSeq          = oObj->seqId++;
    rtpHead->ulTimeStamp    = oObj->timestamp;
    rtpHead->bit1M          = 1;
    /* rtpHead->bit4CC         = 0; */
    oObj->timestamp         += 960;

    //填充Payload字段
    ret = fread(payload, 1, oObj->fillLength, oObj->fpIn);
    if (ret != oObj->fillLength) {
        fseek(oObj->fpIn, 0, SEEK_SET);
        ret = fread(payload, 1, oObj->fillLength, oObj->fpIn);
    }

    elem->seqId             = rtpHead->ulSeq;

    return 0;
}

void *server_callback(void *args) {
    int i = 0;
    int ret = -1;
    int flag = 0;
    server_client *inst = (server_client *)args;
    RedunServerObj *pObj = (RedunServerObj *)&inst->encode;
    serverClientBuf *pkt_with_fpi = NULL;
    unsigned seqId3 = 0;

    while (1) {

        pthread_mutex_lock(&inst->mutex);
        for (i = 0; i < inst->k; i++) {
            printf ("starting ...LLLLLLLLLLLLLLLLLLLLLL server queNum:F %d queu:E %d client F:%d E:%d\n", 
                    QueueGetQueuedCount(&inst->hndlF), 
                    QueueGetQueuedCount(&inst->hndlE), 
                    QueueGetQueuedCount(&inst->clientHndlF), 
                    QueueGetQueuedCount(&inst->clientHndlE));
            int ret = QueueGet(&inst->hndlE, (long long *)&pkt_with_fpi, TIMEOUT_NONE);
            if (!ret) {
                Simple_RedunServerBuildRTP(inst, pkt_with_fpi);

                ret = RedunServerRun(pObj, pkt_with_fpi);
                printf ("ending ...LLLLLLLLLLLLLLLLLLLLLL server queNum:F %d queu:E %d client F:%d E:%d\n", 
                        QueueGetQueuedCount(&inst->hndlF), 
                        QueueGetQueuedCount(&inst->hndlE), 
                        QueueGetQueuedCount(&inst->clientHndlF), 
                        QueueGetQueuedCount(&inst->clientHndlE));
            }
        }
        pthread_mutex_unlock(&inst->mutex);

        usleep(1000000);
    }

    return NULL;
}

/************************************Client***********************/


void *client_callback(void *args) {
    int i = 0;
    int status = -1;
    int ret = -1;
    int esi = -1;
    server_client *inst = (server_client *)args;
    RedunClientObj *pObj = (RedunClientObj *)&inst->decode;
    void *pkt_with_fpi  = NULL;
    unsigned processNum = 0;
    unsigned dropNum = 0;
    unsigned preDropNum = 0;
    srand(time(NULL));

    while (1) {
        pthread_mutex_lock(&inst->mutex);
        status = QueueGet(&inst->clientHndlF, (long long *)&pkt_with_fpi, TIMEOUT_NONE);
        if (!status) {
            //不能连续分组出现丢3个包的情况，这样就没办法恢复了
            //所以前一组丢包为3时，当前分组强制丢包为1
            /*
             * if (processNum % 4 == 0) {
             *     dropNum = rand() % 5;
             *     //不能连续出现丢包超过5个的，不然总有一组没法恢复
             *     if (preDropNum == 3) {
             *         if (dropNum > 1) 
             *             dropNum = 1;
             *     }
             *     else if (preDropNum == 2) {
             *         if (dropNum > 2) {
             *             dropNum = 2;
             *         }
             *     }
             *     else if (preDropNum == 4) {
             *         dropNum = 0;
             *     }
             *     printf ("---------------------->>>> current group processNum:%d drop %d\n", processNum, dropNum);
             *     RedunClientCommand(pObj, PREREDUN_COMMAND_DROP_PACKAGE, &dropNum);
             *     preDropNum = dropNum;
             * }
             * processNum++;
             */
            printf ("starting CCCCCCCCCCCCCCCC clientF:%d E:%d\n", 
                    QueueGetQueuedCount(&inst->clientHndlF), 
                    QueueGetQueuedCount(&inst->clientHndlE));
            RedunClientRun(pObj, pkt_with_fpi);
            printf ("ending CCCCCCCCCCCCCCCC clientF:%d E:%d\n", 
                    QueueGetQueuedCount(&inst->clientHndlF), 
                    QueueGetQueuedCount(&inst->clientHndlE));
        }
        pthread_mutex_unlock(&inst->mutex);
        usleep(20000);
    }

    return NULL;
}

/************************************Server***********************/

int32_t Simple_RedunServerObj(server_client *inst, const char *input) {
    unsigned		esi;
    unsigned		esj;
    unsigned		i;
    unsigned		ret		= -1;
    RedunServerObj *pObj;

    pObj = &inst->encode;

    inst->fpIn = fopen(input, "r");
    if (NULL == inst->fpIn) {
        printf ("fopen file :%s failure\n", input);
        goto ERR0;
    }

    ret = RedunServerInit(pObj, inst->k, inst->n, inst->fillLength, 24,
            inst->codec_id, (void *)inst, 
            myservertransromtoredun, 
            myservertransform, 
            myserverrelease, 
            mysersend, 
            myservergetseqid);
    if (ret) {
        printf ("RedunServerInit failure?\n");
        goto ERR1;
    }

    pthread_t tid = 0;
    if ((pthread_create(&tid, NULL, server_callback, (void *)inst)) != 0) {
        OF_PRINT_ERROR(("create pthtread failure, n=%u)\n", inst->n))
            ret = -1;
        goto ERR2;
    }

    return 0;
ERR2:
    /* RedunServerExit(pObj); */
ERR1:
    fclose(inst->fpIn);
ERR0:
    return -1;
}

void *OpenFecClientRequestSrc(void *oObj, unsigned size, unsigned esi) {
    serverClientBuf *pkt_with_fpi      = NULL;
    RedunBuf *redunBuf      = NULL;
    RedunClientObj *decode  = (RedunClientObj *)oObj;
    server_client *server   = (server_client *)decode->priv;
    RedunServerObj *encode  = (RedunServerObj *)&server->encode;

    int ret = QueueGet(&server->clientHndlE, (long long *)&pkt_with_fpi, TIMEOUT_NONE);
    if (ret) {
        printf ("QueueGet decode->hndlE failure? esi:%d\n", esi);
        return NULL;
    }

    memset((char *)pkt_with_fpi->data, 0x0, decode->fillLength * 3);

    return pkt_with_fpi;
}



int32_t Simple_RedunClientObj(server_client *inst, const char *output) {
    unsigned		esi;
    unsigned		i;
    unsigned		ret		= -1;
    RedunClientObj *pObj;

    pObj = &inst->decode;

    inst->fpOut = fopen(output, "w+");
    if (NULL == inst->fpOut) {
        printf ("fopen file %s failure\n", output);
        goto ERR0;
    }

    ret = RedunClientInit(pObj, inst->k, inst->n, inst->fillLength, inst->headFillLength,
            inst->codec_id, (void *)inst, 
            OpenFecClientRequestSrc, 
            OpenFecClientRequestSrc, 
            myclienttransromtoredun, 
            myclienttransform, 
            myclientrelease, 
            mysend, 
            myclientgetseqid);
    if (ret) {
        printf ("OpenFecClientInit failure?\n");
        goto ERR1;
    }

    pthread_t tid = 0;
    if ((pthread_create(&tid, NULL, client_callback, (void *)inst)) != 0) {
        printf ("create pthtread failure, n=%u)\n", inst->n);
        ret = -1;
        exit(-1);
    }

    return 0;
ERR1:
    fclose(inst->fpOut);
ERR0:
    return -1;
}

int32_t Simple_sys(server_client *inst, int n, int k,
        int fillLength, int headFillLength, const char *input, const char *output) {
    int num             = 30;
    int index           = 0;

    inst->n             = n; 
    inst->k             = k;
    inst->fillLength    = fillLength;
    inst->headFillLength= headFillLength;
    inst->codec_id      = OF_CODEC_REED_SOLOMON_GF_2_8_STABLE;
    /* inst->codec_id      = OF_CODEC_REED_SOLOMON_GF_2_M_STABLE; */

    if (QueueCreate(&inst->hndlE, 1000)) {
        printf(".........s1111\n");
        exit(-1);
    }

    if (QueueCreate(&inst->hndlF, 1000)) {
        printf(".........s2222\n");
        exit(-1);
    }

    if (QueueCreate(&inst->clientHndlE, 1000)) {
        printf(".........s1111\n");
        exit(-1);
    }

    if (QueueCreate(&inst->clientHndlF, 1000)) {
        printf(".........s2222\n");
        exit(-1);
    }

    printf ("ONE hndle:%p hndlF:%p clientHndlE:%p clientHndlF:%p\n", 
            &inst->hndlE, &inst->hndlF, &inst->clientHndlE, &inst->clientHndlF);

    for (index = 0; index < num; index++) {
        void *sample_mem = (void *)malloc (fillLength * 3);
        inst->buffer[index].data = sample_mem;
        inst->buffer[index].priv = NULL;
        printf ("data index:%d %p\n", index, sample_mem);
        RtpHeadDef *rtpHead = (RtpHeadDef *)sample_mem;
        memset(rtpHead, 0x0, sizeof(RtpHeadDef));
        int ret = QueuePut(&inst->hndlE, (long long)&inst->buffer[index], TIMEOUT_NONE);
        if (ret) {
            printf ("QueuePut failure index:%d\n", index);
        }
    }

    for (index = 0; index < num; index++) {
        void *sample_mem = (void *)malloc (fillLength * 3);
        inst->bufferClient[index].data = sample_mem;
        inst->bufferClient[index].priv = NULL;
        int ret = QueuePut(&inst->clientHndlE, (long long)&inst->bufferClient[index], TIMEOUT_NONE);
        if (ret) {
            printf ("QueuePut failure index:%d\n", index);
        }
    }

    pthread_mutex_init(&inst->mutex, NULL);
    printf ("SLLLLLLLLLLLLLLLLLSSSSSSSSSSSS encoe:%p decode:%p\n", 
            &inst->encode, &inst->decode);
    printf ("SLLLLLLLLLLLLLLLLLSSSSSSSSSSSS encoe:%p\n", 
            &inst->hndlF);


    Simple_RedunServerObj(inst, input);
    Simple_RedunClientObj(inst, output);

    return 0;
}

void printBacktrace(int no) {

    const char *errmsg = "unknown";
    int i = 0;
    int size = 0;
    char **strframe = NULL;
#define btSIZE 30
    void* buffer[btSIZE] = {
        0
    };

    switch (no) {

        case SIGSEGV: errmsg = "Segment Fault"; break;
        case SIGHUP: errmsg =  "SIGHUP"; break;
        case SIGQUIT: errmsg = "SIGQUIT"; break;
        case SIGSYS: errmsg =  "SIGSYS"; break;
        case SIGILL: errmsg =  " Illegal code"; break;
        case SIGABRT: errmsg = "send exit command by abort"; break;
        case SIGFPE: errmsg =  " float abort"; break;
        case SIGKILL: errmsg = "AEF Kill signal"; break;
        case SIGALRM: errmsg = "alarm signal"; break;
        case SIGTERM: errmsg = "Terminal signal"; break;
        case SIGSTOP: errmsg = "DEF terminal process"; break;
        default: {
                     printf("undefine sig no = %d ", no); 
                 } break;
    }

    printf ("Media recive  signal %s ,SigNO %d Local_CPU_Id: %d ", errmsg, no, 0);

    size = backtrace(buffer, btSIZE);
    strframe = (char **)backtrace_symbols(buffer, size);
    printf ("backtrace size = %d , strframe: %p", size, strframe);
    for (i = 0; i < size; i++)
    {

        printf ("frame %d: %s\n", i, strframe[i]);

    }
    if (strframe) {

        free(strframe);
        strframe = NULL;

    }
}//lint !e438

void SigSegFault(int no) {

    const char *errmsg = "unknown";
    char tname[16] = {
        0
    };
    switch (no) {

        case SIGSEGV: errmsg = "Segment Fault"; break;
        case SIGFPE: errmsg = "div 0"; break;
        default: break;

    }

    printf ("SigNO %d %s In TID %d ", no, errmsg, ( int)syscall(224));//gettid());
    if (0 == prctl(PR_GET_NAME, tname))
        printf ("Tname %s ", tname);

    printBacktrace(no);

    // exit(0);
    if (SIG_ERR == signal(no, NULL)) {
        /* de-register signal handle, make system to handle it */
        printf("signal %d fail\r\n", no);

    }
}

void *myclienttransform(void *oObj, void *data, void *repair, void *headRepair, int isSrc) {
    RedunBuf *redunBuf = (RedunBuf *)data;

    return redunBuf->priv;
}

void *myservertransform(void *oObj, void *data, void *repair, void *headRepair, int isSrc) {
    //音频接口不需要考虑源数据包与冗余包，都是冗余包，所以最后一个参数不用考虑，视频需要考虑
    RedunBuf *redunBuf          = (RedunBuf *)data;
    serverClientBuf *userBuf    = (serverClientBuf *)redunBuf->priv;
    server_client *pObj        = (server_client *)oObj;
    RtpHeadDef *rtpHead         = (RtpHeadDef *)userBuf->data;
    void *payload               = (void *)((char *)userBuf->data + sizeof(RtpHeadDef));
    RedunRTPHead *redunRtpHead  = (RedunRTPHead *)((char *)userBuf->data + sizeof(RtpHeadDef) + pObj->fillLength);
    void *redunPayload          = (void *)((char *)userBuf->data + sizeof(RtpHeadDef) + pObj->fillLength + sizeof(RedunRTPHead));

    //data repair
    memcpy(redunPayload, repair, pObj->fillLength);

    //data head repair
    memcpy(redunRtpHead, headRepair, sizeof(RedunRTPHead));

    return redunBuf->priv;
}

void *myservertransromtoredun(void *oObj, void **redunBuf, void *data, /* void *dataHead,  */int isSrc) {
    server_client *pObj             = (server_client *)oObj;
    RedunBuf *inBuf                 = (RedunBuf *)(*((RedunBuf **)redunBuf));
    serverClientBuf *pkt_with_fpi   = (serverClientBuf *)data;
    RedunRTPHead *redunRtpHead      = NULL;//(RedunRTPHead *)dataHead;
    RtpHeadDef *rtpHead             = (RtpHeadDef *)pkt_with_fpi->data;
    
    pkt_with_fpi->priv      = inBuf;
    inBuf->priv             = (void *)data;
    inBuf->data             = (char *)pkt_with_fpi->data + POSTION_BYTE;
    inBuf->dataHead         = (char *)pkt_with_fpi->data + POSTION_BYTE + sizeof(RedunRTPHead) + pObj->fillLength * 2;

    inBuf->refId            = 0;

    redunRtpHead = (RedunRTPHead *)inBuf->dataHead;

    if (isSrc) {
        memset(redunRtpHead, 0x0, sizeof(RedunRTPHead));
        redunRtpHead->srcHead.seq           = rtpHead->ulSeq;
        redunRtpHead->srcHead.timestamp     = rtpHead->ulTimeStamp;
        redunRtpHead->srcHead.mark          = rtpHead->bit1M;
        redunRtpHead->srcHead.payloadLen    = 1024;
    }

    return inBuf;
}

void *myclienttransromtoredun(void *oObj, void **redunBuf, void *data, /* void *dataHead,  */int isSrc) {
    server_client  *pObj            = (server_client  *)oObj;
    RedunBuf *inBuf                 = (RedunBuf *)(*((RedunBuf **)redunBuf));
    serverClientBuf *pkt_with_fpi   = (serverClientBuf *)data;
    RedunRTPHead *redunRtpHead      = NULL;//(RedunRTPHead *)dataHead;
    RtpHeadDef *rtpHead             = (RtpHeadDef *)pkt_with_fpi->data;


    pkt_with_fpi->priv              = inBuf;
    inBuf->priv                     = (void *)data;
    inBuf->refId                    = 0;

    if (isSrc) {
        //是原始包
        inBuf->data             = (char *)pkt_with_fpi->data + POSTION_BYTE;
        inBuf->dataHead         = (char *)pkt_with_fpi->data + POSTION_BYTE + sizeof(RedunRTPHead) + pObj->fillLength * 2;
    }
    else {
        //是冗余包
        inBuf->dataRepair       = (char *)pkt_with_fpi->data + POSTION_BYTE + sizeof(RedunRTPHead) + pObj->fillLength;
        inBuf->dataHeadRepair   = (char *)pkt_with_fpi->data + POSTION_BYTE + pObj->fillLength;
    }


    if (isSrc) {
        redunRtpHead = (RedunRTPHead *)inBuf->dataHead;

        memset(redunRtpHead, 0x0, sizeof(RedunRTPHead));
        redunRtpHead->srcHead.seq         = rtpHead->ulSeq;
        redunRtpHead->srcHead.timestamp   = rtpHead->ulTimeStamp;
        redunRtpHead->srcHead.mark         = rtpHead->bit1M;
        redunRtpHead->srcHead.payloadLen    = 1024;
    }

    return inBuf;
}

int myclientrelease(void *oObj, void *data) {
    server_client *pInst    = (server_client *)oObj;

    return QueuePut(&pInst->clientHndlE, (long long)data, TIMEOUT_NONE);
}

int myserverrelease(void *oObj, void *data) {
    server_client *pInst    = (server_client *)oObj;

    return QueuePut(&pInst->hndlE, (long long)data, TIMEOUT_NONE);
}

unsigned myservergetseqid(void *oObj, void *data) {
    serverClientBuf *userBuf    = (serverClientBuf *)data;
    RtpHeadDef *rtpHead         = userBuf->data;

    return rtpHead->ulSeq;
}

unsigned myclientgetseqid(void *oObj, void *data) {
    serverClientBuf *userBuf = (serverClientBuf *)data;
    RtpHeadDef *rtpHead         = userBuf->data;

    return rtpHead->ulSeq;
}

int mysend(void *oObj, void *data) {
    serverClientBuf *pBuf = (serverClientBuf *)data;
    server_client *pObj    = (server_client *)oObj;
    RedunBuf *redunBuf = (RedunBuf *)pBuf->priv;

    static FILE *fp = NULL;
    if (NULL == fp) {
        fp = fopen("./mysenddest.pcm", "w+");
    }

    if (fp) {
        fwrite(redunBuf->data, 1, 1024, fp);
    }

    return RedunClientRecyleRedunBuf(data);
}

int mysersend(void *oObj, void *data) {
    int ret                 = -1;
    serverClientBuf *pkt_with_fpi      = data;//pBuf->priv;
    serverClientBuf *clientData        = NULL;
    server_client *pInst    = (server_client *)oObj;
    RedunServerObj *pObj     = (RedunServerObj *)&pInst->decode;
    RedunBuf *src           = (RedunBuf *)pkt_with_fpi->data;
    RedunBuf *dest          = NULL;

    static unsigned processNum = 0;
    static unsigned dropNum = 0;
    static unsigned preDropNum = 0;
    static unsigned currentDropNum = 0;
    static unsigned test1 = 0;
    if (processNum == 0) {
        srand(time(NULL));
    }

    //不能连续分组出现丢3个包的情况，这样就没办法恢复了
    //所以前一组丢包为3时，当前分组强制丢包为1
    if (processNum % 4 == 0) {
        dropNum = rand() % 5;
        //不能连续出现丢包超过5个的，不然总有一组没法恢复
        if (preDropNum == 3) {
            if (dropNum > 1) 
                dropNum = 1;
        }
        else if (preDropNum == 2) {
            if (dropNum > 2) {
                dropNum = 2;
            }
        }
        else if (preDropNum == 1) {
            if (dropNum > 3) {
                dropNum = 3;
            }
        }
        else if (preDropNum == 4) {
            dropNum = 0;
        }
        printf ("---------------------->>>> current group processNum:%d drop %d\n", processNum, dropNum);
        preDropNum = dropNum;
    }
    processNum++;

    if (dropNum) {
        printf ("----------------> dropNum:%d current drop one frame sei\n", dropNum);
        /*drop*/
        RedunServerRecyleRedunBuf(data);
        dropNum--;
        return 0;
    }

    ret = QueueGet(&pInst->clientHndlE, (long long *)&clientData, TIMEOUT_NONE);
    if (!ret) {
        dest = (RedunBuf *)clientData->priv;
        memcpy(clientData->data, pkt_with_fpi->data, pInst->fillLength * 3);
        clientData->seqId = pkt_with_fpi->seqId;
        RtpHeadDef *rtpHead = (RtpHeadDef *)clientData->data;
        QueuePut(&pInst->clientHndlF, (long long)clientData, TIMEOUT_NONE);

        RedunServerRecyleRedunBuf(data);
    }

    return 0;
}

int main(int args, char *argv[]) {
    if (args != 7) {
        printf ("simple_server n k fillLength headFillLength inputFile outputFile\n");
        return -1;
    }
    void (*segvHandler)(int signo);

    int n               = atoi(argv[1]);
    int k               = atoi(argv[2]);
    int fillLength      = atoi(argv[3]);
    int headFillLength  = atoi(argv[4]);
    const char *input   = argv[5];
    const char *output  = argv[6];
    server_client inst;


    segvHandler = SigSegFault;
    if (SIG_ERR == signal(SIGSEGV, segvHandler)) {
        printf("signal SIGSEGV fail\r\n");
        exit(-1);
    }


    printf ("n :%d k:%d fillLength:%d headFillLength:%d input:%s output:%s\n", 
            n, k, fillLength, headFillLength, input, output);

    memset(&inst, 0x0, sizeof(inst));
    Simple_sys(&inst, n, k, fillLength, headFillLength, input, output);

    while (1) {
        sleep (10);
    }

    return 0;
}

