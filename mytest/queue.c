#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <queue.h>

int QueueCreate(QueHndl *hndl, unsigned maxLen)
{
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;
    int status = 0;

    hndl->curRd = hndl->curWr = 0;
    hndl->count = 0;
    hndl->len   = maxLen;
    hndl->queue = (long long*)malloc(sizeof(long long) * hndl->len);

    if (hndl->queue == NULL) {
        printf("OSA_queCreate() = %d \r\n", status);
        return -1;
    }

    status |= pthread_mutexattr_init(&mutex_attr);
    status |= pthread_condattr_init(&cond_attr);

    status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
    status |= pthread_cond_init(&hndl->condRd, &cond_attr);
    status |= pthread_cond_init(&hndl->condWr, &cond_attr);

    if (status != 0)
        printf("OSA_queCreate() = %d \r\n", status);

    pthread_condattr_destroy(&cond_attr);
    pthread_mutexattr_destroy(&mutex_attr);

    return status;
}

int QueueDelete(QueHndl *hndl)
{
    if (hndl->queue != NULL)
        free(hndl->queue);

    pthread_cond_destroy(&hndl->condRd);
    pthread_cond_destroy(&hndl->condWr);
    pthread_mutex_destroy(&hndl->lock);

    return 0;
}

int QueuePut(QueHndl *hndl, long long value, unsigned timeout)
{
    int status = -1;

    pthread_mutex_lock(&hndl->lock);

    while (1) {
        if (hndl->count < hndl->len) {
            hndl->queue[hndl->curWr] = value;
            hndl->curWr = (hndl->curWr + 1) % hndl->len;
            hndl->count++;
            status = 0;
            pthread_cond_signal(&hndl->condRd);
            break;
        } else {
            if (timeout == TIMEOUT_NONE)
                break;

            pthread_cond_wait(&hndl->condWr, &hndl->lock);
        }
    }

    pthread_mutex_unlock(&hndl->lock);

    return status;
}

int QueueGet(QueHndl *hndl, long long *value, unsigned timeout)
{
    int status = -1;

    pthread_mutex_lock(&hndl->lock);

    while (1) {
        if (hndl->count > 0) {

            if (value != NULL) {
                *value = hndl->queue[hndl->curRd];
            }

            hndl->curRd = (hndl->curRd + 1) % hndl->len;
            hndl->count--;
            status = 0;
            pthread_cond_signal(&hndl->condWr);
            break;
        } else {
            if (timeout == TIMEOUT_NONE)
                break;

            pthread_cond_wait(&hndl->condRd, &hndl->lock);
        }
    }

    pthread_mutex_unlock(&hndl->lock);

    return status;
}

int QueueGetPlus(QueHndl *hndl, long long *value, unsigned timeout, unsigned *pQueuedCounts)
{
    int status = -1;

    pthread_mutex_lock(&hndl->lock);

    while (1) {
        if (hndl->count > 0) {

            if (value != NULL) {
                *value = hndl->queue[hndl->curRd];
            }

            hndl->curRd = (hndl->curRd + 1) % hndl->len;
            hndl->count--;

            if (pQueuedCounts) *pQueuedCounts = hndl->count;

            status = 0;
            pthread_cond_signal(&hndl->condWr);
            break;
        } else {
            if (timeout == TIMEOUT_NONE) {
                if (pQueuedCounts)*pQueuedCounts = 0;
                break;
            }

            pthread_cond_wait(&hndl->condRd, &hndl->lock);
        }
    }

    pthread_mutex_unlock(&hndl->lock);

    return status;
}

unsigned QueueGetQueuedCount(QueHndl *hndl)
{
    unsigned queuedCount = 0;

    pthread_mutex_lock(&hndl->lock);
    queuedCount = hndl->count;
    pthread_mutex_unlock(&hndl->lock);
    return queuedCount;
}

int QueuePeek(QueHndl *hndl, long long *value)
{
    int status = -1;
    pthread_mutex_lock(&hndl->lock);
    if (hndl->count > 0) {
        if (value != NULL) {
            *value = hndl->queue[hndl->curRd];
            status = 0;
        }
    }
    pthread_mutex_unlock(&hndl->lock);

    return status;
}

int QueueIsEmpty(QueHndl *hndl)
{
    int isEmpty;

    pthread_mutex_lock(&hndl->lock);
    if (hndl->count == 0) {
        isEmpty = 1;
    } else {
        isEmpty = 0;
    }
    pthread_mutex_unlock(&hndl->lock);

    return isEmpty;
}

