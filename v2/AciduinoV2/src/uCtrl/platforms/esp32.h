#include <Arduino.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// forward declaration of uClockHandler
void uCtrlHandler();

namespace uctrl {

#define TIMER_ID	1
hw_timer_t * _uctrlTimer = NULL;
// mutex control for ISR
//portMUX_TYPE _uctrlTimerMux = portMUX_INITIALIZER_UNLOCKED;
//#define ATOMIC(X) portENTER_CRITICAL_ISR(&_uctrlTimerMux); X; portEXIT_CRITICAL_ISR(&_uctrlTimerMux);

// FreeRTOS main clock task size in bytes
//#define CTRL_STACK_SIZE     2048
#define CTRL_STACK_SIZE     5*1024
//#define CTRL_STACK_SIZE     configMINIMAL_STACK_SIZE
#define CTRL_TASK_PRIORITY  tskIDLE_PRIORITY + 2
TaskHandle_t _taskHandle;
// mutex to protect the shared resource
SemaphoreHandle_t _mutex;
// mutex control for task
#define ATOMIC(X) xSemaphoreTake(_mutex, portMAX_DELAY); X; xSemaphoreGive(_mutex);

// ISR handler
void ARDUINO_ISR_ATTR handlerISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // send the notification to ctrlTask
    vTaskNotifyGiveFromISR(_taskHandle, &xHigherPriorityTaskWoken);
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// task for user clock process
void ctrlTask(void *pvParameters)
{
    while (1) {
        // wait for a notification from ISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uCtrlHandler();
    }
}

void initTimer(uint32_t init_clock)
{
    // initialize the mutex for shared resource access
    _mutex = xSemaphoreCreateMutex();

    // create the ctrlTask
    xTaskCreate(ctrlTask, "ctrlTask", CTRL_STACK_SIZE, NULL, CTRL_TASK_PRIORITY, &_taskHandle);

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    // ESP32 Arduino core 3.x: timerBegin() takes the frequency directly.
    // The 2.x setup used prescaler 60 of the 80 MHz APB clock = 1.333 MHz, so
    // request the same frequency to keep init_clock alarm units identical.
    _uctrlTimer = timerBegin(1333333);

    // attach to generic uctrl ISR (2-arg form on 3.x)
    timerAttachInterrupt(_uctrlTimer, &handlerISR);

    // init clock tick time and activate it (autoreload, unlimited reloads)
    timerAlarm(_uctrlTimer, init_clock, true, 0);
#else
    _uctrlTimer = timerBegin(TIMER_ID, 60, true);

    // attach to generic uclock ISR
    timerAttachInterrupt(_uctrlTimer, &handlerISR, false);

    // init clock tick time
    timerAlarmWrite(_uctrlTimer, init_clock, true);

    // activate it!
    timerAlarmEnable(_uctrlTimer);
#endif
}

} // end namespace uctrl
