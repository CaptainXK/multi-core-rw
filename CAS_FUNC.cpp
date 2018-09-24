#include <stdio.h>

#include <CAS_FUNC.hpp>

bool DO_CAS(int * ptr, int oldval, int newval){
    return __sync_bool_compare_and_swap(ptr, oldval, newval);
}