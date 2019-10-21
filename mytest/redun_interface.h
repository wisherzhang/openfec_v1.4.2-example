#ifndef __OPENFEC_INTERFACE_H__
#define __OPENFEC_INTERFACE_H__ 

#include "queue.h"
#include "redun.h"
#include "redun_config.h"
#include "preredun.h"
#include "preredun_config.h"

typedef struct {
    void *data; 
    void *priv;
    unsigned seqId;
} serverClientBuf;

typedef void *(*request)(void *, unsigned size, unsigned esi);
typedef void *(*transformto)(void *oObj, void **redunBuf, void *data, int isSrc);
typedef void *(*transformfrom)(void *oObj, void *redunBuf, void *repair, void *headRepair, int isSrc);
typedef int (*release)(void *oObj, void *redunBuf);
typedef int (*ssend)(void *, void *);
typedef int (*fill)(void *, void *, void *, void *);
typedef unsigned (*getSeqId)(void *, void *);


/*   server   */
typedef struct {
    int k;
    int n;
    int fillLength;
    int headFillLength;
    of_codec_id_t	codec_id;

    QueHndl hndlF;
    QueHndl hndlE;
    QueHndl hndlRes;

    RedunBuf redunBuf[REDUN_MAX_INPUT_NUM];

    RedunContext *redunContext;             //冗余器管理结构
    RedunManageStu *redunObj;               //冗余器接口结构

    RedunManageEncParmas config;            //冗余器初始化参数结构

    PreRedunContext *preRedunContext;       //分组管理结构
    PreRedunManageStu *preRedunObj;         //分组接口结构

    PreRedunManageEncParmas configg;        //分组初始化参数结构

    //需要再处理一下
    unsigned headTmp[REDUN_MAX_INPUT_NUM][REDUN_MAX_HEAD_LENGTH];
                                            //头数据临时结构

    RedunBuf *(*request)(void *oObj, unsigned size, unsigned esi);

    void *(*transformToRedun)(void *oObj, void **redunBuf, void *data, int isSrc);
    void *(*transformFromRedun)(void *oObj, void *data, void *repair, void *headRepair, int isSrc);    
    int (*ssend)(void *oObj, void *data);
    int (*release)(void *oObj, void *redunBuf);
    unsigned (*getSeqId)(void *, void *);

    void *priv;
} RedunServerObj;


/*初始化接口*/
int RedunServerInit(RedunServerObj *pObj, 
        int k, int n, int fillLength, 
        int headFillLength,
        int codec_id, void *priv, 
        transformto toRedun, 
        transformfrom fromRedun, 
        release rls, 
        ssend sendd, 
        getSeqId getseqid);

/*销毁接口*/
extern int RedunServerExit(RedunServerObj *pObj);

/*主运行接口*/
extern int RedunServerRun(RedunServerObj *pObj, void *data);

/*下一级环节调用归还数据包接口*/
extern int RedunServerRecyleRedunBuf(void *data);

extern int RedunServerCommand(RedunServerObj *pObj, unsigned cmd, void *value);


/* client */
typedef struct {
    int k;                                  //源始包个数 
    int n;                                  //源始包和冗余包个数
    int fillLength;                         //源数据对齐后的长度
    int headFillLength;                     //源头数据对齐后的长度
    of_codec_id_t	codec_id;               //冗余器ID

    void *priv;                             //指向主外层结构

    RedunContext *redunContext;             //冗余器管理结构
    RedunManageStu *redunObj;               //冗余器接口结构

    RedunManageDecParmas config;            //冗余器初始化参数结构

    PreRedunContext *preRedunContext;       //分组管理结构
    PreRedunManageStu *preRedunObj;         //分组接口结构

    PreRedunManageDecParmas configg;        //分组初始化参数结构

    //需要再处理一下
    // unsigned headTmp[REDUN_MAX_INPUT_NUM][REDUN_MAX_HEAD_LENGTH];
                                            //头数据临时结构

    void *(*requestsrcnew)(void *oObj, unsigned size, unsigned esi);
    void *(*requestrepairnew)(void *oObj, unsigned size, unsigned esi);
    void *(*transformToRedun)(void *oObj, void **redunBuf, void *data, int isSrc);
    void *(*transformFromRedun)(void *oObj, void *data, void *repair, void *headRepair, int isSrc);    
    int (*release)(void *oObj, void *redunBuf);
    int (*ssend)(void *, void *);
    unsigned (*getSeqId)(void *, void *);

    QueHndl hndlRes;
    QueHndl hndlE;
    RedunBuf redunBuf[REDUN_MAX_INPUT_NUM];
                                            //申请内存回调，用于从前一级申请内存
} RedunClientObj;

/*初始化接口*/
extern int RedunClientInit(RedunClientObj *pObj, 
        int k, int n, int fillLength, int headFillLength,
        int codec_id, void *priv,
        request requestsrcnew, 
        request requestrepairnew, 
        transformto toRedun, 
        transformfrom fromRedun, 
        release rls, 
        ssend sendd, 
        getSeqId getseqid);

/*销毁接口*/
extern int RedunClientExit(RedunClientObj *pObj);

/*主运行接口*/
extern int RedunClientRun(RedunClientObj *pObj, void *data);

/*下一级环节归还数据包接口*/
extern int RedunClientRecyleRedunBuf(void *data);

extern int RedunClientCommand(RedunClientObj *pObj, unsigned cmd, void *value);

#endif /*__OPENFEC_INTERFACE_H__*/
