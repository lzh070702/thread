#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t cond;

typedef struct Node {
    int data;
    struct Node* next;
} Node;

Node* head = NULL;

void* producer() {
    while (1) {
        pthread_mutex_lock(&mutex);
        Node* node = (Node*)malloc(sizeof(Node));
        node->data = rand() % 100;
        node->next = head;
        head = node;
        printf("producer_tid:%lu,num:%d\n", pthread_self(), head->data);
        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&cond);
        sleep(rand() % 5);
    }
    return NULL;
}
void* consumer() {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (!head) {
            pthread_cond_wait(&cond, &mutex);
        }
        printf("consumer_tid:%lu,num:%d\n", pthread_self(), head->data);
        Node* del_node = head;
        head = head->next;
        free(del_node);
        pthread_mutex_unlock(&mutex);
        sleep(rand() % 5);
    }
    return NULL;
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_t t1[3], t2[3];
    for (int i = 0; i < 3; i++) {
        pthread_create(&t1[i], NULL, producer, NULL);
    }
    for (int i = 0; i < 3; i++) {
        pthread_create(&t2[i], NULL, consumer, NULL);
    }
    for (int i = 0; i < 3; ++i) {
        pthread_join(t1[i], NULL);
    }

    for (int i = 0; i < 3; ++i) {
        pthread_join(t2[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}
