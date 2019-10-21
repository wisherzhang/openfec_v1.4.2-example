#ifndef __PRE_RUDUN_H__
#define __PRE_RUDUN_H__ 

#include <redun.h>

typedef enum {
    PREREDUN_COMMAND_ISSAVE_FILE,
    PREREDUN_COMMAND_CNT,
} PREREDUN_COMMAND;

struct PreRedunManageStu;
typedef struct PreRedunContext {
    char *name;                     ///< name of this filter instance
    void *priv;                     ///< private data for use by the filter

    const struct PreRedunManageStu *preRedunObj;
} PreRedunContext;

typedef struct PreRedunManageStu {
    const char *name;
    int privSize;

    int (*iinit)(PreRedunContext *oObj, void *config);
    int (*eexit)(PreRedunContext *oObj);
    int (*process)(PreRedunContext *oObj, void *data, RedunBufList *redunBufList);
    int (*add)(PreRedunContext *oObj, RedunBufList *redunBufList);
    int (*processafter)(PreRedunContext *oObj);
    int (*command)(PreRedunContext *oObj, unsigned cmd, void *value);
} PreRedunManageStu;

extern PreRedunManageStu preRedunOpenFecAudDec;
extern PreRedunManageStu preRedunOpenFecAudEnc;

/*manage stu*/
PreRedunManageStu *PreRedunManageGetByName(const char *name);

/*Context*/
PreRedunContext *PreRedunContextInstance(const PreRedunManageStu *preRedunObj);
int PreRedunContextUninstance(PreRedunContext *oObj);

int PreRedunContextInit(PreRedunContext *oObj, void *data);
int PreRedunContextExit(PreRedunContext *oObj);
//对数据进行分组处理
//参数redunBuf       -----> 新接收的数据，待要进行分组的数据
//参数redunBufList   -----> 分组完成，将分组数据返回通过该参数
//返回值             -----> 返回0表示分组完成， -1表示分组未完成
int PreRedunContextProcessFrame(PreRedunContext *oObj, void *redunBuf, RedunBufList *redunBufList);
//将有丢包的数据帧添加到分组器中
int PreRedunContextAddRecoverFrame(PreRedunContext *oObj, RedunBuf *redunBuf);
int PreRedunContextAddRecoverFrameList(PreRedunContext *oObj, RedunBufList *redunBufList);

//冗余处理完成调用，主要是进行一些状态处理，并对数据进行发送
int PreRedunContextProcessAfter(PreRedunContext *oObj);

int PreRedunContextCommand(PreRedunContext *oObj, unsigned cmd, void *value);

#endif /*__PRE_RUDUN_H__*/
