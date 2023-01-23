/**
 * @brief The Console Display Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "k_inc.h"
#include "k_rtx.h"

void task_cdisp(void)
{
    int mbxID = mbx_create(CON_MBX_SIZE + MSG_HDR_SIZE);
    LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
    while(1) {
        char *buf = mem_alloc(CON_MBX_SIZE);
        if(recv_msg(buf, CON_MBX_SIZE) != RTX_ERR) {
            RTX_MSG_HDR *headerInfo = (RTX_MSG_HDR *)buf;
            if(headerInfo->type == DISPLAY) {
                send_msg(TID_UART, buf);
                    pUart->IER |= IER_THRE;
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

