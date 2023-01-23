#include "k_inc.h"
#include "tsk_util.h"

int strlen(char *source) {
    int ans = 0;
    while(*source++ != '\0') {
        ans++;
    }
    return ans;
}

char *stringCopy(char* dest, const char* source) {
    if ( dest == NULL || source == NULL ) {
        return NULL;
    }
    
    char *ptr = dest;
    while ((*ptr++ = *source++) != '\0');
    return dest;   
}

void copyHeader(RTX_MSG_HDR *headerInfo, U8 *buf) {
    for(int i = 0; i < MSG_HDR_SIZE; i++) {
        buf[i] = ((U8 *)headerInfo)[i];
    }
}
void prep_msg(U8 *msgBuf, char *str, int len, int sender, int tid) {
    RTX_MSG_HDR *ptr = (RTX_MSG_HDR *)msgBuf;
    ptr->length = MSG_HDR_SIZE + len;
    ptr->sender_tid = sender;
    if(tid == TID_CON) {
        ptr->type = DISPLAY;
    } else if(tid == TID_KCD){
        ptr->type = KCD_REG;
    }
    else {
            ptr->type = KCD_CMD;
    }
    stringCopy((char *)(msgBuf + MSG_HDR_SIZE), str);
}