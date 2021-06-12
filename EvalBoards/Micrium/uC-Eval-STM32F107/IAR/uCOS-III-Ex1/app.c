/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2009; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                     ST Microelectronics STM32
*                                              on the
*
*                                     Micrium uC-Eval-STM32F107
*                                        Evaluation Board
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : 조현준, 윤성준
                  EHS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include <includes.h>


/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/


/* flag definition */
#define ID_MISMATCH     (OS_FLAGS)0x0000;
#define TIMEOUT         (OS_FLAGS)0x0001;
#define TEMP_HIGH       (OS_FLAGS)0x0002;
#define TEMP_NOT_BODY   (OS_FLAGS)0x0003;
#define TEMP_REDO       (OS_FLAGS)0x0010;
#define DISINF_DONE     (OS_FLAGS)0x0021;
#define DOOR_OPEN       (OS_FLAGS)0x0022;
OS_FLAG_GRP lcdFlgGrp;

#define PROX_DETECT     (OS_FLAGS)0x0101;
#define TEMP_IS_BODY    (OS_FLAGS)0x0102;
OS_FLAG_GRP SanCondFlgGrp; 

#define PIR_DETECT      (OS_FLAGS)0x0201;
#define TEMP_NORMAL     (OS_FLAGS)0x0202;
OS_FLAG_GRP DoorOpenFlgGrp;

#define ID_MATCH        (OS_FLAGS)0x0301;
#define PROX_DETECT_2   (OS_FLAGS)0x0302;
OS_FLAG_GRP DisinfStepFlgGrp;


//static  OS_SEM   AppSem;
static  OS_SEM   DoorSem;       //문 열고 닫을 때 사용하는 Semaphore

static  OS_TCB   AppTaskStartTCB; 
static  CPU_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];

static OS_TCB AppTask1_TCB;     //RFID
static OS_TCB AppTask2_TCB;     //Proximity
static OS_TCB AppTask3_TCB;     //Temperature
static OS_TCB AppTask4_TCB;     //Sanitizer
static OS_TCB AppTask5_TCB;     //Door open/close
static OS_TCB AppTask6_TCB;     //PIR Sensor
static OS_TCB AppTask7_TCB;     //Timer Task(20sec)
static OS_TCB AppTask8_TCB;     //LCD Task

/* 
- priority
    3 - T7
    4 - T1 T2 T3 T4 T6
    5 - T5
    6 - T8
*/

static  CPU_STK  AppTask1_Stk[APP_TASK_START_STK_SIZE];
static  CPU_STK  AppTask2_Stk[APP_TASK_START_STK_SIZE];
static  CPU_STK  AppTask3_Stk[APP_TASK_START_STK_SIZE];
static  CPU_STK  AppTask4_Stk[APP_TASK_START_STK_SIZE];
static  CPU_STK  AppTask5_Stk[APP_TASK_START_STK_SIZE];
static  CPU_STK  AppTask6_Stk[APP_TASK_START_STK_SIZE];
static  CPU_STK  AppTask7_Stk[APP_TASK_START_STK_SIZE];
static  CPU_STK  AppTask8_Stk[APP_TASK_START_STK_SIZE];



/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart  (void *p_arg);

static  void  AppTask1      (void *p_arg);
static  void  AppTask2      (void *p_arg);
static  void  AppTask3      (void *p_arg);
static  void  AppTask4      (void *p_arg);
static  void  AppTask5      (void *p_arg);
static  void  AppTask6      (void *p_arg);
static  void  AppTask7      (void *p_arg);
static  void  AppTask8      (void *p_arg);

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*********************************************************************************************************
*/

