#ifndef __QUEUE_OPENFEC_H__
#define __QUEUE_OPENFEC_H__

#include <pthread.h>

typedef struct {

    unsigned curRd;
    unsigned curWr;
    unsigned len;
    unsigned count;

    long long *queue;

    pthread_mutex_t lock;
    pthread_cond_t  condRd;
    pthread_cond_t  condWr;

} QueHndl;

#define TIMEOUT_NONE        ((unsigned) 0)  // no timeout
#define TIMEOUT_FOREVER     ((unsigned)-1)  // wait forever

int QueueCreate(QueHndl *hndl, unsigned maxLen);
int QueueDelete(QueHndl *hndl);
int QueuePut(QueHndl *hndl, long long value, unsigned timeout);
int QueueGet(QueHndl *hndl, long long *value, unsigned timeout);
int QueueGetPlus(QueHndl *hndl, long long *value, unsigned timeout, unsigned *pQueuedCounts);
unsigned QueueGetQueuedCount(QueHndl *hndl);
int QueueIsEmpty(QueHndl *hndl);
int QueueIsEmpty(QueHndl *hndl);

#endif /*__QUEUE_OPENFEC_H__*/
