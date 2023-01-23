// LAB 4 - TEST SUITE 11
// REAL-TIME AND NON REAL-TIME MIXED TEST

#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

#define     NUM_TESTS       2       // number of tests
#define     NUM_INIT_TASKS  1       // number of tasks during initialization
#define     BUF_LEN         128     // receiver buffer length
#define     MY_MSG_TYPE     100     // some customized message type

const char   PREFIX[]      = "G39-TS11";
const char   PREFIX_LOG[]  = "G39-TS11-LOG";
const char   PREFIX_LOG2[] = "G39-TS11-LOG2";
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
    g_tsk_cases[test_id].p_ae_case->num_bits = 6;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 9;
       
    update_ae_xtest(test_id);
}

void gen_req1(int test_id)
{
    //bits[0:3] pos check, bits[4:12] for exec order check
    g_tsk_cases[test_id].p_ae_case->num_bits = 13;  
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
    
    task_t  *p_seq_expt = g_tsk_cases[test_id].seq_expt;
    p_seq_expt[0] = g_tids[0];
    p_seq_expt[1] = g_tids[1];
    p_seq_expt[2] = g_tids[0];
    p_seq_expt[3] = g_tids[1];
    p_seq_expt[4] = g_tids[0];
    p_seq_expt[5] = g_tids[2];
    p_seq_expt[6] = g_tids[0];
    p_seq_expt[7] = g_tids[1];
    p_seq_expt[8] = g_tids[2];
    
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
    
    //test 1-[4:12]
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
    update_exec_seq(test_id, tid);

    test0_start(test_id);                           	// set message data

    printf("%s: TID = %u, task0: calling tsk_yield()\r\n", PREFIX_LOG2, tid);     
    tsk_yield(); 

    update_exec_seq(test_id, tid);

    U8  *buf = &g_buf1[0];
    struct rtx_msg_hdr *ptr = (void *)buf;
    ptr->length = MSG_HDR_SIZE + 1;  	        // set the message length
    ptr->type = DEFAULT;                    	// set message type
    ptr->sender_tid = tid;                  	// set sender id 
    buf += MSG_HDR_SIZE;                        
    *buf = 'B';                             	// set message data

    //test 0-[3]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task0: send_msg_nb to task1, check 0 return value");
    ret_val = send_msg_nb(g_tids[1], (void *)ptr); 
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);  

    printf("%s: TID = %u, task0: calling tsk_yield()\r\n", PREFIX_LOG2, tid);     
    tsk_yield();

    update_exec_seq(test_id, tid);

    printf("task0: you should read this after real-time task1 has ran once\r\n");

    printf("%s: setting prio of task2 to HIGH\r\n", PREFIX_LOG2);
    tsk_set_prio(g_tids[2], HIGH);

    update_exec_seq(test_id, tid);

    printf("%s: TID = %u, task0: exiting\r\n", PREFIX_LOG2, tid);
    tsk_exit();
		//while(1){
		//}
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

    TIMEVAL tv;
    tv.sec = 0;
    tv.usec = 50000;
    int flag = 1;

    //test 0-[2]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task1: creating a mailbox of size 18 Bytes");
    mbx_id = mbx_create(18);  // create a mailbox for itself
    sub_result = (mbx_id >= 0) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    printf("%s: TID = %u, task1: calling tsk_yield()\r\n", PREFIX_LOG2, tid);     
    tsk_yield(); 

    int ret = rt_tsk_set(&tv);
    int counter = 1;

    while(counter <= 2){
        if(flag == 1){
            
            flag = 0;

            //test 0-[4]
            (*p_index)++;
            strcpy(g_ae_xtest.msg, "task1: checking if rt_tsk_set() was successful, check 0 return value");
            sub_result = (ret == 0) ? 1 : 0;
            process_sub_result(test_id, *p_index, sub_result);

            //test 0-[5]
            (*p_index)++;
            strcpy(g_ae_xtest.msg, "task1: checking received message data, check 0 return value");
            ret_val = recv_msg_nb(g_buf2, BUF_LEN); 
            sub_result = (ret_val == RTX_OK && g_buf2[6] == 'B') ? 1 : 0;
            process_sub_result(test_id, *p_index, sub_result);
        }

        printf("task1: I am running in real-time! iteration: %d/2\r\n", counter);
        update_exec_seq(test_id, tid);
        counter++;
        rt_tsk_susp();
    }

    //printf("%s: TID = %u, task1: exiting\r\n", PREFIX_LOG2, tid);
    //tsk_exit();
		test1_start(test_id + 1, test_id);
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

    TIMEVAL tv;
    tv.sec = 0;
    tv.usec = 30000;

    int ret = rt_tsk_set(&tv);
    int counter = 1;

    while(counter <= 3){
        update_exec_seq(test_id, tid);
        printf("task2: also running, iteration: %d/3\r\n", counter);
        counter++;
        rt_tsk_susp();
    }
}