int  main (void)
{
    OS_ERR  err;


    BSP_IntDisAll();                                            /* Disable all interrupts.                              */

    OSInit(&err);                                               /* Init uC/OS-III.                                      */
    
    OSFlagCreate(&lcdFlgGrp,
                "LCD Flag Group",
                (OS_FLAGS)0,
                &err);

    OSFlagCreate(&SanCondFlgGrp,
                "Sanitizer Condition Flag Group",
                (OS_FLAGS)0,
                &err);

    OSSchedRoundRobinCfg((CPU_BOOLEAN)DEF_TRUE,
                         (OS_TICK)    10,
                         (OS_ERR*)    &err);

    if (err != OS_ERR_NONE) {
    }

    

    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB,                /* Create the start task                                */
                 (CPU_CHAR   *)"App Task Start",
                 (OS_TASK_PTR )AppTaskStart, 
                 (void       *)0,
                 (OS_PRIO     )APP_TASK_START_PRIO,
                 (CPU_STK    *)&AppTaskStartStk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

    if (err != OS_ERR_NONE) {
    }
    
    OSStart(&err);                                              /* Start multitasking (i.e. give control to uC/OS-III). */
    
    if (err != OS_ERR_NONE) {
    }
}


/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  AppTaskStart (void *p_arg)
{
    CPU_INT32U  cpu_clk_freq;
    CPU_INT32U  cnts;
    OS_ERR      err;
    CPU_TS  ts;
    

   (void)p_arg;

    //OSSemCreate(&AppSem, "Test Sem", 0, &err);
    OSSemCreate(&DoorSem,
                "DoorSemaphore",
                1,
                &err);

    BSP_Init();                                                   /* Initialize BSP functions                         */
    CPU_Init();                                                   /* Initialize the uC/CPU services                   */

    cpu_clk_freq = BSP_CPU_ClkFreq();                             /* Determine SysTick reference freq.                */                                                                        
    cnts         = cpu_clk_freq / (CPU_INT32U)OSCfg_TickRate_Hz;  /* Determine nbr SysTick increments                 */
    OS_CPU_SysTickInit(cnts);                                     /* Init uC/OS periodic time src (SysTick).          */

    
#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                                 /* Compute CPU capacity with no task running        */
#endif

    CPU_IntDisMeasMaxCurReset();
    
    OSTaskCreate((OS_TCB     *)&AppTask1_TCB,                // Create Task 01 
                 (CPU_CHAR   *)"App Task 01",
                 (OS_TASK_PTR )AppTask1, 
                 (void       *)0,
                 (OS_PRIO     )4,
                 (CPU_STK    *)&AppTask1_Stk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
    
    OSTaskCreate((OS_TCB     *)&AppTask2_TCB,                // Create Task 02                                
                 (CPU_CHAR   *)"App Task 02",
                 (OS_TASK_PTR )AppTask2, 
                 (void       *)0,
                 (OS_PRIO     )4,
                 (CPU_STK    *)&AppTask2_Stk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
    
    OSTaskCreate((OS_TCB     *)&AppTask3_TCB,                // Create Task 03 
                 (CPU_CHAR   *)"App Task 03",
                 (OS_TASK_PTR )AppTask3, 
                 (void       *)0,
                 (OS_PRIO     )4,
                 (CPU_STK    *)&AppTask3_Stk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
    
    OSTaskCreate((OS_TCB     *)&AppTask4_TCB,                // Create Task 04
                 (CPU_CHAR   *)"App Task 04",
                 (OS_TASK_PTR )AppTask4, 
                 (void       *)0,
                 (OS_PRIO     )4,
                 (CPU_STK    *)&AppTask4_Stk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
    
    OSTaskCreate((OS_TCB     *)&AppTask5_TCB,                // Create Task 05
                 (CPU_CHAR   *)"App Task 05",
                 (OS_TASK_PTR )AppTask5, 
                 (void       *)0,
                 (OS_PRIO     )5,
                 (CPU_STK    *)&AppTask5_Stk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

    OSTaskCreate((OS_TCB     *)&AppTask6_TCB,                // Create Task 06
                 (CPU_CHAR   *)"App Task 06",
                 (OS_TASK_PTR )AppTask6, 
                 (void       *)0,
                 (OS_PRIO     )4,
                 (CPU_STK    *)&AppTask6_Stk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

    OSTaskCreate((OS_TCB     *)&AppTask7_TCB,                // Create Task 07
                 (CPU_CHAR   *)"App Task 07",
                 (OS_TASK_PTR )AppTask7, 
                 (void       *)0,
                 (OS_PRIO     )3,
                 (CPU_STK    *)&AppTask7_Stk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
  
    OSTaskCreate((OS_TCB     *)&AppTask8_TCB,                // Create Task 08
                 (CPU_CHAR   *)"App Task 08",
                 (OS_TASK_PTR )AppTask8, 
                 (void       *)0,
                 (OS_PRIO     )6,
                 (CPU_STK    *)&AppTask8_Stk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
    
    
    BSP_LED_Off(0);

    while (DEF_TRUE) {                                            /* Task body, always written as an infinite loop.   */
        OSTimeDlyHMSM(1, 0, 0, 0,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }

        /* OSSemPend(&AppSem,
                  100,
                  OS_OPT_PEND_BLOCKING,
                  &ts,
                  &err); */
    
}

static  void  AppTask1      (void *p_arg)   //RFID
{
    OS_ERR      err;
    p_arg = p_arg;
    
    while(DEF_ON) {

    }
}
 
static void AppTask2        (void *p_arg)   //Proximity
{
    OS_ERR        err;
    p_arg = p_arg;
    
    while(DEF_ON) {
      PROX_Check();
    }
}


static  void  AppTask3      (void *p_arg)   //Temperature
{
    OS_ERR      err;
    p_arg = p_arg;
    
    while(DEF_ON) {

    }
}

static  void  AppTask4      (void *p_arg)   //Sanitizer
{
    OS_ERR      err;
    p_arg = p_arg;
    
    while(DEF_ON) {
    }
}

static  void  AppTask5      (void *p_arg)   //Door
{
    OS_ERR      err;
    p_arg = p_arg;
    CPU_TS  ts;
    
    while(DEF_ON) {
      
      OSSemPend(&DoorSem, 0, OS_OPT_PEND_BLOCKING, &ts, &err);    //4번째 argument는 function option. 생략 가능하다면 생략.
      switch(err) {                                             
        case OS_ERR_NONE:
            //door open function
            OSTimeDlyHMSM(0, 0, 5, 0, OS_OPT_TIME_HMSM_STRICT, &err);   //문 열리고 5초 딜레이
            //door close function
            OSSemPost(&DoorSem,
                      OS_OPT_POST_1,
                      &err);
            break;

        case OS_ERR_PEND_ABORT:
            break;
        
        case OS_ERR_OBJ_DEL:
            break;
        
        default:
            break;
      }

    }
}

static  void  AppTask6      (void *p_arg)   //PIR Sensor
{
    OS_ERR      err;
    p_arg = p_arg;
    
    while(DEF_ON) {
      PIR_Check();
    }
}

static  void  AppTask7      (void *p_arg)   //Timer Task(20sec)
{
    OS_ERR      err;
    p_arg = p_arg;
    
    while(DEF_ON) {

    }
}

static  void  AppTask8      (void *p_arg)   //LCD Task
{
    OS_ERR      err;
    p_arg = p_arg;
    
    while(DEF_ON) {

    }
}
/*
static  void  AppTask2      (void *p_arg)
{
    OS_ERR      err;
    p_arg = p_arg;
    
    while(DEF_ON) {
      BSP_LED_Toggle(2);
      OSTimeDlyHMSM(0, 0, 2, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

static  void  AppTask3      (void *p_arg)
{
    OS_ERR      err;
    p_arg = p_arg;
    
    while(DEF_ON) {
      BSP_LED_Toggle(3);
      OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

static  void  AppTask4      (void *p_arg)
{
    OS_ERR      err;
    p_arg = p_arg;
    
    while(DEF_ON) {
      BSP_LED_Toggle(4);
      OSTimeDlyHMSM(0, 0, 0, 500, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}
*/