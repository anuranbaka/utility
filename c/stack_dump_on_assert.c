// Makes assert() print out a stack trace on unix-like systems. There is a windows version, but I did not implement it.
// To use, call setup_stack_dump_on_assert() from main.

// Adapted by Paul Foster from rafael on https://oroboro.com/stack-trace-on-crash/, who has graciously put it in the public domain

//NOTE: I have tried to make this c compatible, but it won't demangle if you compile as c


#include <stdio.h> //fprintf
#include <signal.h>
#include <execinfo.h> //basic stack info

#include "stack_dump_on_assert.h"

#ifdef __cplusplus
    #include <errno.h>  //c++ demangle
    #include <cxxabi.h> //c++ demangle
    #include <cstdlib>

    extern "C" {
    static char** backtraceSymbols( void* const* addrList, int numAddr );
#else
    #include <stdlib.h>
#endif

static inline void printStackTrace( FILE *out, unsigned int max_frames);
static void abortHandler( int signum );


void setup_stack_dump_on_assert()
{
  signal( SIGABRT, abortHandler );
  signal( SIGSEGV, abortHandler );
  signal( SIGILL,  abortHandler );
  signal( SIGFPE,  abortHandler );
}

static void abortHandler( int signum )
{
   // associate each signal with a signal name string.
   const char* name = NULL;
   switch( signum )
   {
   case SIGABRT: name = "SIGABRT";  break;
   case SIGSEGV: name = "SIGSEGV";  break;
   case SIGBUS:  name = "SIGBUS";   break;
   case SIGILL:  name = "SIGILL";   break;
   case SIGFPE:  name = "SIGFPE";   break;
   }
 
   // Notify the user which signal was caught. We use printf, because this is the 
   // most basic output function. Once you get a crash, it is possible that more 
   // complex output systems like streams and the like may be corrupted. So we 
   // make the most basic call possible to the lowest level, most 
   // standard print function.
   if ( name )
      fprintf( stderr, "Caught signal %d (%s)\n", signum, name );
   else
      fprintf( stderr, "Caught signal %d\n", signum );
 
   // Dump a stack trace.
   printStackTrace(stderr,63);
 
   // If you caught one of the above signals, it is likely you just 
   // want to quit your program right now.
   exit( signum );
}

static inline void printStackTrace( FILE *out, unsigned int max_frames)
{
    fprintf(out, "stack trace:\n");

    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    unsigned int addrlen = backtrace( addrlist, sizeof( addrlist ) / sizeof( void* ));

    if ( addrlen == 0 )
    {
        fprintf( out, "  \n" );
        return;
    }


#ifndef __cplusplus
    //IF USING PURE C, USE THIS AS THE REMAINDER OF THE FUNCTION
    // resolve addresses into strings containing "filename(function+address)",
    // Actually it will be ## program address function + offset
    // this array must be free()-ed
    char** symbollist = backtrace_symbols( addrlist, addrlen );
    //IF USING PURE C, USE THIS AS THE REMAINDER OF THE FUNCTION
    // print the stack trace.
    unsigned int i;
    for (i = 4; i < addrlen; i++ )
        fprintf( out, "%s\n", symbollist[i]);

    free(symbollist);
#else //__cplusplus
    // resolve addresses into strings containing "filename(function+address)",
    // Actually it will be ## program address function + offset
    // this array must be free()-ed
    char** symbollist = backtrace_symbols( addrlist, addrlen );
    unsigned int i;


   size_t funcnamesize = 1024;
   char funcname[1024];
 
   // iterate over the returned symbol lines. skip the first, it is the
   // address of this function.
   for ( unsigned int i = 4; i < addrlen; i++ )
   {
      char* begin_name   = NULL;
      char* begin_offset = NULL;
      char* end_offset   = NULL;
 
      // find parentheses and +address offset surrounding the mangled name
#ifdef DARWIN
      // OSX style stack trace
      for ( char *p = symbollist[i]; *p; ++p )
      {
         if (( *p == '_' ) && ( *(p-1) == ' ' ))
            begin_name = p-1;
         else if ( *p == '+' )
            begin_offset = p-1;
      }
 
      if ( begin_name && begin_offset && ( begin_name < begin_offset ))
      {
         *begin_name++ = '\0';
         *begin_offset++ = '\0';
 
         // mangled name is now in [begin_name, begin_offset) and caller
         // offset in [begin_offset, end_offset). now apply
         // __cxa_demangle():
         int status;
         char* ret = abi::__cxa_demangle( begin_name, &funcname[0],
                                          &funcnamesize, &status );
         if ( status == 0 ) 
         {
            funcname = ret; // use possibly realloc()-ed string
            fprintf( out, "  %-30s %-40s %s\n",
                     symbollist[i], funcname, begin_offset );
         } else {
            // demangling failed. Output function name as a C function with
            // no arguments.
            fprintf( out, "  %-30s %-38s() %s\n",
                     symbollist[i], begin_name, begin_offset );
         }
 
#else // !DARWIN - but is posix
      // not OSX style
      // ./module(function+0x15c) [0x8048a6d]
      for ( char *p = symbollist[i]; *p; ++p )
      {
         if ( *p == '(' )
            begin_name = p;
         else if ( *p == '+' )
            begin_offset = p;
         else if ( *p == ')' && ( begin_offset || begin_name ))
            end_offset = p;
      }

 
      if ( begin_name && end_offset && ( begin_name < end_offset ))
      {
         *begin_name++   = '\0';
         *end_offset++   = '\0';
         if ( begin_offset )
            *begin_offset++ = '\0';

         // mangled name is now in [begin_name, begin_offset) and caller
         // offset in [begin_offset, end_offset).

         // IN: usr/local/lib/libopencv_core.so.3.2(_ZNK2cv12_OutputArray6createEiiiibi+0x5c5) [0x7f9c569d5dd5]
         // DO: addr2line  -i -e /usr/local/lib/libopencv_core.so.3.2  $( printf '%x\n' $((0x`nm /usr/local/lib/libopencv_core.so.3.2 | grep _ZNK2cv12_OutputArray6createEiiiibi| cut -d " " -f 1` +0x5c5)) )
         size_t syscommandsize = 4096;
         char* syscommand = (char*)malloc(syscommandsize);
         if(begin_name+1 < begin_offset){
             syscommandsize = syscommand ? syscommandsize : funcnamesize;
             syscommand = syscommand ? syscommand : funcname;               //use funcname if cannot alloc
             snprintf(syscommand, syscommandsize,
                      "addr2line  -i -e %s"
                      " $( printf '%%x\\n' $(( 0x0`nm %s 2>/dev/null"
                         " | grep %s"
                         " | cut -d \" \" -f 1` +%s"
                         " )) 2>/dev/null) 1>&2", symbollist[i], symbollist[i], begin_name, begin_offset);
             //printf("%s\n",syscommand);
             //fflush(stdout);
             system(syscommand);
         }
 
         // Now apply
         // __cxa_demangle():
 
         int status = 0;
         char* ret = abi::__cxa_demangle( begin_name, funcname,
                                          &funcnamesize, &status );
         char* fname = begin_name;
         if ( status == 0 ) 
            fname = ret;
 
         if ( begin_offset )
         {
            fprintf( out, "  %-30s ( %-40s  + %-6s) %s\n",
                     symbollist[i], fname, begin_offset, end_offset );
         } else {
            fprintf( out, "  %-30s ( %-40s    %-6s) %s\n",
                     symbollist[i], fname, "", end_offset );
         }
#endif  // !DARWIN - but is posix
      } else {
         // couldn't parse the line? print the whole line.
         fprintf(out, "  %-40s\n", symbollist[i]);
      }
   }
   free(symbollist);
#endif //__cplusplus
}

#ifdef __cplusplus
}
#endif
