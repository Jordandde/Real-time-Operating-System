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
 * @file        k_mem.c
 * @brief       Kernel Memory Management API C Code
 *
 * @version     V1.2021.01.lab2
 * @authors     Yiqing Huang
 * @date        2021 JAN
 *
 * @note        skeleton code
 *
 *****************************************************************************/

#include "k_inc.h"
#include "k_mem.h"

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
                              |                           |     |
                              |---------------------------|     |
                              |                           |     |
                              |      other data           |     |
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
             g_k_stacks[15]-->|---------------------------|     |
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

/*
 *===========================================================================
 *                            GLOBAL VARIABLES
 *===========================================================================
 */
// kernel stack size, referred by startup_a9.s
const U32 g_k_stack_size = KERN_STACK_SIZE;
// task proc space stack size in bytes, referred by system_a9.c
const U32 g_p_stack_size = PROC_STACK_SIZE;

// task kernel stacks
U32 g_k_stacks[MAX_TASKS][KERN_STACK_SIZE >> 2] __attribute__((aligned(8)));

// task process stack (i.e. user stack) for tasks in thread mode
// remove this bug array in your lab2 code
// the user stack should come from MPID_IRAM2 memory pool
/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */
U8 bit_tree_1[32];
U8 bit_tree_2[256];



DLIST free_list_1[8];
DLIST free_list_2[11];

