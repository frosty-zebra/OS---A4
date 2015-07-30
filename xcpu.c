#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "xis.h"
#include "xcpu.h"

void xcpu_print( xcpu *c ) {
  int i;
  unsigned int op1;
  int op2;
  static pthread_mutex_t lk = PTHREAD_MUTEX_INITIALIZER;

  if( pthread_mutex_lock( &lk ) ) {
    printf( "Failure to acquire lock!" );
    abort();
  }

  
  fprintf( stdout, "%2.2d> PC: %4.4x, State: %4.4x\n" , c->id, c->pc, c->state );
  fprintf( stdout, "%2.2d> Registers: ", c->id );
  for( i = 0; i < X_MAX_REGS; i++ ) {
    if( !( i % 8 ) ) {
      fprintf( stdout, "\n%2.2d>     ", c->id );
    }
    fprintf( stdout, " r%2.2d:%4.4x", i, c->regs[i] );
  }
  fprintf( stdout, "\n" );

  op1 = c->memory[c->pc];
  op2 = c->memory[c->pc + 1];
  for( i = 0; i < I_NUM; i++ ) {
    if( x_instructions[i].code == c->memory[c->pc] ) {
      fprintf( stdout, "%2.2d> Instruction: %s ", c->id, 
               x_instructions[i].inst );
      break;
    }
  }

  switch( XIS_NUM_OPS( op1 ) ) {
  case 1:
    if( op1 & XIS_1_IMED ) {
      fprintf( stdout, "%d", op2 );
    } else {
      fprintf( stdout, "r%d", XIS_REG1( op2 ) );
    }
    break;
  case 2:
    fprintf( stdout, "r%d, r%d", XIS_REG1( op2 ), XIS_REG2( op2 ) );
    break;
  case XIS_EXTENDED:
    fprintf( stdout, "%u", (c->memory[c->pc + 2] << 8) | c->memory[c->pc + 3] );
    if( op1 & XIS_X_REG ) {
      fprintf( stdout, ", r%d", XIS_REG1( op2 ) );
    }
    break;
  }
  fprintf( stdout, "\n" );

  if( pthread_mutex_unlock( &lk ) ) {
    printf( "Failure to release lock!" );
    abort();
  }
}

#define MEM_IDX(x) ((x) % XIS_MEM_SIZE)

static void push( xcpu *c, unsigned short v ) {
  c->regs[X_STACK_REG] -= 2;
  c->memory[MEM_IDX(c->regs[X_STACK_REG])] = v >> 8;
  c->memory[MEM_IDX(c->regs[X_STACK_REG] + 1)] = v;
}

static unsigned short pop( xcpu *c ) {
  unsigned short v;
  v = ( c->memory[MEM_IDX(c->regs[X_STACK_REG])] << 8 ) |
        c->memory[MEM_IDX(c->regs[X_STACK_REG] + 1)];
  c->regs[X_STACK_REG] += 2;
  return v;
}

