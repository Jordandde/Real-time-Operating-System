// LAB 4 - TEST SUITE 12
// SENDING AND RECEIVING MESSAGES ACROSS REAL-TIME AND NON REAL-TIME TASKS

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
void task4(void)
{
    int     ret_val;
    U8      *buf        = NULL;
    task_t  tid         = tsk_gettid();
    int     test_id     = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
    mbx_t  mbx_id;

    printf("%s: TID = %u, task4: entering\r\n", PREFIX_LOG2, tid);  

    TIMEVAL tv;
    tv.sec = 2;
    tv.usec = 0;

    rt_tsk_set(&tv);

    int counter = 0;

    while(counter <3){
        update_exec_seq(test_id, tid);
        printf("%s: TID = %u, task4: running in real-time\r\n", PREFIX_LOG2, tid); 
        counter++;
        rt_tsk_susp();
    }
		tsk_exit();
}

void task5(void)
{
    int     ret_val;
    U8      *buf        = NULL;
    task_t  tid         = tsk_gettid();
    int     test_id     = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
    mbx_t  mbx_id;

    printf("%s: TID = %u, task5: entering\r\n", PREFIX_LOG2, tid);  

    TIMEVAL tv;
    tv.sec = 1;
    tv.usec = 0;

    rt_tsk_set(&tv);

    int counter = 0;

    while(counter < 3){
        update_exec_seq(test_id, tid);
        printf("%s: TID = %u, task5: running in real-time\r\n", PREFIX_LOG2, tid);  
        counter++;
        rt_tsk_susp();
    }
		tsk_exit();
}
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
    g_tsk_cases[test_id].p_ae_case->num_bits = 27;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 16;
       
    update_ae_xtest(test_id);
}

void gen_req1(int test_id)
{
    //bits[0:3] pos check, bits[4:12] for exec order check
    g_tsk_cases[test_id].p_ae_case->num_bits = 20;  
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
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "task0: creating a MEDIUM prio task that runs task2 function");
    ret_val = tsk_create(&g_tids[2], &task2, MEDIUM, 0x200);  /*create a user task */
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);    
    if ( ret_val != RTX_OK ) {
        sub_result = 0;
        test_exit();
    }

    //test 0-[2]
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "task0: creating a MEDIUM prio task that runs task3 function");
    ret_val = tsk_create(&g_tids[3], &task3, MEDIUM, 0x200);  /*create a user task */
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);    
    if ( ret_val != RTX_OK ) {
        sub_result = 0;
        test_exit();
    }

    //test 0-[3]
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "task0: creating a MEDIUM prio task that runs task4 function");
    ret_val = tsk_create(&g_tids[4], &task4, MEDIUM, 0x200);  /*create a user task */
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);    
    if ( ret_val != RTX_OK ) {
        sub_result = 0;
        test_exit();
    }

    //test 0-[4]
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "task0: creating a MEDIUM prio task that runs task5 function");
    ret_val = tsk_create(&g_tids[5], &task5, MEDIUM, 0x200);  /*create a user task */
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);    
    if ( ret_val != RTX_OK ) {
        sub_result = 0;
        test_exit();
    }
//for ( int x = 0; x < DELAY; x++); // some artifical delay 
    //test 0-[5]
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "task0: creating a task with PRIO_RT as priority, check -1 return value");
//    ret_val = tsk_create(&g_tids[1], &task5, PRIO_RT, 0x200);
   // sub_result = (ret_val == -1) ? 1 : 0;
   // process_sub_result(test_id, *p_index, sub_result);    
    
    task_t  *p_seq_expt = g_tsk_cases[test_id].seq_expt;

    // IMPORTANT!!

    // TODO:    EXPECTED SEQUENCE IS NOT PUT YET. NEED TO CHECK WHAT EXPECTED SEQUENCE
    //          COULD BE DEPENDING ON THE EXECUTION TIME OF EACH FUNCTION

    // IMPORTANT!!

    p_seq_expt[0] = g_tids[0];
    p_seq_expt[1] = g_tids[1];
    p_seq_expt[2] = g_tids[0];
    p_seq_expt[3] = g_tids[1];
    p_seq_expt[4] = g_tids[2];
    p_seq_expt[5] = g_tids[1];
    
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
    
    //test 1-[4:19]
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

    test0_start(test_id);

    printf("%s: TID = %u, task1: calling tsk_yield()\r\n", PREFIX_LOG2, tid);     
    tsk_yield(); 

    update_exec_seq(test_id, tid);

    for(int i = 0; i < 5; i++){
        RTX_MSG_HDR *buf1 = mem_alloc(MSG_HDR_SIZE);   
        buf1->length = MSG_HDR_SIZE;
        buf1->type = 10 + i;
        buf1->sender_tid = tid;

        //test 0-[7:11]
        (*p_index)++;
        sprintf(g_ae_xtest.msg, "task0: send_msg to tid(%u) in a loop, iteration %d", g_tids[1], i);
        ret_val = send_msg(g_tids[1], buf1);
        sub_result = (ret_val == RTX_OK) ? 1 : 0;
        process_sub_result(test_id, *p_index, sub_result);  
    }

    tsk_exit();
}

