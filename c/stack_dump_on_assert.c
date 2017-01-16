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
         // offset in [begin_offset, end_offset). now apply
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
/*//Stuff for using libbfd to look up symbols
#include <bfd.h>    //c++ lookup file
#include <dlfcn.h>  //c++ lookup file
#include <link.h>   //c++ lookup file

#include <string.h>
#include <cassert>
class FileLineDesc
{
public:
   FileLineDesc( asymbol** syms, bfd_vma pc ) : mPc( pc ), mFound( false ), mSyms( syms ) {}

   void findAddressInSection( bfd* abfd, asection* section );

   bfd_vma      mPc;
   char*        mFilename;
   char*        mFunctionname;
   unsigned int mLine;
   int          mFound;
   asymbol**    mSyms;
};

void FileLineDesc::findAddressInSection( bfd* abfd, asection* section )
{
   if ( mFound )
      return;

   if (( bfd_get_section_flags( abfd, section ) & SEC_ALLOC ) == 0 )
      return;

   bfd_vma vma = bfd_get_section_vma( abfd, section );
   if ( mPc < vma )
      return;

   bfd_size_type size = bfd_section_size( abfd, section );
   if ( mPc >= ( vma + size ))
      return;

   mFound = bfd_find_nearest_line( abfd, section, mSyms, ( mPc - vma ),
                                   (const char**)&mFilename, (const char**)&mFunctionname, &mLine );
}

static void findAddressInSection( bfd* abfd, asection* section, void* data )
{
   FileLineDesc* desc = (FileLineDesc*)data;
   assert( desc );
   return desc->findAddressInSection( abfd, section );
}

static char** translateAddressesBuf( bfd* abfd, bfd_vma* addr, int numAddr, asymbol** syms )
{
   char** ret_buf = NULL;
   int    total   = 0;

   char   b;
   char*  buf     = &b;
   int    len     = 0;

   for ( unsigned int state = 0; state < 2; state++ )
   {
      if ( state == 1 )
      {
         ret_buf = (char**)malloc( total + ( sizeof(char*) * numAddr ));
         buf = (char*)(ret_buf + numAddr);
         len = total;
      }

      for ( int i = 0; i < numAddr; i++ )
      {
         FileLineDesc desc( syms, addr[i] );

         if ( state == 1 )
            ret_buf[i] = buf;

         bfd_map_over_sections( abfd, findAddressInSection, (void*)&desc );

         if ( !desc.mFound )
         {
            total += snprintf( buf, len, "[0x%llx] \?\? \?\?:0", (long long unsigned int) addr[i] ) + 1;

         } else {

            const char* name = desc.mFunctionname;
            if ( name == NULL || *name == '\0' )
               name = "??";
            if ( desc.mFilename != NULL )
            {
               char* h = strrchr( desc.mFilename, '/' );
               if ( h != NULL )
                  desc.mFilename = h + 1;
            }
            total += snprintf( buf, len, "%s:%u %s", desc.mFilename ? desc.mFilename : "??", desc.mLine, name ) + 1;
            // elog << "\"" << buf << "\"\n";
         }
      }

      if ( state == 1 )
      {
         buf = buf + total + 1;
      }
   }

   return ret_buf;
}

static asymbol** kstSlurpSymtab( bfd* abfd, const char* fileName )
{
  if ( !( bfd_get_file_flags( abfd ) & HAS_SYMS ))
  {
     printf( "Error bfd file \"%s\" flagged as having no symbols.\n", fileName );
     return NULL;
  }

  asymbol** syms;
  unsigned int size;

  long symcount = bfd_read_minisymbols( abfd, false, (void**)&syms, &size );
  if ( symcount == 0 )
       symcount = bfd_read_minisymbols( abfd, true,  (void**)&syms, &size );

  if ( symcount < 0 )
  {
     printf( "Error bfd file \"%s\", found no symbols.\n", fileName );
     return NULL;
  }

  return syms;
}

static char** processFile( const char* fileName, bfd_vma* addr, int naddr )
{
  bfd* abfd = bfd_openr( fileName, NULL );
  if ( !abfd )
  {
     printf( "Error opening bfd file \"%s\"\n", fileName );
     return NULL;
  }

  if ( bfd_check_format( abfd, bfd_archive ) )
  {
     printf( "Cannot get addresses from archive \"%s\"\n", fileName );
     bfd_close( abfd );
     return NULL;
  }

  char** matching;
  if ( !bfd_check_format_matches( abfd, bfd_object, &matching ))
  {
     printf( "Format does not match for archive \"%s\"\n", fileName );
     bfd_close( abfd );
     return NULL;
  }

  asymbol** syms = kstSlurpSymtab( abfd, fileName );
  if ( !syms )
  {
     printf( "Failed to read symbol table for archive \"%s\"\n", fileName );
     bfd_close( abfd );
     return NULL;
  }

  char** retBuf = translateAddressesBuf( abfd, addr, naddr, syms );

  free( syms );

  bfd_close( abfd );
  return retBuf;
}

class FileMatch
{
public:
  FileMatch( void* addr ) : mAddress( addr ), mFile( NULL ), mBase( NULL ) {}

  void*       mAddress;
  const char* mFile;
  void*       mBase;
};

static int findMatchingFile( struct dl_phdr_info* info, size_t size, void* data )
{
  FileMatch* match = (FileMatch*)data;

  for ( unsigned int i = 0; i < info->dlpi_phnum; i++ )
  {
     const ElfW(Phdr)& phdr = info->dlpi_phdr[i];

     if ( phdr.p_type == PT_LOAD )
     {
        ElfW(Addr) vaddr = phdr.p_vaddr + info->dlpi_addr;
        ElfW(Addr) maddr = ElfW(Addr)(match->mAddress);
        if (( maddr >= vaddr ) &&
            ( maddr < vaddr + phdr.p_memsz ))
        {
           match->mFile =        info->dlpi_name;
           match->mBase = (void*)info->dlpi_addr;
           return 1;
        }
     }
  }
  return 0;
}

static char** backtraceSymbols( void* const* addrList, int numAddr )
{
  char*** locations = (char***) alloca( sizeof( char** ) * numAddr );

  // initialize the bfd library
  bfd_init();

  int total = 0;
  unsigned int idx = numAddr;
  for ( int i = 0; i < numAddr; i++ )
  {
     // find which executable, or library the symbol is from
     FileMatch match( addrList[--idx] );
     dl_iterate_phdr( findMatchingFile, &match );

     // adjust the address in the global space of your binary to an
     // offset in the relevant library
     bfd_vma addr  = (bfd_vma)( addrList[idx] );
             addr -= (bfd_vma)( match.mBase );

     // lookup the symbol
     if ( match.mFile && strlen( match.mFile ))
        locations[idx] = processFile( match.mFile,      &addr, 1 );
     else
        locations[idx] = processFile( "/proc/self/exe", &addr, 1 );

     total += strlen( locations[idx][0] ) + 1;
  }

  // return all the file and line information for each address
  char** final = (char**)malloc( total + ( numAddr * sizeof( char* )));
  char* f_strings = (char*)( final + numAddr );

  for ( int i = 0; i < numAddr; i++ )
  {
     strcpy( f_strings, locations[i][0] );
     free( locations[i] );
     final[i] = f_strings;
     f_strings += strlen( f_strings ) + 1;
  }

  return final;
}*/

#ifdef __cplusplus
}
#endif
