// LAB 4 - TEST SUITE 10
// TESTING EDGE CASES AND ERRORS

#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

#define     NUM_TESTS       2       // number of tests
#define     NUM_INIT_TASKS  1       // number of tasks during initialization
#define     BUF_LEN         128     // receiver buffer length
#define     MY_MSG_TYPE     100     // some customized message type

const char   PREFIX[]      = "G39-TS10";
const char   PREFIX_LOG[]  = "G39-TS10-LOG";
const char   PREFIX_LOG2[] = "G39-TS10-LOG2";
TASK_INIT    g_init_tasks[NUM_INIT_TASKS];

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];

/* The following arrays can also be dynamic allocated to reduce ZI-data size
 *  They do not have to be global buffers (provided the memory allocator has no bugs)
 */
 
U8 g_buf1[BUF_LEN];
U8 g_buf2[BUF_LEN];
task_t g_tasks[MAX_TASKS];
task_t g_tids[MAX_TASKS];

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num)
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

void set_ae_tasks(TASK_INIT *tasks, int num)
{
    for (int i = 0; i < num; i++ ) {                                                 
        tasks[i].u_stack_size = PROC_STACK_SIZE;    
        tasks[i].prio = MEDIUM;
        tasks[i].priv = 0;
    }

    tasks[0].ptask = &task0;
    
    init_ae_tsk_test();
}

void init_ae_tsk_test(void)
{
    g_ae_xtest.test_id = 0;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests = NUM_TESTS;
    g_ae_xtest.num_tests_run = 0;
    
    for ( int i = 0; i< NUM_TESTS; i++ ) {
        g_tsk_cases[i].p_ae_case = &g_ae_cases[i];
        g_tsk_cases[i].p_ae_case->results  = 0x0;
        g_tsk_cases[i].p_ae_case->test_id  = i;
        g_tsk_cases[i].p_ae_case->num_bits = 0;
        g_tsk_cases[i].pos = 0;  // first avaiable slot to write exec seq tid
        // *_expt fields are case specific, deligate to specific test case to initialize
    }
    printf("%s: START\r\n", PREFIX);
}

void update_ae_xtest(int test_id)
{
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}

void gen_req0(int test_id)
{
    g_tsk_cases[test_id].p_ae_case->num_bits = 16;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 5;
       
    update_ae_xtest(test_id);
}

void gen_req1(int test_id)
{
    //bits[0:3] pos check, bits[4:12] for exec order check
    g_tsk_cases[test_id].p_ae_case->num_bits = 9;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 0;       // N/A for this test
    g_tsk_cases[test_id].pos_expt = 0;  // N/A for this test
    
    update_ae_xtest(test_id);
}

int test0_start(int test_id)
{
    int ret_val = 10;
    
    gen_req0(test_id);

    U8   *p_index    = &(g_ae_xtest.index);
    int  sub_result  = 0;
    
    //test 0-[0]
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "task0: creating a MEDIUM prio task that runs task1 function");
    ret_val = tsk_create(&g_tids[1], &task1, MEDIUM, 0x200);  /*create a user task */
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);    
    if ( ret_val != RTX_OK ) {
        sub_result = 0;
        test_exit();
    }
    
    //test 0-[1]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task0: creating a LOW prio task that runs task2 function");
    ret_val = tsk_create(&g_tids[2], &task2, LOW, 0x200);  /*create a user task */
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    if ( ret_val != RTX_OK ) {
        test_exit();
    }
    
    //test 0-[2]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task0: creating a mailbox of size 256 Bytes");
    mbx_t mbx_id = mbx_create(256);  // create a mailbox for itself
    sub_result = (mbx_id >= 0) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    if ( ret_val != RTX_OK ) {
        test_exit();
    }
    
    task_t  *p_seq_expt = g_tsk_cases[test_id].seq_expt;
    p_seq_expt[0] = g_tids[0];
    p_seq_expt[1] = g_tids[1];
    p_seq_expt[2] = g_tids[2];
    p_seq_expt[3] = g_tids[0];
    p_seq_expt[4] = g_tids[0];
    
    return RTX_OK;
}