void task1(void)
{  
    int ret_val = 10;
    mbx_t  mbx_id;
    U8      *buf        = NULL;
    task_t tid = tsk_gettid();
    int    test_id = 0;
    U8     *p_index    = &(g_ae_xtest.index);
    int    sub_result  = 0;

    printf("%s: TID = %u, task1: entering\r\n", PREFIX_LOG2, tid);    
    update_exec_seq(test_id, tid); 

    TIMEVAL tv;
    tv.sec = 5;
    tv.usec = 0;

    //test 0-[6]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task1: creating a mailbox of size 256 Bytes");
    mbx_id = mbx_create(256);  // create a mailbox for itself
    sub_result = (mbx_id >= 0) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    rt_tsk_set(&tv);

    printf("task1: calling recv_msg blocking, I should be blocked\r\n");
    buf = mem_alloc(BUF_LEN);
    recv_msg(buf, BUF_LEN);

    update_exec_seq(test_id, tid);

    printf("%s: TID = %u, task1: setting task0 prio to HIGH\r\n", PREFIX_LOG2, tid);  
    tsk_set_prio(g_tids[0], HIGH); 

    int counter = 0;

    while(counter < 4){
        update_exec_seq(test_id, tid);
        //test 0-[12:15] (can't tell exact execution order)
        (*p_index)++;
        U8 buffer[BUF_LEN];
        sprintf(g_ae_xtest.msg, "task1: recv_msg_nb in real-time, iteration %d", counter);
        ret_val = recv_msg_nb(buffer, BUF_LEN);
        RTX_MSG_HDR *p_buf = (RTX_MSG_HDR *) buffer;
        sub_result = (ret_val == RTX_OK && p_buf->type == 10 + counter) ? 1 : 0;
        process_sub_result(test_id, *p_index, sub_result);
        counter++;
        rt_tsk_susp();
    }
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

    //test 0-[16] (can't tell exact execution order)
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task2: creating a mailbox of size 256 Bytes");
    mbx_id = mbx_create(256);  // create a mailbox for itself
    sub_result = (mbx_id >= 0) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    tsk_yield();

    TIMEVAL tv;
    tv.sec = 4;
    tv.usec = 0;

    rt_tsk_set(&tv);

    int counter = 0;

    while(counter < 5){
        update_exec_seq(test_id, tid);
        //test 0-[17:21] (can't tell exact execution order)
        (*p_index)++;
        U8 buffer[BUF_LEN];
        sprintf(g_ae_xtest.msg, "task2: recv_msg in a real-time, iteration %d", counter);
        ret_val = recv_msg(buffer, BUF_LEN);
        RTX_MSG_HDR *p_buf = (RTX_MSG_HDR *) buffer;
        sub_result = (ret_val == RTX_OK && p_buf->type == 20 + counter) ? 1 : 0;
        process_sub_result(test_id, *p_index, sub_result);
        counter++;
        rt_tsk_susp();
    }
		tsk_exit();
}

void task3(void)
{
    int     ret_val;
    U8      *buf        = NULL;
    task_t  tid         = tsk_gettid();
    int     test_id     = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
    mbx_t  mbx_id;
    
    printf("%s: TID = %u, task3: entering\r\n", PREFIX_LOG2, tid);  
    update_exec_seq(test_id, tid);

    RTX_MSG_HDR *buf1 = mem_alloc(MSG_HDR_SIZE);   
    buf1->length = MSG_HDR_SIZE;
    buf1->sender_tid = tid;
    buf1->type = 20;

    //test 0-[22] (can't tell exact execution order)
    (*p_index)++;
    sprintf(g_ae_xtest.msg, "task3: send_msg to tid(%u)", g_tids[2]);
    ret_val = send_msg(g_tids[2], buf1);
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    printf("%s: TID = %u, task3: setting task2 prio to HIGH\r\n", PREFIX_LOG2, tid);  
    tsk_set_prio(g_tids[2], HIGH); 

    TIMEVAL tv;
    tv.sec = 0;
    tv.usec = 100000;

    rt_tsk_set(&tv);

    int counter = 0;

    while(counter < 4){
        update_exec_seq(test_id, tid);

        RTX_MSG_HDR *buf2 = mem_alloc(MSG_HDR_SIZE);   
        buf2->length = MSG_HDR_SIZE;
        buf2->sender_tid = tid;
        buf2->type = (20 + counter + 1);
        //test 0-[23:26] (can't tell exact execution order)
        (*p_index)++;
        sprintf(g_ae_xtest.msg, "task3: send_msg to tid(%u) in real-time, iteration %d", g_tids[2], counter);
        ret_val = send_msg(g_tids[2], buf2);
        sub_result = (ret_val == RTX_OK) ? 1 : 0;
        process_sub_result(test_id, *p_index, sub_result); 

        counter++;
        rt_tsk_susp();
			
    }
		tsk_exit();
}

