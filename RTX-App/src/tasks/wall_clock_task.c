/**
 * @brief The Wall Clock Display Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "k_inc.h"
#include "tsk_util.h"

// Some basic validation, the times being in range doesnt matter as long as they are numbers since we covert anyways
int validateTime(U8 buf[MSG_HDR_SIZE + 15]) {
    if(buf[MSG_HDR_SIZE + 5] == ':' && buf[MSG_HDR_SIZE + 8] == ':' && buf[MSG_HDR_SIZE + 11] == NULL &&
                buf[MSG_HDR_SIZE + 3] >= '0' && buf[MSG_HDR_SIZE + 3] <= '2' &&
                buf[MSG_HDR_SIZE + 4] >= '0' && buf[MSG_HDR_SIZE + 4] <= '9' &&
                buf[MSG_HDR_SIZE + 6] >= '0' && buf[MSG_HDR_SIZE + 6] <= '6' &&
                buf[MSG_HDR_SIZE + 7] >= '0' && buf[MSG_HDR_SIZE + 7] <= '9' &&
                buf[MSG_HDR_SIZE + 9] >= '0' && buf[MSG_HDR_SIZE + 9] <= '9' &&
                buf[MSG_HDR_SIZE + 10] >= '0' && buf[MSG_HDR_SIZE + 10] <= '9' ){
                return TRUE;
            }
            return FALSE;
}

U32 convertSeconds(U8 buf[MSG_HDR_SIZE + 15]) {

    U32 hours = (buf[MSG_HDR_SIZE + 3] - '0') * 10 + (buf[MSG_HDR_SIZE + 4] - '0');
    U32 minutes = (buf[MSG_HDR_SIZE + 6] - '0') * 10 + (buf[MSG_HDR_SIZE + 7] - '0');
    U32 seconds = (buf[MSG_HDR_SIZE + 9] - '0') * 10 + (buf[MSG_HDR_SIZE + 10] - '0');
    return (hours * 3600 + minutes * 60 + seconds);
}

// TODO: eventually turn these buffers into dynamically allocated memory, leaving it like this for now since it's safer
void task_wall_clock(void)
{
    mbx_create(CON_MBX_SIZE);
    U8 initMsg[MSG_HDR_SIZE + 1];

    // register
    char wSetup[1];
    wSetup[0] = 'W';
    prep_msg(initMsg, wSetup, 1, TID_WCLCK, TID_KCD);
    send_msg_nb(TID_KCD, initMsg);

    TIMEVAL rtInfo;
    rtInfo.sec = 1;
    rtInfo.usec = 0;
    rt_tsk_set(&rtInfo);
    //WT HH:MM:SS
// MM = (Seconds / 60) % 60, HH = (seconds / 3600) % 24, SS = seconds%60
// 20000 seconds = 05:33:

    U32 baseTime = 0;
    U32 baseTick = 0;// keeps track of the base g_timer_count in case some periods are missed
    int run = FALSE;

    char invalidStr[25];
    sprintf(invalidStr, "\r\nInvalid Command\r\n");  
	U8 invBuf[MSG_HDR_SIZE + 25];
    prep_msg(invBuf, invalidStr, 25, TID_WCLCK, TID_CON);	

    char clearStr[4];
    sprintf(clearStr, "\r\n");  
	U8 clrBuf[MSG_HDR_SIZE + 4];
    prep_msg(clrBuf, clearStr, 4, TID_WCLCK, TID_CON);	


    while(1) {
        U8 buf[MSG_HDR_SIZE + 15];
        int msg = recv_msg_nb(buf, MSG_HDR_SIZE + 15);
        if(msg != RTX_ERR) {
            if(buf[MSG_HDR_SIZE + 1] == 'R') {
                baseTime = 0;
                baseTick = g_timer_count;
                run = TRUE;
                send_msg_nb(TID_CON, clrBuf);
            } else if(buf[MSG_HDR_SIZE + 1] == 'S') {
                if(validateTime(buf)){
                    baseTime = convertSeconds(buf);
                    baseTick = g_timer_count;
                    run = TRUE;
                    send_msg_nb(TID_CON, clrBuf);
											
                } else {
                    send_msg_nb(TID_CON, invBuf);
                }
            } else if(buf[MSG_HDR_SIZE + 1] == 'T') {
                run = FALSE;
                char strClr[30];
                sprintf(strClr,"\033[s\033[1;70f\033[K\033[u");
                U8 tBuf[ MSG_HDR_SIZE + 30];
                prep_msg(tBuf, strClr, 30, TID_WCLCK, TID_CON);
                send_msg_nb(TID_CON, tBuf);
                send_msg_nb(TID_CON, clrBuf);
            }
        }

        if(run) {
            char str[40];
            // We don't need to overwrite/clear this buffer since it's being written to with a constant size
			U8 msgBuf[MSG_HDR_SIZE + 40];

			// We update seconds like this in case we miss some deadlines during overload, otherwise we would just do secondsTotal++
            // 2000 ticks = 1 second
            int secondsTotal = baseTime + ((g_timer_count - baseTick) / 2000);
            int hours = (secondsTotal /3600) % 24;
            int minutes = (secondsTotal / 60) % 60;
            int seconds = secondsTotal % 60;

            // For flash:
            sprintf(str, "\033[s\033[1;74f%02d:%02d:%02d\r\n\033[u", hours, minutes, seconds);
            // For sim:
             //sprintf(str, "\r\n%02d:%02d:%02d\r\n", hours, minutes, seconds);
            prep_msg(msgBuf, str, 40, TID_WCLCK, TID_CON);
            send_msg_nb(TID_CON, msgBuf);
        }
        rt_tsk_susp();
    }
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

