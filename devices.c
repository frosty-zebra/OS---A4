#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
 
#include "xdev.h"
#include "devices.h"

#define DISP_PORT 1
#define KBD_PORT 2
#define RAND_PORT 3


/* title: display device
 * param: void *arg (ignored)
 * function: invoked on a separate thread and peforms the following operations:
 *           - associates device with port 1 
 *           - enters an infinite loop
 *           - the loop 
 *             - gets a charatcer (lower half of a word) from the port
 *             - outputs the character to stdout
 * returns: void *, never returns.
 */
extern void *device_display( void *arg ) {

  /* your code here */

  return NULL;
}


/* title: keyboard device
 * param: void *arg (ignored)
 * function: invoked on a separate thread and peforms the following operations:
 *           - associates device with port 2 
 *           - enters an infinite loop
 *           - the loop 
 *             - gets a character from stdin
 *             - puts the character (lower half of a word) to the port
 * returns: void *, never returns.
 */
extern void *device_keyboard( void *arg ) {

  /* your code here */
  /*psuedo*/

  if( !xdev_associate_port(KBD_PORT) )
	printf( "xdev_associate_port(KBD_PORT) failed );

  while(1){
	if( !xdev_dev_get(KBD_PORT) ){
		
	}
  }



  return NULL;
}


/* title: random device
 * param: void *arg (ignored)
 * function: invoked on a separate thread and peforms the following operations:
 *           - associates device with port 3 
 *           - enters an infinite loop
 *           - the loop 
 *             - generates a random word using random()
 *             - puts the word to the port
 * returns: void *, never returns.
 */
extern void *device_random( void *arg ) {

  /* your code here */

  return NULL;
}
