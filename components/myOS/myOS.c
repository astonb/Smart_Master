//======================Copyright (c)=====================================
// 文件名称: myOS.c
// 功能描述: OS适配器
//
// 修改记录:
//           2019-03-20, luckx 创建文件
//
//
//========================================================================

#define LOG_DEBUG_EN 0

#include "myOS.h"

#include "esp_ota_ops.h"
#include "rom/md5_hash.h"

//= Start ================================================================
static const char *TAG = "OS";


#define RTOS_HIGHEST_PRIORITY (configMAX_PRIORITIES - 1)
#define MG_PRIORITY_TO_NATIVE_PRIORITY(priority) (uint8_t)(RTOS_HIGHEST_PRIORITY - priority)

#ifdef MCU_ESP32
static portMUX_TYPE os_mux = portMUX_INITIALIZER_UNLOCKED;
#define P_MUX    (&os_mux)
#else
#define P_MUX
#endif

//此函数中priority越大, 优先级越低, 与FreeRtos相反
OSStatus my_rtos_create_thread(uint8_t priority, const char *name, os_thread_function_t function, uint32_t stack_size)
{
    ESP_LOGI(TAG, "name:[ %s ],stack_size=[ %d ],priority[ %d],FreeMem[ %d ]", name, stack_size, priority, os_freemem_get());
    /* Limit priority to default lib priority */
    if (priority > RTOS_HIGHEST_PRIORITY)
    {
        priority = RTOS_HIGHEST_PRIORITY;
    }

#ifdef MCU_ESP32
    stack_size *= 2;    //ESP32 栈开大一点
#endif

    if (pdPASS == xTaskCreate((TaskFunction_t)function, name, (unsigned short)(stack_size / sizeof(portSTACK_TYPE)), NULL, MG_PRIORITY_TO_NATIVE_PRIORITY(priority), NULL))
    {
        ESP_LOGI(TAG, "name:[ %s ], FreeMem[%d]", name, os_freemem_get());
        return kNoErr;
    }
    else
    {
        return kGeneralErr;
    }
}

OSStatus my_rtos_delete_thread(os_thread_t *thread)
{
    if (thread == NULL)
    {
        vTaskDelete(NULL);
    }
    else
    {
        vTaskDelete(*thread);
    }

    return kNoErr;
}

void my_rtos_thread_sleep(uint32_t seconds)
{
    uint32_t ticks;

    ticks = (seconds * 1000) / portTICK_PERIOD_MS;
    if (ticks == 0)
        ticks = 1;

    vTaskDelay((portTickType)ticks);
}

void my_rtos_thread_msleep(uint32_t mseconds)
{
    uint32_t ticks;

    ticks = mseconds / portTICK_PERIOD_MS;
    if (ticks == 0)
        ticks = 1;

    vTaskDelay((portTickType)ticks);
}

OSStatus my_rtos_init_queue(os_queue_t *queue, const char *name, uint32_t message_size, uint32_t number_of_messages)
{
    if ((*queue = xQueueCreate(number_of_messages, message_size)) == NULL)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus my_rtos_push_to_queue(os_queue_t *queue, void *message, uint32_t timeout_ms)
{
    signed portBASE_TYPE result;

    result = xQueueSendToBack(*queue, message, (portTickType)(timeout_ms / portTICK_PERIOD_MS));
    return (result == pdPASS) ? kNoErr : kGeneralErr;
}

OSStatus my_rtos_pop_from_queue(os_queue_t *queue, void *message, uint32_t timeout_ms)
{
    if (xQueueReceive(*queue, message, (timeout_ms / portTICK_PERIOD_MS)) != pdPASS)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

//得到空闲队列数目
int my_rtos_queue_SpacesAvailable(os_queue_t *queue)
{
    return uxQueueSpacesAvailable(*queue);
}

bool my_rtos_is_queue_empty(os_queue_t *queue)
{
    signed portBASE_TYPE result;

    TaskENTER_CRITICAL(P_MUX);
    result = xQueueIsQueueEmptyFromISR(*queue);
    TaskEXIT_CRITICAL(P_MUX);

    return (result != 0) ? true : false;
}

bool my_rtos_is_queue_full(os_queue_t *queue)
{
    signed portBASE_TYPE result;

    TaskENTER_CRITICAL(P_MUX);
    result = xQueueIsQueueFullFromISR(*queue);
    TaskEXIT_CRITICAL(P_MUX);

    return (result != 0) ? true : false;
}

static void timer_callback(xTimerHandle handle)
{
    os_timer_t *timer = (os_timer_t *)pvTimerGetTimerID(handle);

    if (timer->function)
    {
        timer->function(timer->arg);
    }
}

OSStatus os_init_timer(os_timer_t *timer, uint32_t time_ms, TimerCallbackFunction_t function, void *arg)
{
    timer->function = function;
    timer->arg = arg;

    timer->handle = xTimerCreate("", (portTickType)(time_ms / portTICK_PERIOD_MS), pdTRUE, timer, (TimerCallbackFunction_t)timer_callback);

    if (timer->handle == NULL)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus os_start_timer(os_timer_t *timer)
{
    signed portBASE_TYPE result;

    result = xTimerStart(timer->handle, OS_WAIT_FOREVER);

    if (result != pdPASS)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus os_stop_timer(os_timer_t *timer)
{
    signed portBASE_TYPE result;

    result = xTimerStop(timer->handle, OS_WAIT_FOREVER);

    if (result != pdPASS)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus my_rtos_init_semaphore(os_semaphore_t *semaphore, int count)
{
    *semaphore = xSemaphoreCreateCounting((unsigned portBASE_TYPE)count, (unsigned portBASE_TYPE)0);

    return (*semaphore != NULL) ? kNoErr : kGeneralErr;
}

OSStatus my_rtos_deinit_semaphore(os_semaphore_t *semaphore)
{
    if (semaphore != NULL)
    {
        vSemaphoreDelete(*semaphore);
        *semaphore = NULL;
    }
    return kNoErr;
}

OSStatus my_rtos_get_semaphore(os_semaphore_t *semaphore, uint32_t timeout_ms)
{
    if (pdTRUE == xSemaphoreTake(*semaphore, (portTickType)(timeout_ms / portTICK_PERIOD_MS)))
    {
        return kNoErr;
    }
    else
    {
        return kTimeoutErr;
    }
}


int my_rtos_set_semaphore(os_semaphore_t *semaphore)
{
    signed portBASE_TYPE result;
    result = xSemaphoreGive(*semaphore);
    return (result == pdPASS) ? kNoErr : kGeneralErr;
}

OSStatus my_rtos_init_mutex(os_mutex_t *mutex)
{
    /* Mutex uses priority inheritance */
    *mutex = xSemaphoreCreateMutex();
    if (*mutex == NULL)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus my_rtos_lock_mutex(os_mutex_t *mutex)
{
    if (xSemaphoreTake(*mutex, OS_WAIT_FOREVER) != pdPASS)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus my_rtos_unlock_mutex(os_mutex_t *mutex)
{
    if (xSemaphoreGive(*mutex) != pdPASS)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

OSStatus my_rtos_deinit_mutex(os_mutex_t *mutex)
{
    vSemaphoreDelete(*mutex);
    *mutex = NULL;
    return kNoErr;
}

void my_rtos_reboot()
{
    my_rtos_thread_msleep(200);
    esp_restart();
}

uint32_t os_freemem_get()
{
    return esp_get_free_heap_size();
}
