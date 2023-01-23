/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *                          All rights reserved.
 *
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
 ****************************************************************************
 */

/**************************************************************************//**
 * @file        k_task.c
 * @brief       task management C file
 * @version     V1.2021.05
 * @authors     Yiqing Huang
 * @date        2021 MAY
 *
 * @attention   assumes NO HARDWARE INTERRUPTS
 * @details     The starter code shows one way of implementing context switching.
 *              The code only has minimal sanity check.
 *              There is no stack overflow check.
 *              The implementation assumes only three simple tasks and
 *              NO HARDWARE INTERRUPTS.
 *              The purpose is to show how context switch could be done
 *              under stated assumptions.
 *              These assumptions are not true in the required RTX Project!!!
 *              Understand the assumptions and the limitations of the code before
 *              using the code piece in your own project!!!
 *
 *****************************************************************************/


#include "k_inc.h"
#include "k_task.h"
#include "k_rtx.h"
#include "k_mem.h"

/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */
TCBList         readyList[5];
TCBList RTList;
TCBList suspendedList;

TCB             *gp_current_task = NULL;    // the current RUNNING task
TCB             g_tcbs[MAX_TASKS];          // an array of TCBs
TASK_INIT       g_null_task_info;           // The null task info
U32             g_num_active_tasks = 0;     // number of non-dormant tasks

/*---------------------------------------------------------------------------
The memory map of the OS image may look like the following:
                   RAM1_END-->+---------------------------+ High Address
                              |                           |
                              |                           |
                              |       MPID_IRAM1          |
                              |   (for user space heap  ) |
                              |                           |
                 RAM1_START-->|---------------------------|
                              |                           |
                              |  unmanaged free space     |
                              |                           |
&Image$$RW_IRAM1$$ZI$$Limit-->|---------------------------|-----+-----
                              |         ......            |     ^
                              |---------------------------|     |
                              |      PROC_STACK_SIZE      |  OS Image
              g_p_stacks[2]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[1]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |                           |  OS Image
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                
    g_k_stacks[MAX_TASKS-1]-->|---------------------------|     |
                              |                           |     |
                              |     other kernel stacks   |     |                              
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |  OS Image
              g_k_stacks[2]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                      
              g_k_stacks[1]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |
              g_k_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |---------------------------|     |
                              |        TCBs               |  OS Image
                      g_tcbs->|---------------------------|     |
                              |        global vars        |     |
                              |---------------------------|     |
                              |                           |     |          
                              |                           |     |
                              |        Code + RO          |     |
                              |                           |     V
                 IRAM1_BASE-->+---------------------------+ Low Address
    
---------------------------------------------------------------------------*/ 
//TODO: NOTE WE CAN REFER TO THE USER STACK POINTER BY ksp[0]
/*
 *===========================================================================
 *                           QUEUE FUNCTIONS 
 *===========================================================================
 */
// side = 1 -> tail, side = 0 -> head
/*
    side = 0
    1. check head = null, if yes assign to head, if not set this tcb's next to the current head and 
    set that head's prev to this tcb and then set the current head to this tcb
    2. check tail == NULL. if yes assign this to tail as well
    side = 1
    1. check tail == null, if yes do the head procedure
    2. check head == null if yes do the procedure agfain

*/

int enqueue(TCBList *list, int side, TCB *task) {
    if(list->head == NULL && list->tail == NULL){
        task->prev = NULL;
        task->next = NULL;
        list->head = task;
        list->tail = task;
    } else if(side == FRONT) {
        TCB *temp = list->head;
        task->next = temp;
        task->prev = NULL;
        temp->prev = task;
        list->head = task;
    } else if(side == BACK) {
        TCB *temp = list->tail;
        task->prev = temp;
        task->next = NULL;
        temp->next = task;
        list->tail = task;
    } else if(side == SORTED) {
        TCB *temp = list->head;
        while(temp != NULL) {
            if(temp->rt->deadline > task->rt->deadline) {
                if(temp->prev != NULL) {
                    temp->prev->next = task;
                    task->prev = temp->prev;
                    temp->prev = task;
                    task->next = temp;
                } else {
                    // This is the head
                    temp->prev = task;
                    task->next = temp;
                    list->head = task;
                }
                temp = NULL;
            } else if(temp == list->tail) {
                temp->next = task;
                task->prev = temp;
                list->tail = task;
                temp = NULL;
            } else {
                temp = temp->next;
            }
        } 
    } else {
        // Side is incorrect
        return 0;
    }
    return 1;
}

