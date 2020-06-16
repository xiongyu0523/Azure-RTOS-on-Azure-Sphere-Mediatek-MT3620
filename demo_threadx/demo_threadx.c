/* This is a small demo of the high-performance ThreadX kernel.  It includes examples of eight
   threads of different priorities, using a message queue, semaphore, mutex, event flags group, 
   byte pool, and block pool.  */

#include "tx_api.h"
#include "printf.h"

#include "os_hal_gpio.h"

#define DEMO_STACK_SIZE         1024
#define DEMO_BYTE_POOL_SIZE     9120

/* Define the ThreadX object control blocks...  */
TX_THREAD               thread_0;
TX_THREAD               thread_1;

TX_EVENT_FLAGS_GROUP    event_flags_0;
TX_BYTE_POOL            byte_pool_0;
UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];

int gpio_toggle(u8 gpio_no)
{
    static u8 level = 0;
    int ret;

    ret = mtk_os_hal_gpio_request(gpio_no);
    if (ret != 0) {
        printf("request gpio[%d] fail\n", gpio_no);
        return ret;
    }
    mtk_os_hal_gpio_set_direction(gpio_no, OS_HAL_GPIO_DIR_OUTPUT);
    mtk_os_hal_gpio_set_output(gpio_no, level);
    level = !level;
    ret = mtk_os_hal_gpio_free(gpio_no);
    if (ret != 0) {
        printf("free gpio[%d] fail\n", gpio_no);
        return ret;
    }
    return 0;
}

void thread_0_entry(ULONG thread_input)
{
    printf("thread 0 start\n");

    /* This thread simply sits in while-forever-sleep loop.  */
    while(1)
    {
        /* Sleep for a few ticks.  */
        tx_thread_sleep(50);

        /* Set event flag 0 to wakeup thread 1.  */
        tx_event_flags_set(&event_flags_0, 0x1, TX_OR);
    }
}

void thread_1_entry(ULONG thread_input)
{
    printf("thread 1 start\n");

    UINT    status;
    ULONG   actual_flags;

    /* This thread simply waits for an event in a forever loop.  */
    while (1)
    {
        /* Wait for event flag 0.  */
        status = tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR,
                                    &actual_flags, TX_WAIT_FOREVER);

        /* Check status.  */
        if ((status != TX_SUCCESS) || (actual_flags != 0x1)) {
            printf("tx_event_flags_get fail\n");
            break;
        } 

        gpio_toggle(OS_HAL_GPIO_8);
    }
}

/* Define what the initial system looks like.  */
void tx_application_define(void* first_unused_memory)
{
    CHAR* pointer;

    /* Create a byte memory pool from which to allocate the thread stacks.  */
    tx_byte_pool_create(&byte_pool_0, "byte pool 0", memory_area, DEMO_BYTE_POOL_SIZE);

    /* Allocate the stack for thread 0.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create the first thread.  */
    tx_thread_create(&thread_0,
        "thread 0",
        thread_0_entry,
        0,
        pointer,
        DEMO_STACK_SIZE,
        1,
        1,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);


    /* Allocate the stack for thread 1.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create the second thread */
    tx_thread_create(&thread_1,
        "thread 1",
        thread_1_entry,
        0,
        pointer,
        DEMO_STACK_SIZE,
        2,
        2,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    /* Create the event flags group used by threads 0 and 1.  */
    tx_event_flags_create(&event_flags_0, "event flags 0");
}

/* Define main entry point.  */
int main(void)
{
    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}