/** 
 * @brief The KCD Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "common.h"
#include "printf.h"
#include "tsk_util.h"
#include "k_inc.h"

// 26 alpha + 26 Upper alpha + 10 numbers (0 -> 9)
int commands[62];
char strBuf[KCD_CMD_BUF_SIZE];

void clear(int pos) {
    for(int i = 0; i < pos; i++) {
        strBuf[i] = NULL;
    }
}

// VERY basic hashmap
void cmd_map_register(int cmd, int senderTID) {
    if(cmd < 10) {
        commands[cmd] = senderTID;
    } else if(cmd < 'a') {
        commands[cmd - 'A' + 10] = senderTID;
    } else {
        commands[cmd - 'a' + 36] = senderTID;
    }
}

int cmd_map_get(int cmd) {
    if(cmd < 10) {
        return commands[cmd];
    } else if(cmd < 'a') {
        return commands[cmd - 'A' + 10];
    } else {
        return commands[cmd - 'a' + 36];
    }
}


char *get_states(int state) {
    switch(state) {
        case 1:
            return "READY";
        case 2:
            return "RUNNING";
        case 3:
            return "BLK_SEND";
        case 4:
            return "BLK_RECV";
        case 5:
            return "SUSPENDED";
        default:
            return "DORMANT";

    }
}
void err(int type) {
    char str[25];
    if(type == 2) {
        sprintf(str, "\r\nCommand not found\r\n");
    } else {
        sprintf(str, "\r\nInvalid command\r\n");
    }
    int length = strlen(str) + 1;
    U8 *msgBuf = mem_alloc(MSG_HDR_SIZE + length);
    prep_msg(msgBuf, str, length, TID_KCD, TID_CON);
    send_msg(TID_CON, msgBuf);
    mem_dealloc(msgBuf);
}
void cmd_LT() {
    for(int i = 0; i < MAX_TASKS; i++){
        RTX_TASK_INFO task_info;
        tsk_get(i, &task_info);
        if(task_info.state != DORMANT) {
            char str[30]; // 30 is roughly the longest this string can be
            sprintf(str, "\r\n TID=%d, State= %s\r\n", task_info.tid, get_states(task_info.state));
            int length = strlen(str) + 1;
            U8 *msgBuf = mem_alloc(MSG_HDR_SIZE + length);
            prep_msg(msgBuf, str, length, TID_KCD, TID_CON );
            send_msg(TID_CON, msgBuf);
            mem_dealloc(msgBuf);
        }
    }
}

void cmd_LM() {
    for(int i = 0; i < MAX_TASKS; i++){
        RTX_TASK_INFO task_info;
        tsk_get(i, &task_info);
        if(task_info.state != DORMANT ) {
            int space = mbx_get(task_info.tid);
            if(space != -1) {
                char str[50]; // 50 is roughly the longest this string can be
                sprintf(str, "\r\n TID = %d, State = %s, Free = %d\r\n", task_info.tid, get_states(task_info.state), space);
                int length = strlen(str) + 1;
                U8 *msgBuf = mem_alloc(MSG_HDR_SIZE + length);
                prep_msg(msgBuf, str, length, TID_KCD, TID_CON);
                send_msg(TID_CON, msgBuf);
                mem_dealloc(msgBuf);
            }
        }
    }
}
void task_kcd(void)
{
    int mbxID = mbx_create(KCD_MBX_SIZE);
    int pos = 0;
    while(1) {
        // call blocking receive
        char *buf = mem_alloc(KCD_CMD_BUF_SIZE + MSG_HDR_SIZE);
        if(recv_msg(buf, KCD_CMD_BUF_SIZE) != RTX_ERR) {
            RTX_MSG_HDR *headerInfo = (RTX_MSG_HDR *) buf;
            if(headerInfo->type == KCD_REG) {
                cmd_map_register(buf[MSG_HDR_SIZE], headerInfo->sender_tid);
            } else if (headerInfo->type == KEY_IN) {
                // Check if we have an enter or just a normal key
                if((int)(buf[MSG_HDR_SIZE]) == '\r') {
                  if(strBuf[0] != '%') {
                        // error, command not found 
                        err(1);
                    } else if(strBuf[1] == 'L') {
                        // we have a command for us
                        if(strBuf[2] == 'T') {
                            cmd_LT();
                        } else if(strBuf[2] == 'M') {
                            cmd_LM();
                        } else {
                            // Unrecognized
                            err(2);
                        }
                    } else {
                        // we have a command for someone else
                        int receiverID = cmd_map_get(strBuf[1]);
                        RTX_TASK_INFO task_info;
                        if(receiverID != 0) {
                            tsk_get(receiverID, &task_info);
                            if(task_info.state != DORMANT) {
                                U8 *msgBuf = mem_alloc(MSG_HDR_SIZE + pos);
                                prep_msg(msgBuf, strBuf + 1, pos, TID_KCD, receiverID); 
                                if(mbx_get(receiverID) != -1) {
                                    send_msg(receiverID, msgBuf);
                                } else {
                                    err(2);
                                }
                                mem_dealloc(msgBuf);
                            } else {
                                err(2);
                            }
                        } else {
                            err(2);
                        }
                    }
                    clear(pos);
                    pos = 0;
                } else {
                    headerInfo->sender_tid = TID_KCD;
                    headerInfo->type = DISPLAY;
                    send_msg(TID_CON, buf);
                    if(pos < KCD_CMD_BUF_SIZE) {
                        for(int i = MSG_HDR_SIZE; i < headerInfo->length; i++) {
                            strBuf[pos] = buf[i];
                            pos++;
                        }
                    } 
                    if(pos > KCD_CMD_BUF_SIZE) {
                        // Make sure the first character in the buffer is not %
                        if(strBuf[0] == '%') {
                            strBuf[0] = '0';
                        }
                    }
                } 

            }
        }
        mem_dealloc(buf);
    }


}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

