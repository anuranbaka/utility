#ifndef UTIL_P_TIME
uint64_t nanotime();
void current_utc_time(struct timespec *ts);
#define UTIL_P_TIME
