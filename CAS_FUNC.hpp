#ifndef __CAS_FUNC__
#define __CAS_FUNC__


#include <stdbool.h>

template<class T>
bool DO_CAS(T * ptr, T oldval, T newval)
{
    return __sync_bool_compare_and_swap(ptr, oldval, newval);
}

#endif