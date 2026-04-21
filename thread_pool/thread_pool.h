#ifndef thread_pool_h
#define thread_pool_h

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct Task {
    void (*func)(void* arg);
    void* arg;
} Task;

typedef struct TaskQueue {
    Task* tasks;
    int capacity;
    int size;
    int front;
    int rear;
    pthread_mutex_t mutex;
    pthread_cond_t full_cond;
    pthread_cond_t empty_cond;
    bool shutdown;
} TaskQueue;

typedef struct ThreadPool {
    TaskQueue task_queue;
    pthread_t* threads;
    int thread_cnt;
} ThreadPool;

void taskQueueInit(TaskQueue* queue, int capacity);
void taskQueueDestroy(TaskQueue* queue);
ThreadPool* threadPoolInit(int thread_cnt, int capacity);
void threadPoolDestroy(ThreadPool* pool);
void taskQueuePush(TaskQueue* queue, void (*func)(void*), void* arg);
bool taskQueuePop(TaskQueue* queue, Task* task);
void* worker(void* arg);
#endif