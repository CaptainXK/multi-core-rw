#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sched.h>

#include "CAS_FUNC.h"

#define DATA_NB 1024
#define MAX_THD_NB 2

//functions
void thd_func();
void test_data_init();
void env_init();
void env_uinit();

//thread var
pthread_cond_t cond;
pthread_mutex_t cond_mutex;
pthread_t * thds;
int thd_stride;

//shared data
struct shared_data{
    int start_work;
    int thd_count;
    int head;
    int data[DATA_NB];
};
typedef struct shared_data shared_data;
shared_data test_data;

void env_init()
{
    thds = NULL;

    thds = (pthread_t*)calloc(MAX_THD_NB, sizeof(pthread_t));

    test_data_init();
}

void env_uinit()
{
    free(thds);
}

void test_data_init()
{
    test_data.thd_count = 0;
    test_data.head = 0;
    test_data.start_work = 0;

    for(int i = 0; i<DATA_NB; ++i){
        test_data.data[i] = 0;
    }

    thd_stride = DATA_NB / MAX_THD_NB;
}

void thd_func(void* arg)
{
    int tar_cpu = *((int*)arg);

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(tar_cpu, &cpu_set);

    if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set) != 0){
        fprintf(stderr, "cpu migrate error");
        exit(1);
    }
    else{
        printf("[CPU#%d working thread is lunched done]\n", sched_getcpu());
    }

    pthread_cond_signal(&cond);

    while(test_data.start_work == 0);        

    //start to work
    int old_head;

    do{
        old_head = test_data.head;
    }while(!DO_CAS(&(test_data.head), old_head, old_head + thd_stride));

    for(int i = old_head; i < (old_head + thd_stride) && i < DATA_NB; ++i){
        test_data.data[i] = tar_cpu + 1;
    }

    int old_thd_count;

    do{
        old_thd_count = test_data.thd_count;
    }while(!DO_CAS(&test_data.thd_count, old_thd_count, old_thd_count + 1));

}

int main(int argc, char** argv)
{
    env_init();

    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&cond_mutex, NULL);

    int i = 0;

    for(i = 0; i < 2; ++i){
        pthread_create(&thds[i], NULL, (void*)(&thd_func), &i);
        pthread_cond_wait(&cond, &cond_mutex);
    }

    test_data.start_work = 1;    

    while(test_data.thd_count != MAX_THD_NB);

    int error_count = 0;

    for(i = 0; i < DATA_NB; ++i){
        if(test_data.data[i] != 1)
            error_count++;
    }

    printf("error rate %d/%d\n", error_count, DATA_NB);

    for(i = 0; i < MAX_THD_NB; ++i){
        printf("thd%d:\n", i);

        for(int j = i*thd_stride, k=0; k < thd_stride; k++){
            printf("%d", test_data.data[j + k]);
        }

        printf("\n");
    }

    env_uinit();

    return 0;
}