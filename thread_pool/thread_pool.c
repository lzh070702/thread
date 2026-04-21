#include "thread_pool.h"

void taskQueueInit(TaskQueue* queue, int capacity) {
    queue->capacity = capacity;
    queue->tasks = (Task*)malloc(sizeof(Task) * capacity);
    queue->size = 0;
    queue->front = 0;
    queue->rear = 0;
    queue->shutdown = false;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->full_cond, NULL);
    pthread_cond_init(&queue->empty_cond, NULL);
}

void taskQueueDestroy(TaskQueue* queue) {
    free(queue->tasks);
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->full_cond);
    pthread_cond_destroy(&queue->empty_cond);
}

ThreadPool* threadPoolInit(int thread_cnt, int capacity) {
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (!pool) {
        return NULL;
    }
    taskQueueInit(&pool->task_queue, capacity);
    pool->thread_cnt = thread_cnt;
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_cnt);
    if (!pool->threads) {
        taskQueueDestroy(&pool->task_queue);
        free(pool);
        return NULL;
    }
    for (int i = 0; i < thread_cnt; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker, pool) != 0) {
            pool->task_queue.shutdown = true;
            pthread_cond_broadcast(&pool->task_queue.empty_cond);
            for (int j = 0; j < i; j++) {
                pthread_join(pool->threads[j], NULL);
            }
            taskQueueDestroy(&pool->task_queue);
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }
    return pool;
}

void threadPoolDestroy(ThreadPool* pool) {
    if (!pool) {
        return;
    }
    pthread_mutex_lock(&pool->task_queue.mutex);
    pool->task_queue.shutdown = true;
    pthread_mutex_unlock(&pool->task_queue.mutex);
    pthread_cond_broadcast(&pool->task_queue.empty_cond);
    pthread_cond_broadcast(&pool->task_queue.full_cond);
    for (int i = 0; i < pool->thread_cnt; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    taskQueueDestroy(&pool->task_queue);
    free(pool->threads);
    free(pool);
}

void taskQueuePush(TaskQueue* queue, void (*func)(void*), void* arg) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->size == queue->capacity && !queue->shutdown) {
        pthread_cond_wait(&queue->full_cond, &queue->mutex);
    }
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return;
    }
    queue->tasks[queue->rear].func = func;
    queue->tasks[queue->rear].arg = arg;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->size++;
    pthread_cond_signal(&queue->empty_cond);
    pthread_mutex_unlock(&queue->mutex);
}

bool taskQueuePop(TaskQueue* queue, Task* task) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->size == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->empty_cond, &queue->mutex);
    }
    if (queue->shutdown && queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    *task = queue->tasks[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    pthread_cond_signal(&queue->full_cond);
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

void* worker(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    Task task;
    while (1) {
        if (!taskQueuePop(&pool->task_queue, &task)) {
            break;
        }
        task.func(task.arg);
    }
    return NULL;
}