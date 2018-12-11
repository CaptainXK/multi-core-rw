#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sched.h>
#include <iostream>
#include <thread>

#include <CAS_FUNC.hpp>
#include <numa-arch-helper.hpp>
#include <time_measurer.hpp>

#define DATA_NB 40960
#define MAX_THD_NB 4

//functions
void test_data_init();
void env_init();
void env_uinit();

//thread var
pthread_cond_t cond;
pthread_mutex_t cond_mutex;
std::thread * thds[MAX_THD_NB];
int thd_stride;

//shared data
struct shared_data{
    int start_work;
    int thd_count;
    int head;
    int data[DATA_NB];
};
typedef struct shared_data shared_data;
volatile shared_data test_data;

void env_init()
{
    init_numa_cpu_env();
    
    test_data_init();
}

void env_uinit()
{
    for(int i = 0; i<MAX_THD_NB; ++i){
        thds[i]->join();
        delete thds[i];
    }
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

void thd_func(int id)
{
    int tar_cpu = id;

/*
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
*/

    while(test_data.start_work == 0);        

    //start to work
    int old_head;

    //CAS op to find work area 
    do{
        old_head = test_data.head;
    }while(!DO_CAS(&(test_data.head), old_head, old_head + thd_stride));

    for(int i = old_head; i < (old_head + thd_stride) && i < DATA_NB; ++i){
        test_data.data[i] = tar_cpu + 1;
    }

    
    //CAS op to modify shared count var
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

    for(i = 0; i < MAX_THD_NB; ++i){
    //    pthread_create(&thds[i], NULL, thd_func, &i);
        thds[i] = new std::thread(thd_func, i);
        thread_migrate(*(thds[i]));
    }

    test_data.start_work = 1;    

    time_start();

    //poll working status
    while(test_data.thd_count != MAX_THD_NB);

    time_stop();

    time_report();

    int error_count = 0;

    for(i = 0; i < DATA_NB; ++i){
        if(test_data.data[i] == 0)
            error_count++;
    }

    printf("error rate %d/%d\n", error_count, DATA_NB);

#ifdef DEBUG
    for(i = 0; i < MAX_THD_NB; ++i){
        printf("thd%d:\n", i);

        for(int j = i*thd_stride, k=0; k < thd_stride; k++){
            printf("%d", test_data.data[j + k]);
        }

        printf("\n");
    }
#endif

    env_uinit();

    return 0;
}
