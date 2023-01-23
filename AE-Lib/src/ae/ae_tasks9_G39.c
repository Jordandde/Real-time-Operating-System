// LAB 3 - TEST SUITE 9

#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

#define     NUM_TESTS       2       // number of tests
#define     NUM_INIT_TASKS  1       // number of tasks during initialization
#define     BUF_LEN         128     // receiver buffer length
#define     MY_MSG_TYPE     100     // some customized message type

const char   PREFIX[]      = "G39-TS9";
const char   PREFIX_LOG[]  = "G39-TS9-LOG";
const char   PREFIX_LOG2[] = "G39-TS9-LOG2";
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
    g_tsk_cases[test_id].p_ae_case->num_bits = 13;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 8;
       
    update_ae_xtest(test_id);
}

void gen_req1(int test_id)
{
    //bits[0:3] pos check, bits[4:12] for exec order check
    g_tsk_cases[test_id].p_ae_case->num_bits = 12;  
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
    strcpy(g_ae_xtest.msg, "task0: creating a MEDIUM prio task that runs task2 function");
    ret_val = tsk_create(&g_tids[2], &task2, MEDIUM, 0x200);  /*create a user task */
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    if ( ret_val != RTX_OK ) {
        test_exit();
    }
    
    //test 0-[2]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task0: creating a mailbox of size 32 Bytes");
    mbx_t mbx_id = mbx_create(32);  // create a mailbox for itself
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
    p_seq_expt[4] = g_tids[2];
    p_seq_expt[5] = g_tids[0];
    p_seq_expt[6] = g_tids[2];
    p_seq_expt[7] = g_tids[1];
    
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
    
    //test 1-[4:11]
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

    printf("%s: TID = %u, task0: calling tsk_yield()\r\n", PREFIX_LOG2, tid);     
    tsk_yield();

    update_exec_seq(test_id, tid);

    for(int i = 0; i < 5; i++){
        printf("%s: TID = %u, receiving message in loop iteration: %d\r\n", PREFIX_LOG2, tid, i);
        U8 buf2[6];  
        recv_msg(buf2, 6);
    }

    update_exec_seq(test_id, tid);

    //test 0-[8]
    (*p_index)++;
    U8 buf3[8];
    recv_msg(buf3, 8);
    strcpy(g_ae_xtest.msg, "task0: test received message data against message sent by task2");
    sub_result = (buf3[6] == 'B' && buf3[7] == 'C') ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[9]
    (*p_index)++;
    U8 buf4[8];
    recv_msg(buf4, 8);
    strcpy(g_ae_xtest.msg, "task0: test received message data against message sent by task1");
    sub_result = (buf4[6] == 'A' && buf4[7] == 'B') ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    printf("%s: TID = %u, task0: calling tsk_yield()\r\n", PREFIX_LOG2, tid);     
    tsk_yield();

    test1_start(test_id + 1, test_id);
}

void task1(void)
{ 
    int ret_val = 10;
    mbx_t  mbx_id;
    task_t tid = tsk_gettid();
    int    test_id = 0;
    U8     *p_index    = &(g_ae_xtest.index);
    int    sub_result  = 0;
    
    size_t msg_hdr_size = sizeof(struct rtx_msg_hdr);
    U8  *buf = &g_buf1[0];
    struct rtx_msg_hdr *ptr = (void *)buf;
   
    printf("%s: TID = %u, task1: entering\r\n", PREFIX_LOG2, tid);   
    update_exec_seq(test_id, tid);

    for(int i = 0; i < 5; i++){
        RTX_MSG_HDR *buf1 = mem_alloc(MSG_HDR_SIZE);   
        buf1->length = MSG_HDR_SIZE;
        buf1->type = DEFAULT;
        buf1->sender_tid = tid;

        //test 0-[3:7]
        (*p_index)++;
        sprintf(g_ae_xtest.msg, "task1: blocking send_msg to tid(%u) in a loop, iteration %d", g_tids[0], i);
        ret_val = send_msg_nb(g_tids[0], buf1);      // no-blocking send to task2, a mesage with no data field 
        sub_result = (ret_val == RTX_OK) ? 1 : 0;
        process_sub_result(test_id, *p_index, sub_result);  
    }

    ptr->length = MSG_HDR_SIZE + 2;         // set the message length
    ptr->type = DEFAULT;                    // set message type
    ptr->sender_tid = tid;                  // set sender id 
    buf += MSG_HDR_SIZE;                        
    *buf = 'A';
		buf++;
		*buf = 'B';
 
    printf("%s: TID = %u, trying to send blocking message to mailbox with insufficient space\r\n", PREFIX_LOG2, tid);
    send_msg(g_tids[0], (void *)ptr);

    update_exec_seq(test_id, tid);

    //test 0-[11]
    (*p_index)++;
    ptr->type = DISPLAY;
    strcpy(g_ae_xtest.msg, "task1: calling DISPLAY send_msg to TID_CON");
    ret_val = send_msg_nb(TID_CON, (void *)ptr); 
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result); 

    ptr->length = MSG_HDR_SIZE + 1;
    ptr->type = KCD_REG;
    *buf = 'X';

    //test 0-[12]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task1: calling KCD_REG send_msg to TID_KCD, re-register command X");
    ret_val = send_msg_nb(TID_KCD, (void *)ptr); 
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result); 

    printf("%s: TID = %u, task1: I should be the last one to run\r\n", PREFIX_LOG2, tid);
    test1_start(test_id + 1, test_id);
}

void task2(void)
{
    int     ret_val;
    task_t  tid         = tsk_gettid();
    int     test_id     = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
    
    printf("%s: TID = %u, task2: entering\r\n", PREFIX_LOG2, tid);  
    update_exec_seq(test_id, tid);

    printf("%s: setting own priority to HIGH\r\n", PREFIX_LOG2);
    tsk_set_prio(tid, HIGH);

    U8  *buf = &g_buf1[0];
    struct rtx_msg_hdr *ptr = (void *)buf;
    ptr->length = MSG_HDR_SIZE + 2;         // set the message length
    ptr->type = DEFAULT;                    // set message type
    ptr->sender_tid = tid;                  // set sender id 
    buf += MSG_HDR_SIZE;                        
    *buf = 'B';
	buf++;
	*buf = 'C';


    printf("%s: TID = %u, trying to send blocking message to mailbox with insufficient space\r\n", PREFIX_LOG2, tid);
    send_msg(g_tids[0], (void *)ptr);

    update_exec_seq(test_id, tid);

    printf("%s: setting own priority to MEDIUM\r\n", PREFIX_LOG2);
    tsk_set_prio(tid, MEDIUM);
tsk_set_prio(tid, HIGH);
    update_exec_seq(test_id, tid);

    ptr->length = MSG_HDR_SIZE + 1;
    ptr->type = KCD_REG;
	buf -=2;                 
    *buf = 'X';

    //test 0-[10]
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "task1: calling KCD_REG send_msg to TID_KCD, register command X");
    ret_val = send_msg_nb(TID_KCD, (void *)ptr); 
    sub_result = (ret_val == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result); 
    tsk_set_prio(tid, MEDIUM);
    printf("%s: TID = %u, task2: calling tsk_yield()\r\n", PREFIX_LOG2, tid);     
    tsk_yield();
    
    printf("%s: TID = %u, task2: exiting...\r\n", PREFIX_LOG2, tid);
    tsk_exit();         // terminating the task
}