/* note list[n] is for blocks with order of n */
mpool_t k_mpool_create (int algo, U32 start, U32 end)
{
    mpool_t mpid = MPID_IRAM1;

#ifdef DEBUG_0
    printf("k_mpool_init: algo = %d\r\n", algo);
    printf("k_mpool_init: RAM range: [0x%x, 0x%x].\r\n", start, end);
#endif /* DEBUG_0 */    
    
    if (algo != BUDDY ) {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    if ( start == RAM1_START) {

	DNODE *ptr = (void *) RAM1_START;
        ptr->next = NULL;
	ptr->prev = NULL;

        free_list_1[0].head = (void *)((char *)ptr);

    } else if ( start == RAM2_START) {
        mpid = MPID_IRAM2;

	DNODE *ptr = (void *) RAM2_START;
	ptr->next = NULL;
	ptr->prev = NULL;
        free_list_2[0].head = (void *)((char *)ptr);
    } else {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    return mpid;
}

int CheckBit(U8 bit_tree[], int pos){
	U8 shifted = 1 << (pos%8);
	return ((bit_tree[pos/8] & shifted) != 0);
}

int log_ceil (size_t size) {
    int i = 0;
    int ones = size&1;
    while(size >>= 1) {
        if( size&1 ) {
            ones++;
        }
        i++;
    }
    if(ones > 1) {
        i++;
    }
    return i;
}

int exponent(int base, int exp){
    if(exp == 0){
        return 1;
    }
    int ans = base;
    for(int i = 1; i < exp; i++){
	ans *= base;
    }
    return ans;
}

void FlipBit(U8 bit_tree[], int level, int address, int base, int size) {

    int basicAddress= exponent(2,level) - 1 + ((int)address - base)/size;
    int addFirst = basicAddress / 8;
    int addSecond = basicAddress % 8;


    bit_tree[addFirst] ^= (1 << addSecond);
}

void *LookForMemory(DLIST free_list[], U8 bit_tree[], int level, mpool_t mpid) {
    // check if we have space
    int exp = (mpid == MPID_IRAM1 ? RAM1_SIZE_LOG2 : RAM2_SIZE_LOG2) - level;
    int base = (mpid == MPID_IRAM1 ? RAM1_START: RAM2_START);
    int size = exponent(2, exp);
    if(free_list[level].head == NULL) {
        if(level == 0) {
            return 0;
        }
        // look for a memory block higher, if we don't receive 0, split the block, add the second half of the block as the head of this tree and then return the first half of the block's address/ptr 
        int lower = level - 1;
        void *mem = LookForMemory(free_list, bit_tree, lower, mpid);
        if((int)mem != 0) {
            // we have received memory
            FlipBit(bit_tree, level, (int)mem, base, size);
            // create a new block with the second half of the memory
            DNODE *ptr = (void *)((char *)mem + size);
            ptr->next = NULL;
            ptr->prev = NULL;
            free_list[level].head = (void *)((char *)ptr);
            return mem;
        }
        return 0;
    }
    // We have memory, return it 
    DNODE *temp1 = (void *)free_list[level].head;
    // Address - base address = position
		
    FlipBit(bit_tree, level, (int)temp1, base, size);
    
    if(free_list[level].head->next != NULL) {
			
        free_list[level].head->next->prev = NULL;
	free_list[level].head = free_list[level].head->next;
    } else {
        free_list[level].head = NULL;
    }
    return temp1;
}

void *k_mpool_alloc (mpool_t mpid, size_t size)
{
#ifdef DEBUG_0
    printf("k_mpool_alloc: mpid = %d, size = %d, 0x%x\r\n", mpid, size, size);
#endif /* DEBUG_0 */

    
    if(mpid != MPID_IRAM1 && mpid != MPID_IRAM2) {
        errno = EINVAL;
        return (void*) NULL;
    }

    int sizeNeededLog = log_ceil(size);
    int maxSizeLog = (mpid == MPID_IRAM1 ? RAM1_SIZE_LOG2: RAM2_SIZE_LOG2 );
    // Edge case accomodation block
    if(sizeNeededLog < 5){
        // Accomodate minimum size of a block, 32 or 2^5
        sizeNeededLog = 5;
    } else if(sizeNeededLog > maxSizeLog) {
        errno = ENOMEM;
        return (void*) NULL;
    }

    int desired = maxSizeLog - sizeNeededLog;

    void *ans = LookForMemory((mpid == MPID_IRAM1 ? free_list_1: free_list_2), (mpid == MPID_IRAM1 ? bit_tree_1: bit_tree_2), desired, mpid);
    return ans;
}

void AddToFreeList(DNODE *curr, int level, DNODE *ptr) {
    if((int) curr < (int)ptr) {
        // we found our place to insert
        if(curr->next != NULL) {
            ptr->next = (void *)curr->next;
            curr->next->prev = (void *)ptr;
        }
        curr->next = (void *)ptr;
        ptr->prev = (void *)curr;
        return;
    }
    AddToFreeList(curr->next, level, ptr);
}

void RemoveFromFreeList(DNODE *curr, int level, int address) {
    if((int) curr == address){
        curr->prev->next = curr->next;
        if(curr->next != NULL) {
            curr->next->prev = curr->prev;
        }
        curr = NULL;
        return;
    } 
    DNODE *temp2 = (void *)curr->next;
    RemoveFromFreeList(temp2, level, address);
    return;
}
int GetRelativePos(mpool_t mpid, int level, int ptr) {
    int exp = (mpid == MPID_IRAM1 ? RAM1_SIZE_LOG2 : RAM2_SIZE_LOG2) - level;
    int base = (mpid == MPID_IRAM1 ? RAM1_START: RAM2_START);
    int size = exponent(2, exp);
    int pos = ((ptr - base)/ size);
    return pos;
}
int Coalesce(DLIST free_list[],U8 bit_tree[], DNODE *ptr, /*int pos,*/ int level, int size, mpool_t mpid) {
    // pos should be pos relative to level
    int relPos = GetRelativePos(mpid, level, (int)ptr);
    int base = (mpid == MPID_IRAM1 ? RAM1_START: RAM2_START);
    int pos = relPos + exponent(2,level) - 1;
    int buddy = exponent(2,level) - 1 + (relPos ^ 1);
    if(level == 0 || CheckBit(bit_tree,buddy) != 0 ) {
        return 0;
    }
    // Coalesce
    if(free_list[level].head == ptr) {
        free_list[level].head = free_list[level].head->next;
        free_list[level].head->prev =NULL;
    } else {
        RemoveFromFreeList(free_list[level].head, level, (int)ptr);
    }
    int buddyAdd= (int)ptr;
    int firstAdd = (int)ptr;
    int lower = level - 1;
    if(buddy > pos){
        // buddy is right node
        buddyAdd = buddyAdd + size;
    } else {
        // buddy comes before this node address-wise
        buddyAdd = buddyAdd - size;
        firstAdd = buddyAdd;
    }

    if((int)free_list[level].head == buddyAdd) {
        if(free_list[level].head->next != NULL) {
            free_list[level].head->next->prev = NULL;
        }
        free_list[level].head = free_list[level].head->next;
    } else {
        RemoveFromFreeList(free_list[level].head, level, buddyAdd);
    }

    DNODE *ptr2 = (void *)firstAdd;
    ptr2->next = NULL;
    int sizeParents = size * 2;

    FlipBit(bit_tree, lower, firstAdd, base, sizeParents);
    if(free_list[lower].head == NULL) {
        free_list[lower].head = ptr2;
        // Coalescing more from here is impossible, return
        return 0;
    } else if(free_list[lower].head > ptr2) {
        DNODE *temp3 = free_list[lower].head;
        temp3->prev = ptr2;
        ptr2->next = temp3;
        free_list[lower].head = ptr2;
    } else {
        AddToFreeList(free_list[lower].head, lower, ptr2);
    }
    // try to coalesce parents
    return Coalesce(free_list, bit_tree, ptr2,  lower, sizeParents, mpid);

}

int LookForDealloc(DLIST free_list[], U8 bit_tree[], int level, mpool_t mpid, DNODE *ptr, int pos) {
    int exp = (mpid == MPID_IRAM1 ? RAM1_SIZE_LOG2 : RAM2_SIZE_LOG2) - level;
    int base = (mpid == MPID_IRAM1 ? RAM1_START: RAM2_START);
    int size = exponent(2, exp);
    if (CheckBit(bit_tree, pos) == 1) {
        // We are at the memory that must be deallocated
        FlipBit(bit_tree, level, (int)ptr, base, size);
        // Add pointer to free list
        if(free_list[level].head == NULL) {
            free_list[level].head = ptr;
            return 0;
        } else if (free_list[level].head > ptr){
            DNODE *temp4 = free_list[level].head;
            temp4->prev = ptr;
            ptr->next = temp4;
            free_list[level].head = ptr;
        } else {
            AddToFreeList(free_list[level].head, level, ptr);
        }
        int coalesced = Coalesce(free_list, bit_tree, ptr, level, size, mpid);
    } else {
        int lower = level - 1;
        int lowerPos = pos/2 ;
        return LookForDealloc(free_list, bit_tree, lower, mpid, ptr, lowerPos);
    }
    return 0;
}

int k_mpool_dealloc(mpool_t mpid, void *ptr)
{
#ifdef DEBUG_0
    printf("k_mpool_dealloc: mpid = %d, ptr = 0x%x\r\n", mpid, ptr);
#endif /* DEBUG_0 */

    if(mpid != MPID_IRAM1 && mpid != MPID_IRAM2) {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    //RAM1_START and end of IRAM1
    int pool_start = (mpid == MPID_IRAM1 ? RAM1_START: RAM2_START);
    int pool_end = (mpid == MPID_IRAM1 ? RAM1_END: RAM2_END);
    if((int)ptr < pool_start || (int)ptr > pool_end) {
        errno = EFAULT;
        return -1;
    }

    int level = (mpid == MPID_IRAM1 ? RAM1_SIZE_LOG2: RAM2_SIZE_LOG2) - 5;
    int exp = (mpid == MPID_IRAM1 ? RAM1_SIZE_LOG2 : RAM2_SIZE_LOG2) - level;
    int base = (mpid == MPID_IRAM1 ? RAM1_START: RAM2_START);
    int size = exponent(2, exp);
    int pos = (((int)ptr - base)/ size) + exponent(2,level) - 1;
    
    DNODE *dePtr = ptr;
    dePtr->next = NULL;
    dePtr->prev= NULL;
    return LookForDealloc((mpid == MPID_IRAM1 ? free_list_1 : free_list_2), ( mpid == MPID_IRAM1 ? bit_tree_1: bit_tree_2), level, mpid, dePtr, pos);
}

int WalkFreeList(DLIST free_list[], int level, mpool_t mpid) {
    if(level < 0) {
        return 0;
    }
    int memBlks = 0;
    DNODE *ptr = free_list[level].head;
    int exp = (mpid == MPID_IRAM1 ? RAM1_SIZE_LOG2 : RAM2_SIZE_LOG2) - level;
    int size = exponent(2, exp);
    while(ptr != NULL) {
        memBlks++;
        printf("0x%x: 0x%x\n", (int)ptr, size);
        ptr = ptr->next;
    }
    int lower = level - 1;
		
    return memBlks + WalkFreeList(free_list, lower, mpid);
}

int k_mpool_dump (mpool_t mpid)
{
#ifdef DEBUG_0
    printf("k_mpool_dump: mpid = %d\r\n", mpid);
#endif /* DEBUG_0 */

//ML - mpid validity check
    if(mpid != MPID_IRAM1 && mpid != MPID_IRAM2) {
        return 0;
    }

    int level = (mpid == MPID_IRAM1 ? RAM1_SIZE_LOG2: RAM2_SIZE_LOG2) - 5;
    int blocks =  WalkFreeList((mpid == MPID_IRAM1 ? free_list_1 : free_list_2), level, mpid);
    printf("%d free memory block(s) found \n",blocks);
    
    return blocks;
}
 
int k_mem_init(int algo)
{
#ifdef DEBUG_0
    printf("k_mem_init: algo = %d\r\n", algo);
#endif /* DEBUG_0 */
        
    if ( k_mpool_create(algo, RAM1_START, RAM1_END) < 0 ) {
        return RTX_ERR;
    }
    
    if ( k_mpool_create(algo, RAM2_START, RAM2_END) < 0 ) {
        return RTX_ERR;
    }
    
    return RTX_OK;
}

/**
 * @brief allocate kernel stack statically
 */
U32* k_alloc_k_stack(task_t tid)
{
    
    if ( tid >= MAX_TASKS) {
        errno = EAGAIN;
        return NULL;
    }
    U32 *sp = g_k_stacks[tid+1];
    
    // 8B stack alignment adjustment
    if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
        sp--;               // adjust it to 8B aligned
    }
    return sp;
}


U32* k_alloc_user_stack(U32 stack_size, task_t tid)
{
    if ( tid >= MAX_TASKS) {
        errno = EAGAIN;
        return NULL;
    }
    
    // Allocate the desired size of memory for this stack and set the pointer to the value returned
    int address = (int)k_mpool_alloc(MPID_IRAM2, stack_size); 
    if(address > 0){
        U32 *sp = (U32 *)(address + exponent(2,log_ceil(stack_size)));
       
        // 8B stack alignment adjustment
        if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
            sp--;               // adjust it to 8B aligned
        }
        return sp;
    } else {
        return NULL;
    }
    
    
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

