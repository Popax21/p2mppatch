#include <assert.h>
#include <time.h>
#include <math.h>

//The SDK we link against is obviously not the actual P2 SDK, so we have to do some hacky jank to make it work ._.
//We use the older __sync primitives as the SDK GCC is too old for the __atomic ones

extern "C" {

const char *__asan_default_options() {
    //Needed for debugging
    return "verify_asan_link_order=false:abort_on_error=1:detect_leaks=0";
}

double __pow_finite(double a, double b) {
    return pow(a, b);
}

struct tm *Plat_localtime(const time_t *timep, struct tm *result) {
#ifdef LINUX
    return localtime_r(timep, result);
#else
    assert(localtime_s(result, timep) == 0);
    return result;
#endif
}

long ThreadInterlockedIncrement(long volatile *p) {
    return __sync_add_and_fetch(p, 1);
}

long ThreadInterlockedDecrement(long volatile *p) {
    return __sync_sub_and_fetch(p, 1);
}

long ThreadInterlockedExchange(long volatile *p, long value) {
    return __sync_lock_test_and_set(p, value);
}

long ThreadInterlockedExchangeAdd(long volatile *p, long value) {
    return __sync_fetch_and_add(p, value);
}

long ThreadInterlockedCompareExchange(long volatile *p, long value, long comperand)  {
    return __sync_val_compare_and_swap(p, comperand, value);
}

bool ThreadInterlockedAssignIf(long volatile *p, long value, long comperand) {
    return __sync_bool_compare_and_swap(p, comperand, value);
}

void *ThreadInterlockedExchangePointer(void * volatile *p, void *value) {
    return __sync_lock_test_and_set(p, value);
}

void *ThreadInterlockedCompareExchangePointer(void * volatile *p, void *value, void *comperand) {
    return __sync_val_compare_and_swap(p, comperand, value);
}

bool ThreadInterlockedAssignPointerIf(void * volatile *p, void *value, void *comperand) {
    return __sync_bool_compare_and_swap(p, comperand, value);
}

}