#include <stdio.h>
#include "thread_pool.h"

void task_func(void* arg) {
    int num = *(int*)arg;
    printf("tid = %ld,num = %d\n", pthread_self(), num);
    sleep(1);
}

int main() {
    ThreadPool* pool = threadPoolInit(10, 10);
    for (int i = 0; i < 50; i++) {
        int* num = (int*)malloc(sizeof(int));
        *num = i;
        taskQueuePush(&pool->task_queue, task_func, num);
    }
    sleep(10);
    threadPoolDestroy(pool);
    return 0;
}