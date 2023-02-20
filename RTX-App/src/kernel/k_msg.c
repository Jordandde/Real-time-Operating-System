/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTX LAB  
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *                          All rights reserved.
 *---------------------------------------------------------------------------
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice and the following disclaimer.
 *
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
 

/**************************************************************************//**
 * @file        k_msg.c
 * @brief       kernel message passing routines          
 * @version     V1.2021.06
 * @authors     Yiqing Huang
 * @date        2021 JUN
 *****************************************************************************/

#include "k_inc.h"
#include "k_rtx.h"
#include "k_msg.h"
#include "k_task.h"

//Mailbox g_mbxs[MAX_TASKS];
Mailbox *uart_mbx;
// Block the currently running task
void block(int blockType) {
    if(blockType) {
        gp_current_task->state = BLK_SEND;
    } else {
        gp_current_task->state = BLK_RECV;
    }
   k_tsk_run_new(FRONT); 
}

void enqueueML(MSGList waitingList[5], Node *curr, int prio) {
    Node *temp = waitingList[prio % 0x80].tail;
    temp->next = curr;
    curr->prev = temp;
    curr->next = NULL;
    waitingList[prio % 0x80].tail = curr;
}

int write_msg(Ring_buffer *ring_buffer, const void *buf) {
    RTX_MSG_HDR *headerInfo = (RTX_MSG_HDR *)buf;
    int length = headerInfo->length;

    if( length > (ring_buffer->length - ring_buffer->used)) {
        return -1;
    }
    //Now we can assume that we have enough space in the buffer to write the message
    for(int i = 0; i < length; i++) {
        ring_buffer->buff[ring_buffer->tail] = ((U8 *)buf)[i];
        ring_buffer->tail = (ring_buffer->tail + 1) % ring_buffer->length;
    }

    ring_buffer->used += length;
    
    return 0;
}

int pop_msg(Ring_buffer *ring_buffer, void *buf, int len) {
    if(ring_buffer->used == 0) {
        return -1;
    }

    RTX_MSG_HDR *headerInfo = (RTX_MSG_HDR *)((void *)((int)ring_buffer->buff + ring_buffer->head));
    if(headerInfo->length > len) {
        // return an error saying the sent buffer is not big enough
        return -1;
    }

    for(int i = 0; i < headerInfo->length; i++){
        ((U8 *)buf)[i] = ring_buffer->buff[ring_buffer->head];
        ring_buffer->head = (ring_buffer->head + 1) % ring_buffer->length;
    }
    ring_buffer->used -= headerInfo->length;
    return 0;
}

int write_unblock_buffer( MSGList *waitingList, Node *tempBuf, Ring_buffer *rb, int tid) {
    int results = 0;
    while(tempBuf!= NULL) {
        const void *buffer = tempBuf->info;
        RTX_MSG_HDR *headerInfo = (RTX_MSG_HDR *)buffer;
        int result = write_msg(rb, buffer);
        // unblock the sender of that message if results is 0 
        if(result == 0) {
            results++;
            Node *old = tempBuf;
            TCB *p_tcb;
            if(headerInfo->sender_tid != TID_UART){
                p_tcb = &g_tcbs[headerInfo->sender_tid];
            }
            if(waitingList->head->next == NULL ) {
                waitingList->head = NULL;
                waitingList->tail = NULL;
                k_mpool_dealloc(MPID_IRAM2, old->info);
                k_mpool_dealloc(MPID_IRAM2, old);
                tempBuf = NULL;
            } else {
                waitingList->head = waitingList->head->next;
                waitingList->head->prev = NULL;
                tempBuf = tempBuf->next;
                k_mpool_dealloc(MPID_IRAM2, old->info);
                k_mpool_dealloc(MPID_IRAM2, old);
                tempBuf->prev = NULL;
			}
            if(headerInfo->sender_tid != TID_UART && p_tcb->state == BLK_SEND) {
                p_tcb->state = READY;
                if(p_tcb->prio != PRIO_RT) {
                    enqueue(&readyList[p_tcb->prio %0x80], BACK, p_tcb);
                } else {
                    enqueue(&RTList, SORTED, p_tcb);
                }
                if((p_tcb->prio < gp_current_task->prio) && tid != TID_UART) {
                    // preempt this task now
                    k_tsk_yield();
                }
            }
        } else {
            return results;
        }
    }
    waitingList->head = NULL;
    waitingList->tail = NULL;
    return results;
}

