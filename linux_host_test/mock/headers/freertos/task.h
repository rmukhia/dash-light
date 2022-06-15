//
// Created by rmukhia on 2/6/22.
//

#ifndef TEST_APP_TASK_H
#define TEST_APP_TASK_H

#include <unistd.h>

typedef void* TaskHandle_t;

#define tskIDLE_PRIORITY 0


void xTaskCreate(void *a, char *b, size_t c, void *d, int e, TaskHandle_t *f)
{
    *f = (void *)0x1;
}

void vTaskDelete(TaskHandle_t t)
{

}

void vTaskDelay(int t)
{
    //sleep(t);
}

#endif //TEST_APP_TASK_H