void test1_start(int test_id, int test_id_data)
{  
    gen_req1(1);
    
    U8      pos         = g_tsk_cases[test_id_data].pos;
    U8      pos_expt    = g_tsk_cases[test_id_data].pos_expt;
    task_t  *p_seq      = g_tsk_cases[test_id_data].seq;
    task_t  *p_seq_expt = g_tsk_cases[test_id_data].seq_expt;
       
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
    
    // output the real execution order
    printf("%s: real exec order: ", PREFIX_LOG);
    int pos_len = (pos > MAX_LEN_SEQ)? MAX_LEN_SEQ : pos;
    for ( int i = 0; i < pos_len; i++) {
        printf("%d -> ", p_seq[i]);
    }
    printf("NIL\r\n");
    
    // output the expected execution order
    printf("%s: expt exec order: ", PREFIX_LOG);
    for ( int i = 0; i < pos_expt; i++ ) {
        printf("%d -> ", p_seq_expt[i]);
    }
    printf("NIL\r\n");
    
    int diff = pos - pos_expt;
    
    // test 1-[0]
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "checking execution shortfalls");
    sub_result = (diff < 0) ? 0 : 1;
    process_sub_result(test_id, *p_index, sub_result);
    
    //test 1-[1]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "checking unexpected execution once");
    sub_result = (diff == 1) ? 0 : 1;
    process_sub_result(test_id, *p_index, sub_result);
    
    //test 1-[2]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "checking unexpected execution twice");
    sub_result = (diff == 2) ? 0 : 1;
    process_sub_result(test_id, *p_index, sub_result);
    
    //test 1-[3]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "checking correct number of executions");
    sub_result = (diff == 0) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    
    //test 1-[4:8]
    for ( int i = 0; i < pos_expt; i ++ ) {
        (*p_index)++;
        sprintf(g_ae_xtest.msg, "checking execution sequence @ %d", i);
        sub_result = (p_seq[i] == p_seq_expt[i]) ? 1 : 0;
        process_sub_result(test_id, *p_index, sub_result);
    }
           
    test_exit();
}


int update_exec_seq(int test_id, task_t tid) 
{
    U8 len = g_tsk_cases[test_id].len;
    U8 *p_pos = &g_tsk_cases[test_id].pos;
    task_t *p_seq = g_tsk_cases[test_id].seq;
    p_seq[*p_pos] = tid;
    (*p_pos)++;
    (*p_pos) = (*p_pos)%len;  // preventing out of array bound
    return RTX_OK;
}

