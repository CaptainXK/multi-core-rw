#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sched.h>
#include <iostream>
#include <thread>
#include <functional>
#include <signal.h>
#include <inttypes.h>

#include <CAS_FUNC.hpp>
#include <numa-arch-helper.hpp>

#define DATA_NB 1024
#define MAX_THD_NB 2

//functions
void test_data_init();
void env_init();
void env_uinit();

//thread var
int force_quit;
uint8_t self_lock;
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
shared_data test_data;

void env_init()
{
    init_numa_cpu_env();

    test_data_init();

    self_lock = 0xFF;
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

    uint8_t lock_mask = (uint8_t)1 << id; 

//    printf("Target CPU id=%d, self lock mask = %.2hhx, inverse mask = %.2hhx\n", id, lock_mask, (uint8_t)~lock_mask);

    int old_head;
    int old_thd_count;
    uint8_t old_lock;
   
    while(!force_quit){
        //test self lock
        while( (self_lock & lock_mask ) == 0 );

        //wait work start
        while(test_data.start_work == 0);

        //start to work

        //CAS op to find work area 
        do{
            old_head = test_data.head;
        }while(!DO_CAS(&(test_data.head), old_head, old_head + thd_stride));

        //do work
        for(int i = old_head; i < (old_head + thd_stride) && i < DATA_NB; ++i){
            test_data.data[i] = tar_cpu + 1;
        }

        //CAS op to modify shared count var
        do{
            old_thd_count = test_data.thd_count;
        }while(!DO_CAS(&test_data.thd_count, old_thd_count, old_thd_count + 1));

        //lock self lock
        do{
            old_lock = self_lock; 
        }while(!DO_CAS(&self_lock, old_lock, (uint8_t)(old_lock & ~lock_mask) ) );        
    }
}

void assign_thread(std::thread ** thd, std::function<void(int)> func, int id)
{
    *thd = new std::thread(func, id);
}

void signal_handler(int sig)
{
    force_quit = 1;

    env_uinit();
}

void sig_init()
{
    force_quit = 0;
    signal(SIGINT, signal_handler);
}

int main(int argc, char** argv)
{
    env_init();

    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&cond_mutex, NULL);

    int i = 0;
    int test_count = 0;
    uint8_t old_lock;

    for(i = 0; i < MAX_THD_NB; ++i){
        assign_thread(&(thds[i]), thd_func, i);
        thread_migrate(*(thds[i]));
    }

    while(1){
        //start work
        test_data.start_work = 1;    

        //wait work over
        while(test_data.thd_count != MAX_THD_NB);

        int error_count = 0;

        for(i = 0; i < DATA_NB; ++i){
            if(test_data.data[i] == 0)
                error_count++;
            test_data.data[i] = 0;
        }

        printf("%d:error rate %d/%d\n", ++test_count % 10, error_count, DATA_NB);

        error_count = 0;

        test_data.head = 0;

        test_data.thd_count = 0;

        //this line can obtain more better stability, but i have not found the reason yet
        __sync_synchronize();

        do{
            old_lock = self_lock;
        }while(!DO_CAS(&self_lock, old_lock, (uint8_t)(old_lock | (uint8_t)0xFF)) );
        //self_lock |= 0xFF;
    }

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
