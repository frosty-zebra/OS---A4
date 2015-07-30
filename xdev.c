#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "xdev.h"

int xdev_associate_port( unsigned short port ) {
  return 0; // replace with your code
}
  

int xdev_dev_put( unsigned short data, unsigned short port ) {
  return 0; // replace with your code
}


int xdev_dev_get( unsigned short port, unsigned short *data ) {
  return 0; // replace with your code
}


int xdev_outp_sync( unsigned short data, unsigned short port ) {
  return 0; // replace with your code
}


int xdev_outp_async( unsigned short data, unsigned short port ) {
  return 0; // replace with your code
}


int xdev_inp_sync( unsigned short port, unsigned short *data ) {
  return 0; // replace with your code
}


int xdev_inp_poll( unsigned short port, unsigned short *data ) {
  return 0; // replace with your code
}


