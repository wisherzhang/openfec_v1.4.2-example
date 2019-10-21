#ifndef __PRE_RUDUN_OPENFEC_H__
#define __PRE_RUDUN_OPENFEC_H__ 

#define REDUN_MAX_LENGTH        (2048)
#define REDUN_MAX_NUM           (255)
#define REDUN_MAX_HEAD_LENGTH   (128)
#define REDUN_ALT_NUM           (2)
#define REDUN_BUFFER_NUM        (50)
#define REDUN_REF_RESTORE_NUM   (2)
#define REDUN_REF_JUST_SEND_NUM (1)

typedef struct PreRedunManageDecParmas {
    unsigned k;
    unsigned n;
    unsigned fillLength;
    unsigned headFillLength;

    void *priv;

    //内部结构与外部结构转换接口,只有转换后才能发送
    //参数1     ---->外部调用接口的主句柄
    //参数2     ---->RedunBuf结构
    //返回值    ---->外部调用接口对应的存储结构
    void *(*transformFromRedun)(void *oObj, void *data, void *repair, void *headRepair, int isSrc);    

    //内部结构与外部结构转换接口,只有转换后才能处理
    //参数1     ---->外部调用接口的主句柄
    //参数2     ---->RedunBuf结构
    //参数3     ---->外部调用接口对应的存储结构
    //返回值    ---->RedunBuf结构
    void *(*transformToRedun)(void *oObj, void **redunBuf, void *data, int isSrc);

    //发送到下一个环节接口
    //参数1     ---->外部调用接口的主句柄
    //参数2     ---->外部调用接口对应的存储结构
    //返回值    ---->发送是否成功
    int (*ssend)(void *oObj, void *data);       

    //释放数据接口
    //参数1     ---->外部调用接口的主句柄
    //参数2     ---->RedunBuf结构
    //返回值    ---->释放是否成功
    int (*release)(void *oObj, void *redunBuf); 

    //获取序列号接口
    unsigned (*getSeqId)(void *oObj, void *data);
} PreRedunManageDecParmas;

typedef struct PreRedunManageEncParmas {
    unsigned k;
    unsigned n;
    unsigned fillLength;
    unsigned headFillLength;

    void *priv;

    //内部结构与外部结构转换接口,只有转换后才能发送
    //参数1     ---->外部调用接口的主句柄
    //参数2     ---->RedunBuf结构
    //返回值    ---->外部调用接口对应的存储结构
    void *(*transformFromRedun)(void *oObj, void *data, void *repair, void *headRepair, int isSrc);    

    //内部结构与外部结构转换接口,只有转换后才能处理
    //参数1     ---->外部调用接口的主句柄
    //参数2     ---->RedunBuf结构
    //参数3     ---->外部调用接口对应的存储结构
    //返回值    ---->RedunBuf结构
    void *(*transformToRedun)(void *oObj, void **redunBuf, void *data, int isSrc);

    //发送到下一个环节接口
    //参数1     ---->外部调用接口的主句柄
    //参数2     ---->外部调用接口对应的存储结构
    //返回值    ---->发送是否成功
    int (*ssend)(void *oObj, void *data);       

    //释放数据接口
    //参数1     ---->外部调用接口的主句柄
    //参数2     ---->RedunBuf结构
    //返回值    ---->释放是否成功
    int (*release)(void *oObj, void *redunBuf);

    //获取序列号接口
    unsigned (*getSeqId)(void *oObj, void *data);
} PreRedunManageEncParmas;

#endif /*__PRE_RUDUN_OPENFEC_H__*/
