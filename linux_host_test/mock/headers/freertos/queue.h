//
// Created by rmukhia on 2/6/22.
//

#ifndef TEST_APP_QUEUE_H
#define TEST_APP_QUEUE_H

#include <stddef.h>

typedef void* QueueHandle_t;

void * xQueueCreate(int a, size_t b)
{
    return (void *)0x1;
}

void vQueueDelete(QueueHandle_t) {

}

void xQueueReceive(QueueHandle_t t,  void *type, int del)
{

}
#endif //TEST_APP_QUEUE_H