TCB *dequeue(TCBList *list) {
    if(list->head == NULL) {
        return NULL;
    } else {
        TCB *temp = list->head;
        if(list->head->next != NULL){
            list->head->next->prev = NULL;
            list->head = list->head->next;
            temp->next = NULL;
        } else {
            // This node is also the tail
            list->head = NULL;
            list->tail = NULL;
        }
        return temp;
    }
}


/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */


/**************************************************************************//**
 * @brief   scheduler, pick the TCB of the next to run task
 *
 * @return  TCB pointer of the next to run task
 * @post    gp_curret_task is updated
 * @note    you need to change this one to be a priority scheduler
 *
 *****************************************************************************/
// Note we assume that before calling scheduler there is no task currently running
TCB *scheduler(int side)
{
    // Try to pop a ready RT task, if not possible then continue
    if(RTList.head != NULL) {
        if(gp_current_task->rt == NULL || RTList.head->rt->deadline < gp_current_task->rt->deadline || gp_current_task->state == SUSPENDED){
            TCB *temp1 = dequeue(&RTList);
            return temp1;
        } else {
            return gp_current_task;
        }
    } else if(gp_current_task->rt != NULL && gp_current_task->state == RUNNING) {
        return gp_current_task;
    }

    int limit = (gp_current_task != NULL && gp_current_task->state == RUNNING && gp_current_task->prio != PRIO_RT  ? (gp_current_task->prio == 0xFF ? 5 : gp_current_task->prio % 0x80) : 5);
    if(side == BACK) { 
        limit = limit + 1;
    }
    // As there is always a null task in the readylist[4] slot, we can assume we will always find a task to send
    for(int i = 0; i < limit; i++) {
        TCB *temp2 = dequeue(&readyList[i]);
        if(temp2 != NULL){
            return temp2;
        }
    }
    return gp_current_task;
}
/**
 * @brief initialzie the first task in the system
 */
