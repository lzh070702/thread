#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATA_SIZE 100000

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

void merge_sort(int arr[], int len) {
    if (len <= 1) {
        return;
    }
    int mid = len / 2;
    int* left = (int*)malloc(mid * sizeof(int));
    int* right = (int*)malloc((len - mid) * sizeof(int));
    memcpy(left, arr, mid * sizeof(int));
    memcpy(right, arr + mid, (len - mid) * sizeof(int));
    merge_sort(left, mid);
    merge_sort(right, len - mid);
    merge(arr, left, mid, right, len - mid);
    free(left);
    free(right);
}

int main() {
    int* data = (int*)malloc(DATA_SIZE * sizeof(int));
    srand((unsigned int)time(NULL));
    for (int i = 0; i < DATA_SIZE; i++) {
        data[i] = rand() % 100000 + 1;
    }
    merge_sort(data, DATA_SIZE);
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