#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <preredun.h>

PreRedunManageStu *PreRedunManageGetByName(const char *name) {
    if (!strcmp(name, preRedunOpenFecAudDec.name)) {
        return &preRedunOpenFecAudDec;
    }
    else if (!strcmp(name, preRedunOpenFecAudEnc.name)) {
        return &preRedunOpenFecAudEnc;
    }

    return NULL;
}

PreRedunContext *PreRedunContextInstance(const PreRedunManageStu *preRedunObj) {
    PreRedunContext *preRedun = (PreRedunContext *) malloc (sizeof(*preRedun));
    if (NULL == preRedun) {
        printf ("PreRedunContext %s malloc failure\n", preRedunObj->name);
        goto ERR0;
    }

    memset(preRedun, 0x0, sizeof(*preRedun));

    preRedun->preRedunObj = preRedunObj;

    preRedun->priv     = (void *) malloc (preRedunObj->privSize);
    if (NULL == preRedun->priv) {
        printf ("preRedun->priv %s malloc failure\n", preRedunObj->name);
        goto ERR1;
    }

    return preRedun;
ERR1:
    if (preRedun) free(preRedun);
ERR0:
    return NULL;
}

int PreRedunContextUninstance(PreRedunContext *oObj) {
    if (oObj) {
        if (oObj->priv) free(oObj->priv);

        free(oObj);
    }

    return 0;
}

int PreRedunContextInit(PreRedunContext *oObj, void *data) {
    if (oObj->preRedunObj) {
        if (oObj->preRedunObj->iinit) {
            return oObj->preRedunObj->iinit(oObj, data);
        }
    }

    return -1;
}

int PreRedunContextExit(PreRedunContext *oObj) {
    if (oObj->preRedunObj) {
        if (oObj->preRedunObj->eexit) {
            return oObj->preRedunObj->eexit(oObj);
        }
    }

    return -1;
}

int PreRedunContextProcessFrame(PreRedunContext *oObj, void *redunBuf, RedunBufList *redunBufList) {
    if (oObj->preRedunObj) {
        if (oObj->preRedunObj->process) {
            return oObj->preRedunObj->process(oObj, redunBuf, redunBufList);
        }
    }

    return -1;
}

int PreRedunContextProcessAfter(PreRedunContext *oObj) {
    if (oObj->preRedunObj) {
        if (oObj->preRedunObj->processafter) {
            return oObj->preRedunObj->processafter(oObj);
        }
    }

    return -1;
}

int PreRedunContextAddRecoverFrame(PreRedunContext *oObj, RedunBuf *redunBuf) {
    RedunBufList redunBufList;
    
    redunBufList.numBufs = 0;
    redunBufList.redunBuf[redunBufList.numBufs++] = redunBuf;

    if (oObj->preRedunObj) {
        if (oObj->preRedunObj->add) {
            return oObj->preRedunObj->add(oObj, &redunBufList);
        }
    }

    return -1;
}

int PreRedunContextAddRecoverFrameList(PreRedunContext *oObj, RedunBufList *redunBufList) {
    if (oObj->preRedunObj) {
        if (oObj->preRedunObj->add) {
            return oObj->preRedunObj->add(oObj, redunBufList);
        }
    }

    return -1;
}


int PreRedunContextCommand(PreRedunContext *oObj, unsigned cmd, void *value) {
    if (oObj->preRedunObj) {
        if (oObj->preRedunObj->command) {
            return oObj->preRedunObj->command(oObj, cmd, value);
        }
    }

    return -1;
}