void k_tsk_init_first(TASK_INIT *p_task)
{
    p_task->prio         = PRIO_NULL;
    p_task->priv         = 0;
    p_task->tid          = TID_NULL;
    p_task->ptask        = &task_null;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

// TODO: Consider increasing size of these tasks
void k_tsk_init_kcd_con(TASK_INIT *kcd, TASK_INIT *con, TASK_INIT *wclck) {
    kcd->prio = HIGH;
    kcd->priv = 0;
    kcd->tid = TID_KCD;
    kcd->ptask = &task_kcd;
    kcd->u_stack_size = PROC_STACK_SIZE;

    wclck->prio = HIGH;
    wclck->priv = 0;
    wclck->tid = TID_WCLCK;
    wclck->ptask = &task_wall_clock;
    wclck->u_stack_size = PROC_STACK_SIZE;

    con->prio = HIGH;
    con->priv = 1;
    con->tid = TID_CON;
    con->ptask = &task_cdisp;
    con->u_stack_size = PROC_STACK_SIZE;

}

/**************************************************************************//**
 * @brief       initialize all boot-time tasks in the system,
 *
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       task_info   boot-time task information structure pointer
 * @param       num_tasks   boot-time number of tasks
 * @pre         memory has been properly initialized
 * @post        none
 * @see         k_tsk_create_first
 * @see         k_tsk_create_new
 *****************************************************************************/
int k_tsk_init(TASK_INIT *task, int num_tasks)
{
    if (num_tasks > MAX_TASKS - 1) {
        return RTX_ERR;
    }

    TASK_INIT taskinfo;
    TASK_INIT kcdtask;
    TASK_INIT contask;
    TASK_INIT wclcktask;
    
    k_tsk_init_first(&taskinfo);
    if ( k_tsk_create_new(&taskinfo, &g_tcbs[TID_NULL], TID_NULL) == RTX_OK ) {
        g_num_active_tasks = 1;
        gp_current_task = &g_tcbs[TID_NULL];
    } else {
        g_num_active_tasks = 0;
        return RTX_ERR;
    }

    k_tsk_init_kcd_con(&kcdtask, &contask, &wclcktask);
    if ( k_tsk_create_new(&kcdtask, &g_tcbs[TID_KCD], TID_KCD) == RTX_OK ) {
        g_num_active_tasks++;
        enqueue(&readyList[0],BACK, &g_tcbs[TID_KCD]);
    }
    if ( k_tsk_create_new(&contask, &g_tcbs[TID_CON], TID_CON) == RTX_OK ) {
        g_num_active_tasks++;
        enqueue(&readyList[0],BACK, &g_tcbs[TID_CON]);
    }
    if ( k_tsk_create_new(&wclcktask, &g_tcbs[TID_WCLCK], TID_WCLCK) == RTX_OK ) {
        g_num_active_tasks++;
        enqueue(&readyList[0],BACK, &g_tcbs[TID_WCLCK]);
    }
    
    // create the rest of the tasks
    
    for ( int i = 0; i < num_tasks; i++ ) {
        TCB *p_tcb = &g_tcbs[i+1];
        if (k_tsk_create_new(&task[i], p_tcb, i+1) == RTX_OK) {
            enqueue(&readyList[(&task[i])->prio % 0x80], BACK, p_tcb);
            g_num_active_tasks++;
        }
    }
    
    return RTX_OK;
}
/**************************************************************************//**
 * @brief       initialize a new task in the system,
 *              one dummy kernel stack frame, one dummy user stack frame
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       p_taskinfo  task initialization structure pointer
 * @param       p_tcb       the tcb the task is assigned to
 * @param       tid         the tid the task is assigned to
 *
 * @details     From bottom of the stack,
 *              we have user initial context (xPSR, PC, SP_USR, uR0-uR3)
 *              then we stack up the kernel initial context (kLR, kR4-kR12, PSP, CONTROL)
 *              The PC is the entry point of the user task
 *              The kLR is set to SVC_RESTORE
 *              20 registers in total
 * @note        YOU NEED TO MODIFY THIS FILE!!!
 *****************************************************************************/
int k_tsk_create_new(TASK_INIT *p_taskinfo, TCB *p_tcb, task_t tid)
{
    extern U32 SVC_RTE;

    U32 *usp;
    U32 *ksp;

    int size =  (p_taskinfo->u_stack_size < PROC_STACK_SIZE ? PROC_STACK_SIZE : exponent(2, log_ceil(p_taskinfo->u_stack_size)));
    if (p_taskinfo == NULL || p_tcb == NULL)
    {
        return RTX_ERR;
    }
    if(p_taskinfo->prio == PRIO_NULL) {
        usp = (U32 *)(__get_PSP() + 32);
    } else {
        usp = k_alloc_user_stack(size,tid);             
    if (usp == NULL) {
        return RTX_ERR;
    }
    }
    p_tcb->tid   = tid;
    p_tcb->prio  = p_taskinfo->prio;
    p_tcb->priv  = p_taskinfo->priv;
    p_tcb->ptask = p_taskinfo->ptask;
    /*---------------------------------------------------------------
     *  Step1: allocate user stack for the task
     *         stacks grows down, stack base is at the high address
     * -------------------------------------------------------------*/
   
    p_tcb->state = READY;
    p_tcb->lowu = (int)usp;

    p_tcb->usize = size;
    /*-------------------------------------------------------------------
     *  Step2: create task's thread mode initial context on the user stack.
     *         fabricate the stack so that the stack looks like that
     *         task executed and entered kernel from the SVC handler
     *         hence had the exception stack frame saved on the user stack.
     *         This fabrication allows the task to return
     *         to SVC_Handler before its execution.
     *
     *         8 registers listed in push order
     *         <xPSR, PC, uLR, uR12, uR3, uR2, uR1, uR0>
     * -------------------------------------------------------------*/

    // if kernel task runs under SVC mode, then no need to create user context stack frame for SVC handler entering
    // since we never enter from SVC handler in this case
    *(--usp) = INITIAL_xPSR;             // xPSR: Initial Processor State
    *(--usp) = (U32) (p_taskinfo->ptask);// PC: task entry point
        
    // uR14(LR), uR12, uR3, uR3, uR1, uR0, 6 registers
    for ( int j = 0; j < 6; j++ ) {
        
#ifdef DEBUG_0
        *(--usp) = 0xDEADAAA0 + j;
#else
        *(--usp) = 0x0;
#endif
    }

    p_tcb->usp = usp;
    
    // allocate kernel stack for the task
    ksp = k_alloc_k_stack(tid);
    if ( ksp == NULL ) {
        return RTX_ERR;
    }

    p_tcb->lowk = (int)ksp;

    /*---------------------------------------------------------------
     *  Step3: create task kernel initial context on kernel stack
     *
     *         12 registers listed in push order
     *         <kLR, kR4-kR12, PSP, CONTROL>
     * -------------------------------------------------------------*/
    // a task never run before directly exit
    *(--ksp) = (U32) (&SVC_RTE);
    // kernel stack R4 - R12, 9 registers
#define NUM_REGS 9    // number of registers to push
      for ( int j = 0; j < NUM_REGS; j++) {        
#ifdef DEBUG_0
        *(--ksp) = 0xDEADCCC0 + j;
#else
        *(--ksp) = 0x0;
#endif
    }
        
    // put user sp on to the kernel stack
    *(--ksp) = (U32) usp;
    
    // save control register so that we return with correct access level
    if (p_taskinfo->priv == 1) {  // privileged 
        *(--ksp) = __get_CONTROL() & ~BIT(0); 
    } else {                      // unprivileged
        *(--ksp) = __get_CONTROL() | BIT(0);
    }

    p_tcb->msp = ksp;
    
    return RTX_OK;
}

/**************************************************************************//**
 * @brief       switching kernel stacks of two TCBs
 * @param       p_tcb_old, the old tcb that was in RUNNING
 * @return      RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre         gp_current_task is pointing to a valid TCB
 *              gp_current_task->state = RUNNING
 *              gp_crrent_task != p_tcb_old
 *              p_tcb_old == NULL or p_tcb_old->state updated
 * @note        caller must ensure the pre-conditions are met before calling.
 *              the function does not check the pre-condition!
 * @note        The control register setting will be done by the caller
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *****************************************************************************/
// TODO: Edit this to get the user stack pointer and save user stack pointer as well
__asm void k_tsk_switch(TCB *p_tcb_old)
{
        PRESERVE8
        EXPORT  K_RESTORE
        
        PUSH    {R4-R12, LR}                // save general pupose registers and return address
        MRS     R4, CONTROL                 
        MRS     R5, PSP
        PUSH    {R4-R5}                     // save CONTROL, PSP
        STR     SP, [R0, #TCB_MSP_OFFSET]   // save SP to p_old_tcb->msp
K_RESTORE
        LDR     R1, =__cpp(&gp_current_task)
        LDR     R2, [R1]
        LDR     SP, [R2, #TCB_MSP_OFFSET]   // restore msp of the gp_current_task
        POP     {R4-R5}
        MSR     PSP, R5                     // restore PSP
        MSR     CONTROL, R4                 // restore CONTROL
        ISB                                 // flush pipeline, not needed for CM3 (architectural recommendation)
        POP     {R4-R12, PC}                // restore general purpose registers and return address
}


__asm void k_tsk_start(void)
{
        PRESERVE8
        B K_RESTORE
}

/**************************************************************************//**
 * @brief       run a new thread. The caller becomes READY and
 *              the scheduler picks the next ready to run task.
 * @return      RTX_ERR on error and zero on success
 * @pre         gp_current_task != NULL && gp_current_task == RUNNING
 * @post        gp_current_task gets updated to next to run task
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *****************************************************************************/
// TODO: Change this to support using any list rather than just readylist
int k_tsk_run_new(int side)
{
    TCB *p_tcb_old = NULL;
    
    if (gp_current_task == NULL) {
        return RTX_ERR;
    }

    p_tcb_old = gp_current_task;
        p_tcb_old->usp = (U32 *)__get_PSP();
        //p_tcb_old->msp = (U32 *)__get_MSP();
    gp_current_task = scheduler(side);
    
    if ( gp_current_task == NULL  ) {
        gp_current_task = p_tcb_old;        // revert back to the old task
        return RTX_ERR;
    }

    // at this point, gp_current_task != NULL and p_tcb_old != NULL
    // TODO: we are accidentally pushing non rt tasks to the lists
    if (gp_current_task != p_tcb_old) {
        gp_current_task->state = RUNNING;   // change state of the to-be-switched-in  tcb
        if(side == SORTED && p_tcb_old->rt!= NULL) {
            if(p_tcb_old->state != SUSPENDED) {
                p_tcb_old->state = READY;
            }
            enqueue((p_tcb_old->state == SUSPENDED ? &suspendedList : &RTList), side, p_tcb_old);
        } else {
            if(p_tcb_old->state == RUNNING || p_tcb_old->state == READY) {
            p_tcb_old->state = READY;           // change state of the to-be-switched-out tcb
            enqueue(&readyList[(p_tcb_old->prio != 0xFF ? p_tcb_old->prio % 0x80 : 4)], side, p_tcb_old);
            }
        }
        k_tsk_switch(p_tcb_old);            // switch kernel stacks       
    }

    return RTX_OK;
}

 
/**************************************************************************//**
 * @brief       yield the cpu
 * @return:     RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre:        gp_current_task != NULL &&
 *              gp_current_task->state = RUNNING
 * @post        gp_current_task gets updated to next to run task
 * @note:       caller must ensure the pre-conditions before calling.
 *****************************************************************************/

int k_tsk_yield(void)
{
    if(gp_current_task->rt != NULL) {
        return 0;
    }
    return k_tsk_run_new(BACK);
}

/**
 * @brief   get task identification
 * @return  the task ID (TID) of the calling task
 */
task_t k_tsk_gettid(void)
{
    return gp_current_task->tid;
}

/*
 *===========================================================================
 *                             TO BE IMPLEMETED IN LAB2
 *===========================================================================
 */

int k_tsk_create(task_t *task, void (*task_entry)(void), U8 prio, U32 stack_size)
{
#ifdef DEBUG_0
    printf("k_tsk_create: entering...\n\r");
    printf("task = 0x%x, task_entry = 0x%x, prio=%d, stack_size = %d\n\r", task, task_entry, prio, stack_size);
#endif /* DEBUG_0 */
if(prio != PRIO_NULL && (prio % 0x80 > 3 || prio/0x80 > 1)){
    errno = EINVAL;
    return RTX_ERR;
}
    TASK_INIT info;
    info.prio =  (int) prio;
    info.priv = 0;
    info.ptask = task_entry;
    U32 size = exponent(2, log_ceil(stack_size));
    info.u_stack_size = size;

    for(int i = 1; i < 10; i++){
			
        TCB *p_tcb = &g_tcbs[i];
        if(p_tcb->state == DORMANT || p_tcb->state == NULL) {
            if(k_tsk_create_new(&info, p_tcb, i) == RTX_OK) {
			*task = i;
                g_num_active_tasks++;
                enqueue(&readyList[prio % 0x80], BACK, p_tcb);
                if(prio < gp_current_task->prio) {
                    // Involuntary interrupt
                    k_tsk_run_new(FRONT);
                }
                return RTX_OK;
            } else {
                errno = ENOMEM;
                return RTX_ERR;
            }
        }
    }
    errno = EAGAIN;
    return RTX_ERR;

}

void k_tsk_exit(void) 
{
#ifdef DEBUG_0
    printf("k_tsk_exit: entering...\n\r");
#endif /* DEBUG_0 */
    

   g_num_active_tasks--;
   gp_current_task->state = DORMANT;
	if(gp_current_task->rt != NULL) {
        if(gp_current_task->rt->deadline < g_timer_count) {
        printf("Job %d of task %d missed its deadline", gp_current_task->rt->completed, gp_current_task->tid);
        }
        k_mpool_dealloc(MPID_IRAM2, gp_current_task->rt);
        gp_current_task->rt = NULL;
	}
   TCB *old = gp_current_task;
   gp_current_task = scheduler(FRONT);
   gp_current_task->state = RUNNING;
   k_mpool_dealloc(MPID_IRAM2, (void *)(old->lowu - old->usize));
  if(old->mbx != NULL) { 
	k_mpool_dealloc(MPID_IRAM2, old->mbx);
		  old->mbx = NULL;
	}
   old->next = NULL;
   old->prev = NULL;
 
   k_tsk_switch(old);


    return;
}

int k_tsk_set_prio(task_t task_id, U8 prio) 
{
#ifdef DEBUG_0
    printf("k_tsk_set_prio: entering...\n\r");
    printf("task_id = %d, prio = %d.\n\r", task_id, prio);
#endif /* DEBUG_0 */

    if(task_id > 9 || (task_id == TID_NULL && prio != PRIO_NULL)) {
        errno= EINVAL;
        return RTX_ERR;
    } 
    TCB *p_tcb = &g_tcbs[task_id];
    // Checking if this is a dormant task
    if(p_tcb->state == DORMANT) {
        return RTX_OK;
    } else if(prio == PRIO_NULL) {
        // We can only set null task to null
        if(task_id == TID_NULL) {
            return RTX_OK;
        } else {
            errno = EINVAL;
            return RTX_ERR;
        }
    }else if(p_tcb->rt == NULL && prio == PRIO_RT) {
        errno = EPERM;
        return RTX_ERR;
    } else if(p_tcb->rt != NULL ) {
        if( prio != PRIO_RT) {
            errno = EPERM;
            return RTX_ERR;
        } else {
            return RTX_OK;
        }
    } else if(prio % 0x80 < 4 && prio/0x80 == 1) {
        // We have a correct priority value
            
            if(p_tcb->priv == 1) {
                // If this task is priveleged and we are not priveleged, error
                if(gp_current_task->priv == 0) {
                    errno = EPERM;
                    return RTX_ERR;
                }
            } 
            U8 original = p_tcb->prio;
            p_tcb->prio = prio;
            // If this is the running task, we check if the new priority is less than the old one and try running a new task if it is
            if(p_tcb->state == RUNNING) {
                if(original < prio) {
                    k_tsk_run_new(BACK);
                }
            } else {
                // Dequeue this task from it's current queue
                int level = original% 0x80;
                TCB *tmp = readyList[level].head;
                while(tmp != NULL) {
                    if(tmp->tid == task_id) {
                        // Found the task
                        if(readyList[level].head == tmp) {
                            // task is head, check if there are more tasks in the ready list
                            if(tmp->next != NULL) {
                                tmp->next->prev = NULL;
                                readyList[level].head = tmp->next;
                            } else {
                                // As there are no more tasks in the readylist we can assume that the tail should be null too
                                readyList[level].head = NULL;
                                readyList[level].tail = NULL;
                            }
                        } else if(readyList[level].tail == tmp) {
                            // We assume there are more than 1 tasks in the list as this node is not the head, but it is the tail
                            tmp->prev->next = NULL;
                            readyList[level].tail = tmp->prev;
                        } else {
                            // This task is not the head or the tail
                            tmp->prev->next = tmp->next;
                            tmp->next->prev = tmp->prev;
                        }
                        break;
                    } else {
                        tmp = tmp->next;
                    }
                }
                level = prio %0x80;
                // Enqueue the task at the back of it's new level, if it is higher priority it'll be the only task in it's readylist, else it will be in the right spot
                tmp->next = NULL;
                tmp->prev = NULL;
                enqueue(&readyList[level], BACK, tmp);
                if(prio < gp_current_task->prio) {
                    // we want to run this task instead
                    k_tsk_run_new(FRONT);
                } 
            }
            return RTX_OK;
    } else {
        errno = EINVAL;
        return RTX_ERR;
    }
}

/**
 * @brief   Retrieve task internal information 
 * @note    this is a dummy implementation, you need to change the code
 */
int k_tsk_get(task_t tid, RTX_TASK_INFO *buffer)
{
#ifdef DEBUG_0
    printf("k_tsk_get: entering...\n\r");
    printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */    
    
    if(buffer == NULL) {
        errno = EFAULT;
        return -1;
    } else if (tid > 9) {
        errno = EINVAL;
        return -1;
    }

    TCB *tmp = &g_tcbs[tid];
    buffer->tid           = tid;
    buffer->prio          = tmp->prio;
    buffer->u_stack_size  = tmp->usize; 
    buffer->priv          = tmp->priv;
    buffer->ptask         = tmp->ptask;
    buffer->k_sp          = (tmp->state == RUNNING ? __get_MSP(): (U32)tmp->msp);
    buffer->k_sp_base     = tmp->lowk- 256;
    buffer->k_stack_size  = 256;
    buffer->state         = tmp->state;
    buffer->u_sp          = (tmp->state == RUNNING ? __get_PSP() : (U32)tmp->usp);
    buffer->u_sp_base     = tmp->lowu - tmp->usize; 
    return RTX_OK;     
}

int k_tsk_ls(task_t *buf, size_t count){
#ifdef DEBUG_0
    printf("k_tsk_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */
    // We want the task id's of the first "count" non-dormant tasks in the system

    
    if(buf == NULL || count == 0) {
        errno = EFAULT;
        return -1;
    }

    int nonDormant = 0;    
    for(int i = 0; i < 10; i++) {
        TCB *p_tcb = &g_tcbs[i];
        if(p_tcb->state != DORMANT) {
            buf[nonDormant] = p_tcb->tid;
            nonDormant++;
            if(nonDormant == count) {
                break;
            }
        }
    }
    return nonDormant;
}

int k_rt_tsk_set(TIMEVAL *p_tv)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_set: p_tv = 0x%x\r\n", p_tv);
#endif /* DEBUG_0 */
// if secs * 1000000 + usecs % 2500 != 0 we dont have a multiple
    if((p_tv->sec * 1000000 + p_tv->usec) % 2500 != 0) {
        errno = EINVAL;
        return RTX_ERR;
    }
// 1 tick = 500 usec, 1 000 000 usec = 1 sec, 1 sec/ 1 tick = 2000 => 2000 ticks = 1 sec
U32 ticks = p_tv->sec * 2000 + p_tv->usec / 500;
// min period is 2500 usec, if (usec + secE-6) % minPeriod != 0 error
    if(ticks == 0) {
        errno = EINVAL;
        return RTX_ERR;
    }

    if(gp_current_task->rt != NULL) {
        errno = EPERM;
        return RTX_ERR;
    }

    gp_current_task->rt = k_mpool_alloc(MPID_IRAM2, sizeof(RT_Info));
    if(gp_current_task->rt == NULL) {
        errno = ENOMEM;
        return RTX_ERR;
    }

    gp_current_task->prio = PRIO_RT;
    gp_current_task->rt->ticks = ticks;
    gp_current_task->rt->completed = 0;
    gp_current_task->rt->deadline = g_timer_count + ticks;

    return RTX_OK;   
}

int k_rt_tsk_susp(void)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_susp: entering\r\n");
#endif /* DEBUG_0 */
    if(gp_current_task->rt == NULL) {
        errno = EPERM;
        return RTX_ERR;
    }
    RT_Info *info = gp_current_task->rt;

    info->completed++;
    if(info->deadline > g_timer_count) {
        // We have completed before the deadline, we can simply suspend this task
        gp_current_task->state = SUSPENDED;
    } else {
        // We are overdeadline, just increment deadline and put it back in the rt list
        // Check if we are exactly on deadline
        if(info->deadline != g_timer_count) {
            printf("Job %d of task %d missed its deadline\r\n", gp_current_task->rt->completed, gp_current_task->tid);
        }
        while(info->deadline <= g_timer_count) {
            info->deadline = info->deadline + info->ticks;
        }
    }
    k_tsk_run_new(SORTED);
    return RTX_OK;
}

int k_rt_tsk_unsusp(void) {
    int tasks = 0;
    TCB *temp = suspendedList.head;
    while(temp != NULL && temp->rt->deadline <= g_timer_count) {
        TCB *susTCB = dequeue(&suspendedList);
        // Update the tasks deadlines and queue it back into the RTList
        // We deal with overload here
        while(susTCB->rt->deadline <= g_timer_count) {
            susTCB->rt->deadline = susTCB->rt->deadline + susTCB->rt->ticks;
        }
        susTCB->state = READY;
        enqueue(&RTList, SORTED, susTCB);
        tasks++;
        temp = temp->next;
    }
    return tasks;
}

int k_rt_tsk_get(task_t tid, TIMEVAL *buffer)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_get: entering...\n\r");
    printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */    
    if (buffer == NULL) {
        return RTX_ERR;
    }   
    TCB *p_tcb = &g_tcbs[tid]; 
    if(p_tcb->rt == NULL) {
        errno = EINVAL;
        return RTX_ERR;
    }
    if(gp_current_task->rt == NULL) {
        errno = EPERM;
        return RTX_ERR;
    }

    int period = p_tcb->rt->ticks;
    buffer->sec = period / 2000;
    buffer->usec = (period - (buffer->sec * 2000)) * 500;
    
    return RTX_OK;
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