extern int xcpu_execute( xcpu *c ) {
  unsigned char op;
  char op2;
  int r1;
  int r2;
  unsigned short val;
  static pthread_mutex_t lk = PTHREAD_MUTEX_INITIALIZER;


  assert( c );

  op = c->memory[MEM_IDX(c->pc)];
  op2 = c->memory[MEM_IDX(c->pc + 1)];
  c->pc = MEM_IDX(c->pc + 2);
  r1 = ( op2 >> 4 ) & 0xf;
  r2 = op2 & 0xf;
 
  if( XIS_IS_EXT_OP( op ) ) { 
    val = ( c->memory[MEM_IDX(c->pc)] << 8 ) | c->memory[MEM_IDX(c->pc + 1)];
    c->pc = MEM_IDX(c->pc + 2);
  }

  switch( op ) {
  case I_PUSH:
    push( c, c->regs[r1] );
    break;
  case I_POP:
    c->regs[r1] = pop( c );
    break;
  case I_MOV:
    c->regs[r2] = c->regs[r1];
    break; 
  case I_LOAD:
    c->regs[r2] = ( c->memory[MEM_IDX(c->regs[r1])] << 8 ) |
                    c->memory[MEM_IDX(c->regs[r1] + 1)];
    break; 
  case I_STOR:
    c->memory[MEM_IDX(c->regs[r2])] = c->regs[r1] >> 8;
    c->memory[MEM_IDX(c->regs[r2] + 1)] = c->regs[r1];
    break; 
  case I_LOADB:
    c->regs[r2] = c->memory[MEM_IDX(c->regs[r1])];
    break; 
  case I_STORB:
    c->memory[MEM_IDX(c->regs[r2])] = c->regs[r1];
    break; 
  case I_LOADA:
    if( pthread_mutex_lock( &lk ) ) {
      return 0;
    }
    c->regs[r2] = ( c->memory[MEM_IDX(c->regs[r1])] << 8 ) |
                    c->memory[MEM_IDX(c->regs[r1] + 1)];
    if( pthread_mutex_unlock( &lk ) ) {
      return 0;
    }
    break;
  case I_TNSET:
    if( pthread_mutex_lock( &lk ) ) {
      return 0;
    }
    c->regs[r2] = ( c->memory[MEM_IDX(c->regs[r1])] << 8 ) |
                    c->memory[MEM_IDX(c->regs[r1] + 1)];
    c->memory[MEM_IDX(c->regs[r1])] = 0;
    c->memory[MEM_IDX(c->regs[r1] + 1)] = 1;
    if( pthread_mutex_unlock( &lk ) ) {
      return 0;
    }
    break;
  case I_STORA:
    if( pthread_mutex_lock( &lk ) ) {
      return 0;
    }
    c->memory[MEM_IDX(c->regs[r2])] = c->regs[r1] >> 8;
    c->memory[MEM_IDX(c->regs[r2] + 1)] = c->regs[r1];
    if( pthread_mutex_unlock( &lk ) ) {
      return 0;
    }
    break;
  case I_JMPR:
    val = c->regs[r1];
  case I_JMP:
    c->pc = val;
    break; 
  case I_CALLR:
    val = c->regs[r1];
  case I_CALL:
    push( c, c->pc );
    c->pc = val;
    break; 
  case I_RET:
    c->pc = pop( c );
    break; 
  case I_LOADI:
    c->regs[r1] = val;
    break; 
  case I_ADD:
    c->regs[r2] += c->regs[r1];
    break; 
  case I_SUB:
    c->regs[r2] -= c->regs[r1];
    break; 
  case I_MUL:
    c->regs[r2] *= c->regs[r1];
    break; 
  case I_DIV:
    c->regs[r2] /= c->regs[r1];
    break; 
  case I_NEG:
    c->regs[r1] = -c->regs[r1];
    break; 
  case I_AND:
    c->regs[r2] &= c->regs[r1];
    break; 
  case I_OR:
    c->regs[r2] |= c->regs[r1];
    break; 
  case I_XOR:
    c->regs[r2] ^= c->regs[r1];
    break; 
  case I_NOT:
    c->regs[r1] = !c->regs[r1];
    break; 
  case I_INC:
    c->regs[r1]++;
    break; 
  case I_DEC:
    c->regs[r1]--;
    break; 
  case I_SHL:
    c->regs[r2] <<= c->regs[r1];
    break; 
  case I_SHR:
    c->regs[r2] >>= c->regs[r1];
    break; 
  case I_TEST:
    if( c->regs[r2] & c->regs[r1] ) {
      c->state |= X_STATE_COND_FLAG;
    } else {
      c->state &= ~X_STATE_COND_FLAG;
    }
    break; 
  case I_CMP:
    if( c->regs[r1] < c->regs[r2] ) {
      c->state |= X_STATE_COND_FLAG;
    } else {
      c->state &= ~X_STATE_COND_FLAG;
    }
    break; 
  case I_EQU:
    if( c->regs[r1] == c->regs[r2] ) {
      c->state |= X_STATE_COND_FLAG;
    } else {
      c->state &= ~X_STATE_COND_FLAG;
    }
    break; 
  case I_BR:
    if( c->state & X_STATE_COND_FLAG ) {
      c->pc += op2 - 2;
    }
    break; 
  case I_JR: 
    c->pc += op2 - 2;
    break; 
  case I_CLD:
    c->state &= ~X_STATE_DEBUG_ON;
    break; 
  case I_STD:
    c->state |= X_STATE_DEBUG_ON;
    break; 
  case I_OUT:
    putchar( c->regs[r1] );
    break; 
  case I_CLI:
    c->state &= ~X_STATE_IN_EXCEPTION;
    break; 
  case I_STI:
    c->state |= X_STATE_IN_EXCEPTION;
    break; 
  case I_IRET:
    c->pc = pop( c );
    c->state = pop( c );
    break; 
  case I_TRAP:
    xcpu_exception( c, X_E_TRAP );
    break; 
  case I_LIT:
    c->itr = c->regs[r1];
    break; 
  case I_CPUID:
    c->regs[r1] = c->id;
    break;
  case I_CPUNUM:
    c->regs[r1] = c->num;
    break;

  /*new instructions*/
  
  /*synchronous ouput*/
  /*outpa S, D*/
  case I_OUTP:

    if( xdev_outp_sync( c -> regs[r2], c -> regs[r1] ) )
	c -> state |= X_STATE_COND_FLAG;
    else
	c -> state &= ~X_STATE_COND_FLAG; 
    break;

  /*asynchronous output*/
  case I_OUTPA:

  if( xdev_outp_async( c -> regs[r2], c -> regs[r1] ) )
	c -> state |= X_STATE_COND_FLAG;
    else
	c -> state &= ~X_STATE_COND_FLAG;
  break;

  /*synchronous input*/

  case I_INP:
  
  if( xdev_inp_sync( c -> regs[r1], c -> regs[r2] ) )
	c -> state |= X_STATE_COND_FLAG;
    else
	c -> state &= ~X_STATE_COND_FLAG;
  break;

  /*asynchronous input*/
  case I_INPA:
  
  if( xdev_inp_async( c -> regs[r1], c -> regs[r2] ) )
	c -> state |= X_STATE_COND_FLAG;
    else
	c -> state &= ~X_STATE_COND_FLAG;
  break;

  case I_BAD:
  default:
    return 0;
  }

  if( c->state & X_STATE_DEBUG_ON ) {
    xcpu_print( c );
  }
  return 1;
}



int xcpu_exception( xcpu *c, unsigned int ex ) {
  unsigned short adr;

  if( c->state & X_STATE_IN_EXCEPTION ) {
    return 1;
  } else if( c->itr && ( ex < X_E_LAST ) ) {
    push( c, c->state );
    push( c, c->pc );
    c->state |= X_STATE_IN_EXCEPTION;
    adr = c->itr + ( sizeof( unsigned short ) * ex );
    c->pc = ( c->memory[MEM_IDX(adr)] << 8 ) | c->memory[MEM_IDX(adr + 1)];
    return 1;
  }
  return 0;
}