int write_msg_from_waiting_list(Mailbox *mbx, int tid) {
    for(int i = 0; i < 5; i++) {
        if(mbx->waitingList[i].head != NULL) {
           int results = write_unblock_buffer(&mbx->waitingList[i], mbx->waitingList[i].head, mbx->buffer, tid);
            mbx->msgsWaiting = mbx->msgsWaiting - results; 
            return results;
        }
    }
    // No messages waitiing
    return -1;
}
void mbx_create_general(size_t size, Mailbox *mbx, int tid) {
    void *buf = k_mpool_alloc(MPID_IRAM2, sizeof(Mailbox) + sizeof(Ring_buffer) + size + sizeof(MSGList) * 5);
    uart_mbx = (Mailbox *) buf;
    uart_mbx->buffer = (void *)((char *)buf + sizeof(Mailbox));
    uart_mbx->buffer->buff = (void *)((char *)buf + sizeof(Mailbox) + sizeof(Ring_buffer));
    uart_mbx->waitingList = (void *)((char *)buf + sizeof(Mailbox) + sizeof(Ring_buffer) + size);
    uart_mbx->buffer->length = (int)size;
    uart_mbx->buffer->head= 0;
    uart_mbx->buffer->tail= 0;
    uart_mbx->buffer->used= 0;
    uart_mbx->id = tid;

    for(int i = 0 ; i < 5; i++) {

        uart_mbx->waitingList[i].head = NULL;
        uart_mbx->waitingList[i].tail= NULL;
    }
}

int k_mbx_create(size_t size) {
#ifdef DEBUG_0
    printf("k_mbx_create: size = %u\r\n", size);
#endif /* DEBUG_0 */
    if(gp_current_task->mbx != NULL) {
        errno = EEXIST;
        return RTX_ERR;
    } 
    if(size < MIN_MSG_SIZE) {
        errno = EINVAL;
        return RTX_ERR;
    }
    void *buf = k_mpool_alloc(MPID_IRAM2, sizeof(Mailbox) + sizeof(Ring_buffer) + size + sizeof(MSGList) * 5);
    if(buf == NULL) {
        errno = ENOMEM;
        return RTX_ERR;
    }
    gp_current_task->mbx = buf;
    gp_current_task->mbx->buffer = (void *)((char *)buf + sizeof(Mailbox));
    gp_current_task->mbx->buffer->buff = (void *)((char *)buf + sizeof(Mailbox) + sizeof(Ring_buffer));
    gp_current_task->mbx->waitingList = (void *)((char *)buf + sizeof(Mailbox) + sizeof(Ring_buffer) + size);
    gp_current_task->mbx->buffer->length = (int)size;
    gp_current_task->mbx->buffer->head= 0;
    gp_current_task->mbx->buffer->tail= 0;
    gp_current_task->mbx->buffer->used= 0;
    gp_current_task->mbx->id = gp_current_task->tid;
    for(int i = 0 ; i < 5; i++) {

        gp_current_task->mbx->waitingList[i].head = NULL;
        gp_current_task->mbx->waitingList[i].tail= NULL;
    }

    return gp_current_task->tid;
}


