// Makes assert() print out a stack trace on unixlike systems. There is a windows version, but I did not implement.
//To use, call setup_stack_dump_on_assert() from main.

// Adapted from rafael on https://oroboro.com/stack-trace-on-crash/, who has graciously put it in the public domain
#ifndef P_STACK_DUMP_ON_ASSERT_H
#define P_STACK_DUMP_ON_ASSERT_H
#ifdef __cplusplus
extern "C" {
#endif

void setup_stack_dump_on_assert();

#ifdef __cplusplus
}
#endif

#endif // P_STACK_DUMP_ON_ASSERT_H
