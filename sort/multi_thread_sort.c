#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DATA_SIZE 100000

typedef struct {
    int* arr;
    int begin;
    int end;
    int len;
} SubArr;

int threads_cnt = 0;
int sub_arr_len = 0;

void arr_init(void) {
    threads_cnt = sysconf(_SC_NPROCESSORS_ONLN);
    sub_arr_len = DATA_SIZE / threads_cnt;
}

void merge(int arr[], int arr1[], int len1, int arr2[], int len2) {
    int i = 0, j = 0, k = 0;
    while (i < len1 && j < len2) {
        if (arr1[i] < arr2[j]) {
            arr[k] = arr1[i];
            i++;
        } else {
            arr[k] = arr2[j];
            j++;
        }
        k++;
    }
    while (i < len1) {
        arr[k++] = arr1[i++];
    }
    while (j < len2) {
        arr[k++] = arr2[j++];
    }
}

void sub_arr_merge_sort(int arr[], int len) {
    if (len <= 1) {
        return;
    }
    int mid = len / 2;
    int* left = (int*)malloc(mid * sizeof(int));
    int* right = (int*)malloc((len - mid) * sizeof(int));
    memcpy(left, arr, mid * sizeof(int));
    memcpy(right, arr + mid, (len - mid) * sizeof(int));
    sub_arr_merge_sort(left, mid);
    sub_arr_merge_sort(right, len - mid);
    merge(arr, left, mid, right, len - mid);
    free(left);
    free(right);
}

void* sort(void* arg) {
    SubArr* subarr = (SubArr*)arg;
    sub_arr_merge_sort(subarr->arr + subarr->begin, subarr->len);
    return NULL;
}

void arr_merge_sort(int arr[], SubArr* sub_arrs, int threads_cnt) {
    if (threads_cnt <= 1) {
        return;
    }
    int mid = threads_cnt / 2;
    SubArr* left = (SubArr*)malloc(mid * sizeof(SubArr));
    SubArr* right = (SubArr*)malloc((threads_cnt - mid) * sizeof(SubArr));
    memcpy(left, sub_arrs, mid * sizeof(SubArr));
    memcpy(right, sub_arrs + mid, (threads_cnt - mid) * sizeof(SubArr));
    int* left_arr = (int*)malloc(mid * (sub_arr_len + 1) * sizeof(int));
    int* right_arr =
        (int*)malloc((threads_cnt - mid) * (sub_arr_len + 1) * sizeof(int));
    int left_arr_len = 0;
    for (int i = 0; i < mid; i++) {
        memcpy(left_arr + left_arr_len, sub_arrs[i].arr + sub_arrs[i].begin,
               sub_arrs[i].len * sizeof(int));
        left_arr_len += sub_arrs[i].len;
    }
    int right_arr_len = 0;
    for (int i = 0; i < threads_cnt - mid; i++) {
        memcpy(right_arr + right_arr_len,
               sub_arrs[mid + i].arr + sub_arrs[mid + i].begin,
               sub_arrs[mid + i].len * sizeof(int));
        right_arr_len += sub_arrs[mid + i].len;
    }
    arr_merge_sort(left_arr, left, mid);
    arr_merge_sort(right_arr, right, threads_cnt - mid);
    merge(arr, left_arr, left_arr_len, right_arr, right_arr_len);
    free(left);
    free(right);
    free(left_arr);
    free(right_arr);
}

int main() {
    int* data = (int*)malloc(DATA_SIZE * sizeof(int));
    srand((unsigned int)time(NULL));
    for (int i = 0; i < DATA_SIZE; i++) {
        data[i] = rand() % 100000 + 1;
    }
    arr_init();
    int extra_len = DATA_SIZE % threads_cnt;
    SubArr sub_arrs[threads_cnt];
    pthread_t threads[threads_cnt];
    int pos = 0;
    for (int i = 0; i < threads_cnt; i++) {
        sub_arrs[i].arr = data;
        sub_arrs[i].begin = pos;
        sub_arrs[i].end = pos + sub_arr_len - 1;
        sub_arrs[i].len = sub_arr_len;
        if (i < extra_len) {
            sub_arrs[i].end++;
            sub_arrs[i].len++;
        }
        pos = sub_arrs[i].end + 1;
        pthread_create(&threads[i], NULL, sort, &sub_arrs[i]);
    }
    for (int i = 0; i < threads_cnt; i++) {
        pthread_join(threads[i], NULL);
    }
    arr_merge_sort(data, sub_arrs, threads_cnt);
    FILE* fp = fopen("sorted_output.txt", "w");
    if (fp) {
        for (int i = 0; i < DATA_SIZE; i++) {
            fprintf(fp, "%d\n", data[i]);
        }
        fclose(fp);
    }
    free(data);
    return 0;
}