int k_send_general(task_t receiver_tid, int preempt, const void *buf, int blocking) {

    if(buf == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    if(receiver_tid > 9 && receiver_tid != TID_UART) {
        errno = EINVAL;
        return RTX_ERR;
    }

    TCB *p_tcb;
    RTX_MSG_HDR *header = (RTX_MSG_HDR *) buf;
    
    if(header->length < MIN_MSG_SIZE) {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    Mailbox *mbx;
    if(receiver_tid == TID_UART) {
        mbx = uart_mbx;
    } else {
        p_tcb = &g_tcbs[receiver_tid];
        if(p_tcb == NULL) {
            errno = EINVAL;
            return RTX_ERR;
        }
        mbx = p_tcb->mbx;
    }
    if(mbx == NULL) {
        errno = ENOENT;
        return RTX_ERR;
    }
    if(header->length > mbx->buffer->length) {
        errno = EMSGSIZE;
        return RTX_ERR;
    }
    if(mbx->msgsWaiting > 0 && ((!preempt  && mbx->waitingList[0].head != NULL) || (preempt && mbx->waitingList[gp_current_task->prio %0x80].head != NULL))) {
        mbx->msgsWaiting++;
        int prio = 0;
        if(!preempt) {
            prio = 0x80;
        } else {
            prio = gp_current_task->prio;
        }
        Node *tempMsgBuf = k_mpool_alloc(MPID_IRAM2, sizeof(Node));
        tempMsgBuf->prev = NULL;
        tempMsgBuf->next= NULL;
        tempMsgBuf->info = k_mpool_alloc(MPID_IRAM2, header->length + MSG_HDR_SIZE);
        for(int i = 0; i < header->length; i++) {
            tempMsgBuf->info[i] = ((U8 *) buf)[i]; 
        }
        enqueueML(mbx->waitingList, tempMsgBuf, prio); 
        if(blocking) {
            block(1);
        } else {
            errno = ENOSPC;
            return RTX_ERR;
        }
    }
    if (receiver_tid != TID_UART && p_tcb->state == BLK_RECV) {

       // write it straight into the buffer
        int result = write_msg(mbx->buffer, buf);
        if(result < 0) {
        // buffer length is too long for mailbox
            errno = EMSGSIZE;
            return RTX_ERR;
        }
        p_tcb->state = READY;
        if(p_tcb->rt != NULL) {
            enqueue(&RTList, SORTED, p_tcb);
            if(gp_current_task->rt == NULL || p_tcb->rt->deadline < gp_current_task->rt->deadline) {
                k_tsk_run_new(SORTED);
            }
        } else {
            enqueue(&readyList[p_tcb->prio %0x80], BACK, p_tcb);
            if((p_tcb->prio < gp_current_task->prio) && preempt)  {
                k_tsk_yield();
            }
        }
    } else {
        int state = write_msg(mbx->buffer, buf);
        if(state < 0) {
            // there is not enough space in the mailbox, start the waiting queue
            Node *tempMsgBuf = k_mpool_alloc(MPID_IRAM2, sizeof(Node));
            tempMsgBuf->prev = NULL;
            tempMsgBuf->next= NULL;
            tempMsgBuf->info = k_mpool_alloc(MPID_IRAM2, header->length + MSG_HDR_SIZE);
            for(int i = 0; i < header->length; i++) {
                tempMsgBuf->info[i] = ((U8 *) buf)[i]; 
            }
            int prio = 0;
            if(preempt) {
                prio = gp_current_task->prio;
            }
            mbx->waitingList[prio % 0x80].head = tempMsgBuf;
            mbx->waitingList[prio % 0x80].tail = tempMsgBuf;
            mbx->msgsWaiting++;
            if(blocking) {
                block(1);
            } else {
                errno = ENOSPC;
                return RTX_ERR;
            }
        }
    }
    return 0;
}

int k_send_msg(task_t receiver_tid, const void *buf) {
#ifdef DEBUG_0
    printf("k_send_msg: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
    return k_send_general(receiver_tid, 1, buf, 1);
}

int k_send_msg_nb(task_t receiver_tid, const void *buf) {
#ifdef DEBUG_0
    printf("k_send_msg_nb: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
    return k_send_general(receiver_tid, 1, buf, 0);
}


int k_recv_msg_general(void *buf, size_t len, int blocking, int tid) {

    if(buf == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    Mailbox *mbx;
    if(tid == TID_UART) {
        mbx = uart_mbx;
    } else {
        mbx = gp_current_task->mbx;
    }
    if(mbx == NULL) {
        // no mailbox
        errno = ENOENT;
        return RTX_ERR;
    }
    while(mbx->msgsWaiting == 0 && mbx->buffer->used == 0) {
    // no messages, if we need to block we do so now 
        if(!blocking) {
            errno = ENOMSG;
            return RTX_ERR;
        }
        block(0);
    } 
    if(mbx->buffer->used == 0) {
        // we have messages waiting but none in the mailbox
        // we can assume that this wont error as we cant send a message bigger than a mail box
        write_msg_from_waiting_list(mbx, tid);
    }
    int result = pop_msg(mbx->buffer, buf, len);
    if(result < 0) {
        // either there is no messages in the mailbox or the length of the buffer is too small
        errno = ENOSPC;
        return RTX_ERR;
    }
    // message has been read into the buffer correctly
    write_msg_from_waiting_list(mbx, tid);
    return RTX_OK;
}
 
int k_recv_msg(void *buf, size_t len) {
#ifdef DEBUG_0
    printf("k_recv_msg: buf=0x%x, len=%d\r\n", buf, len);
#endif /* DEBUG_0 */
    return k_recv_msg_general(buf, len, 1, gp_current_task->tid);
}

int k_recv_msg_nb(void *buf, size_t len) {
#ifdef DEBUG_0
    printf("k_recv_msg_nb: buf=0x%x, len=%d\r\n", buf, len);
#endif /* DEBUG_0 */
    return k_recv_msg_general(buf, len, 0, gp_current_task->tid);
}
 

int k_mbx_ls(task_t *buf, size_t count) {
#ifdef DEBUG_0
    printf("k_mbx_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */
    if(buf == NULL || count == 0) {
        errno = EFAULT;
        return -1;
    }

    int mbxPresent = 0;
    for(int i = 0; i < 10; i++){
        TCB *p_tcb = &g_tcbs[i];
        if(p_tcb->mbx != NULL) {
            buf[mbxPresent] = p_tcb->tid;
            mbxPresent++;
            if(mbxPresent == count) {
                break;
            }
        }
    }
    return mbxPresent;
}

int k_mbx_get(task_t tid)
{
#ifdef DEBUG_0
    printf("k_mbx_get: tid=%u\r\n", tid);
#endif /* DEBUG_0 */
    Mailbox *mbx;
    if(tid != TID_UART) {
        TCB *p_tcb = &g_tcbs[tid];
        mbx = p_tcb->mbx;
    } else {
        mbx = uart_mbx;
    }
    if(mbx == NULL) {
        errno = ENOENT;
        return -1;
    }

    return (mbx->buffer->length - mbx->buffer->used);
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

