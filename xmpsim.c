#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#define X_INSTRUCTIONS_NOT_NEEDED
#include "xis.h"
#include "xcpu.h"

#define TICK_ARG 1
#define IMAGE_ARG 2
#define QUANTUM_ARG 3
#define CORES_ARG 4

static int ticks;
static int quantum;

static void * run_core( void *arg ) {
  xcpu *cs = (xcpu *) arg;
  unsigned int i;

  for( i = 0; ( ticks < 1 ) || ( i < ticks ); i++ ) {
    if( i && ( quantum > 0 ) && !( i % quantum ) ) {
      if( !xcpu_exception( cs, X_E_INTR ) ) {
        fprintf( stderr, "Exception error, CPU %d has halted.\n", cs->id );
        return 0;
      } 
    }

    if( !xcpu_execute( cs ) ) {
      fprintf( stderr, "CPU %d has halted.\n", cs->id );
      return 0;
    }
  }

  fprintf( stderr, "CPU %d ran out of time.\n", cs->id );
  return NULL;
}

/* new functions*/


int main( int argc, char **argv ) {

  FILE *fp;
  struct stat fs;
  xcpu *cs;
  unsigned char *mem;
  int cores;
  unsigned short i;
  pthread_t *tid;

  if( ( argc < 5 ) || ( sscanf( argv[TICK_ARG], "%d", &ticks ) != 1 ) || 
      ( ticks < 0 ) || ( sscanf( argv[QUANTUM_ARG], "%d", &quantum ) != 1 ) ||
      ( quantum < 0 ) || ( sscanf( argv[CORES_ARG], "%d", &cores ) != 1 ) ||
      ( cores < 1 ) ) {
    fprintf( stderr, "usage: xsim <ticks> <obj file> <quantum> <cores>\n" );
    fprintf( stderr, 
            "      <ticks> is number instructions to execute (0 = forever)\n" );
    fprintf( stderr, 
            "      <image file> xis object file created by or xasxld\n" );
    fprintf( stderr, 
            "      <quantum> is the # of ticks between interrupts %s\n",
                         "(0 = no interrupts)" );
    fprintf( stderr, "      <cores> is the # of cores in the system (>= 1)\n" );
    return 1;
  } 

  tid = (pthread_t *)malloc( sizeof( pthread_t ) * cores );
  if( !tid ) {
    fprintf( stderr, "error: memory allocation (%d) failed\n", 
                    (int)sizeof( pthread_t ) * cores );
    exit( 1 );
  }

  mem = (unsigned char *)malloc( XIS_MEM_SIZE );
  if( !mem ) {
    fprintf( stderr, "error: memory allocation (%d) failed\n", XIS_MEM_SIZE );
    exit( 1 );
  }
  memset( mem, I_BAD, XIS_MEM_SIZE );

  if( stat( argv[IMAGE_ARG], &fs ) ) {
    fprintf( stderr, "error: could not stat image file %s\n", argv[IMAGE_ARG] );
    return 1;
  } else if( fs.st_size > XIS_MEM_SIZE ) {
    fprintf( stderr, "Not enough memory to run all the programs." );
    return 1;
  }

  fp = fopen( argv[IMAGE_ARG], "rb" );
  if( !fp ) {
    fprintf( stderr, "error: could not open image file %s\n", argv[IMAGE_ARG] );
    return 1;
  } else if( fread( mem, 1, fs.st_size, fp ) != fs.st_size ) {
    fprintf( stderr, "error: could not read file %s\n", argv[IMAGE_ARG] );
    return 1;
  }
  fclose( fp );

  cs = (xcpu *)malloc( sizeof( xcpu ) * cores );
  if( !cs ) {
    fprintf( stderr, "error: memory allocation (%d) failed\n", 
                   (int)sizeof( xcpu ) * cores );
    exit( 1 );
  }
  memset( cs, 0, sizeof( xcpu ) * cores );

  for( i = 0; i < cores; i++ ) {
    cs[i].memory = mem;
    cs[i].id = i;
    cs[i].num = (unsigned short)cores;
    if( pthread_create( &tid[i], NULL, run_core, &cs[i] ) ) {
      fprintf( stderr, "error: could not create thread %d\n", i );
      exit( 1 );
    }
  }

  for( i = 0; i < cores; i++ ) {
    if( pthread_join( tid[i], NULL ) ) {
      fprintf( stderr, "error: could not join thread %d\n", i );
      exit( 1 );
    }
  }

  return 0;
}
