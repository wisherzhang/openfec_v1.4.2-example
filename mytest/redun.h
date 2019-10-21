#ifndef __RUDUN_H__
#define __RUDUN_H__ 

#define REDUN_MAX_INPUT_NUM (255)

typedef enum {
    REDUN_COMMAND_ISSAVE_FILE,
    REDUN_COMMAND_CNT,
} REDUN_COMMAND;

typedef struct {
    int sei;                    //分组序列
    int refId;                  //包被引用次数，决定是否能被释放

    void *handle;               //包所在的实际队列句柄
    void *handleRes;            //包从下一个环节释放时的中间队列句柄

    void *data;                 //指向具体的数据      ::::::注:需要外部填充
    void *dataHead;             //指向头部数据内存    ::::::注:看需要，由外部填充

    void *dataRepair;           //指向冗余数据        ::::::注:需要外部填充
    void *dataHeadRepair;       //指向头部数据冗余区  ::::::注:看需要，由外部填充

    void *priv;                 //指向外部对接的管理数据结构
} RedunBuf;

typedef struct {
    int numBufs;
    RedunBuf *redunBuf[REDUN_MAX_INPUT_NUM];
} RedunBufList;

struct RedunManageStu;
typedef struct RedunContext {
    char *name;
    void *priv;

    const struct RedunManageStu *redunObj;
} RedunContext;

typedef struct RedunManageStu {
    const char *name;                                           //冗余器名字
    int privSize;                                               //冗余器私有结构体大小

    int (*oopen)(RedunContext *oObj, void *config);             //打开冗余器
    int (*cclose)(RedunContext *oObj);                          //关闭冗余器
    int (*reset)(RedunContext *oObj);                           //复位冗余器
    int (*add)(RedunContext *data, RedunBufList *pDataList);    //导入数据组到冗余器
    int (*process)(RedunContext *oObj);//处理一次冗余
    int (*command)(RedunContext *oObj, unsigned cmd, void *value);
} RedunManageStu;


extern RedunManageStu redunOpenFecAudDec;
extern RedunManageStu redunOpenFecAudEnc;

/*manage stu*/
RedunManageStu *RedunManageGetByName(const char *name);

/*Context*/
RedunContext *RedunContextInstance(const RedunManageStu *redunObj);
int RedunContextUninstance(RedunContext *oObj);

//初始化、打开冗余器
int RedunContextInit(RedunContext *oObj, void *data);

//关闭冗余器
int RedunContextExit(RedunContext *oObj);

//复位冗余器
int RedunContextReset(RedunContext *oObj);

//添加一帧数据帧到冗余器
int RedunContextAddFrame(RedunContext *oObj, RedunBuf *data);

//添加多帧数据到冗余器
int RedunContextAddFrameList(RedunContext *oObj, RedunBufList *pDataList);

//实际冗余处理接口
int RedunContextProcessFrame(RedunContext *oObj);

int RedunContextCommand(RedunContext *oObj, unsigned cmd, void *value);

#endif /*__RUDUN_H__*/
