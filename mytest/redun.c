#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <redun.h>

RedunManageStu *RedunManageGetByName(const char *name) {
    if (!strcmp(name, redunOpenFecAudDec.name)) {
        return &redunOpenFecAudDec;
    }
    else if (!strcmp(name, redunOpenFecAudEnc.name)) {
        return &redunOpenFecAudEnc;
    }

    return NULL;
}

RedunContext *RedunContextInstance(const RedunManageStu *redunObj) {
    RedunContext *redun = (RedunContext *) malloc (sizeof(*redun));
    if (NULL == redun) {
        printf ("RedunContext %s malloc failure\n", redunObj->name);
        goto ERR0;
    }

    memset(redun, 0x0, sizeof(*redun));

    redun->redunObj = redunObj;

    redun->priv     = (void *) malloc (redunObj->privSize);
    if (NULL == redun->priv) {
        printf ("redun->priv %s malloc failure\n", redunObj->name);
        goto ERR1;
    }

    return redun;
ERR1:
    if (redun) free(redun);
ERR0:
    return NULL;
}

int RedunContextUninstance(RedunContext *oObj) {
    if (oObj) {
        if (oObj->priv) free(oObj->priv);

        free(oObj);
    }

    return 0;
}

int RedunContextInit(RedunContext *oObj, void *data) {
    if (oObj->redunObj) {
        if (oObj->redunObj->oopen) {
            return oObj->redunObj->oopen(oObj, data);
        }
    }

    return -1;
}

int RedunContextExit(RedunContext *oObj) {
    if (oObj->redunObj) {
        if (oObj->redunObj->cclose) {
            return oObj->redunObj->cclose(oObj);
        }
    }

    return -1;
}

int RedunContextReset(RedunContext *oObj) {
    if (oObj->redunObj) {
        if (oObj->redunObj->reset) {
            return oObj->redunObj->reset(oObj);
        }
    }

    return -1;
}

int RedunContextAddFrame(RedunContext *oObj, RedunBuf *data) {
    RedunBufList pDataList;
    
    pDataList.numBufs       = 1;
    pDataList.redunBuf[0]   = data;

    if (oObj->redunObj) {
        if (oObj->redunObj->add) {
            return oObj->redunObj->add(oObj, &pDataList);
        }
    }

    return -1;
}

int RedunContextAddFrameList(RedunContext *oObj, RedunBufList *pDataList) {
    if (oObj->redunObj) {
        if (oObj->redunObj->add) {
            return oObj->redunObj->add(oObj, pDataList);
        }
    }

    return -1;
}

int RedunContextProcessFrame(RedunContext *oObj) {
    if (oObj->redunObj) {
        if (oObj->redunObj->process) {
            return oObj->redunObj->process(oObj);
        }
    }

    return -1;
}

int RedunContextCommand(RedunContext *oObj, unsigned cmd, void *value) {
    if (oObj->redunObj) {
        if (oObj->redunObj->command) {
            return oObj->redunObj->command(oObj, cmd, value);
        }
    }

    return -1;
}