void task0(void)
{
    int ret_val = 10;
    task_t tid = tsk_gettid();
    g_tids[0] = tid;
    int     test_id    = 0;
    U8      *p_index   = &(g_ae_xtest.index);
    int     sub_result = 0;

    printf("%s: TID = %u, task0 entering\r\n", PREFIX_LOG2, tid);

    test0_start(test_id);

    TIMEVAL tv;
    tv.sec = 0;
    tv.usec = 0;

    //test 0-[3]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task0: calling rt_tsk_susp() when task is not real-time, check -1 return value and errno");
    ret_val = rt_tsk_susp(); 
    sub_result = (ret_val == -1 && errno == EPERM) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result); 

    //test 0-[4]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task0: calling rt_tsk_set() with period as 0, check -1 return value and errno");
    ret_val = rt_tsk_set(&tv); 
    sub_result = (ret_val == -1 && errno == EINVAL) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result); 

    //test 0-[5]
    (*p_index)++;
    tv.usec = 2000;
    strcpy(g_ae_xtest.msg, "task0: calling rt_tsk_set() with period smaller than minimum, check -1 return value and errno");
    ret_val = rt_tsk_set(&tv); 
    sub_result = (ret_val == -1 && errno == EINVAL) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result); 

    //test 0-[6]
    (*p_index)++;
    tv.usec = 2650;
    strcpy(g_ae_xtest.msg, "task0: calling rt_tsk_set() with period not multiple of RTX_TICK_SIZE * MIN_PERIOD, check -1 return value and errno");
    ret_val = rt_tsk_set(&tv); 
    sub_result = (ret_val == -1 && errno == EINVAL) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[7]
    (*p_index)++;
    tv.sec = 0;
    tv.usec = 200000;
    strcpy(g_ae_xtest.msg, "task0: calling rt_tsk_set() with correct period, check 0 return value");
    ret_val = rt_tsk_set(&tv); 
    sub_result = (ret_val == 0) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result); 

    //test 0-[8]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task0: calling rt_tsk_set() when it is already a real-time task, check -1 return value and errno");
    ret_val = rt_tsk_set(&tv); 
    sub_result = (ret_val == -1 && errno == EPERM) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[9]
    (*p_index)++;
    TIMEVAL buf[1];
    strcpy(g_ae_xtest.msg, "task0: calling rt_tsk_get() with task_tid of a NON real-time task, check -1 return value and errno");
    ret_val = rt_tsk_get(g_tids[2], buf); 
    sub_result = (ret_val == -1 && errno == EINVAL) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[10]
    // NOTE: Sending NULL pointer to rt_tsk_get() should not crash the program
    // The instructor said that it is a free design choice on how we want to handle this error
    // Just return -1, since no errno is specified in the manual
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task0: calling rt_tsk_get() with NULL pointer, check -1 return value");
    ret_val = rt_tsk_get(g_tids[0], NULL); 
    sub_result = (ret_val == -1) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    int flag = 1;
    int counter = 1;

    while(counter <= 3){
        if(flag == 1){
            flag = 0;

            // tsk_yield() should not do anything if a REAL-TIME task calls it
            printf("%s: TID = %u, task0: calling tsk_yield(), I SHOULD NOT BE YIELDED\r\n", PREFIX_LOG2, tid);     
            tsk_yield();
            printf("%s: TID = %u, task0: If you're reading this right after my last message, I was not yielded (as expected)\r\n", PREFIX_LOG2, tid);     
            
            //test 0-[11]
            (*p_index)++;
            strcpy(g_ae_xtest.msg, "task0: trying to change priority of real-time task to MEDIUM, check -1 return value and errno");
            ret_val = tsk_set_prio(tid, MEDIUM); 
            sub_result = (ret_val == -1 && errno == EPERM) ? 1 : 0;
            process_sub_result(test_id, *p_index, sub_result);

            //test 0-[12]
            (*p_index)++;
            strcpy(g_ae_xtest.msg, "task0: trying to change priority of real-time task to PRIO_RT, check 0 return value");
            ret_val = tsk_set_prio(tid, PRIO_RT); 
            sub_result = (ret_val == 0) ? 1 : 0;
            process_sub_result(test_id, *p_index, sub_result);
        }
        update_exec_seq(test_id, tid);
        printf("task0: I am running. Now calling suspend\r\n");
        counter++;
        rt_tsk_susp();
    }

    printf("%s: TID = %u, task0 calling TEST1 function\r\n", PREFIX_LOG2, tid);
    test1_start(test_id + 1, test_id);

    // TODO: (FOR BOTH SET AND SUSPEND FUNCTIONS) 
    // CHECK FOR ENOMEM (NOT ENOUGH MEMORY TO SUPPORT THIS OPERATION)
}

void task1(void)
{  
    int ret_val = 10;
    mbx_t  mbx_id;
    task_t tid = tsk_gettid();
    int    test_id = 0;
    U8     *p_index    = &(g_ae_xtest.index);
    int    sub_result  = 0;

    printf("%s: TID = %u, task1: entering\r\n", PREFIX_LOG2, tid);   
    update_exec_seq(test_id, tid);

    TIMEVAL buf[1];

    //test 0-[13]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task1: calling rt_tsk_get() from a NON real-time task, check -1 return value and errno");
    ret_val = rt_tsk_get(g_tids[0], buf); 
    sub_result = (ret_val == -1 && errno == EPERM) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[14]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task1: trying to change priority of NON real-time task to PRIO_RT, check -1 return value and errno");
    ret_val = tsk_set_prio(tid, PRIO_RT); 
    sub_result = (ret_val == -1 && errno == EPERM) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    TIMEVAL tv;
    tv.sec = 0;
    tv.usec = 25000;

    //test 0-[15]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task1: elevating itself to real-time with smallest possible period, check 0 return value");
    ret_val = rt_tsk_set(&tv); 
    sub_result = (ret_val == 0) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    printf("%s: TID = %u, task1 exiting\r\n", PREFIX_LOG2, tid);
    tsk_exit();
}

void task2(void)
{
    int     ret_val;
    U8      *buf        = NULL;
    task_t  tid         = tsk_gettid();
    int     test_id     = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
    mbx_t  mbx_id;
    
    printf("%s: TID = %u, task2: entering\r\n", PREFIX_LOG2, tid);  
    update_exec_seq(test_id, tid);
	
		printf("%s: TID = %u, task2: exiting\r\n", PREFIX_LOG2, tid);
		tsk_exit();
}