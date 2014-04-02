/*
 * Wiegand test v0.1
 * Written for Raspberry Pi
 * By Ben Kent
 * 11/08/2012
 * Based on an Arduino sketch by Daniel Smith: www.pagemac.com
 * Depends on the wiringPi library by Gordon Henterson: https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 * The Wiegand interface has two data lines, DATA0 and DATA1.  These lines are normall held
 * high at 5V.  When a 0 is sent, DATA0 drops to 0V for a few us.  When a 1 is sent, DATA1 drops
 * to 0V for a few us. There are a few ms between the pulses.
 *   *************
 *   * IMPORTANT *
 *   *************
 *   The Raspberry Pi GPIO pins are 3.3V, NOT 5V. Please take appropriate precautions to bring the
 *   5V Data 0 and Data 1 voltges down. I used a 330 ohm resistor and 3V3 Zenner diode for each
 *   connection. FAILURE TO DO THIS WILL PROBABLY BLOW UP THE RASPBERRY PI!
 *
 * The wiegand reader should be powered from a separate 12V supply. Connect the green wire (DATA0)
 * to the Raspberry Pi GPIO 0(SDA) pin, and the white wire (DATA1) to GPIO 1 (SCL).
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

#define D0_PIN 2
#define D1_PIN 3

#define MAX_BITS 100                 // max number of bits
#define WIEGAND_WAIT_TIME 300000      // time to wait for another wiegand pulse.

static unsigned char databits[MAX_BITS];    // stores all of the data bits
static unsigned char bitCount;              // number of bits currently captured
static unsigned int flagDone;              // goes low when data is currently being captured
static unsigned int wiegand_counter;        // countdown until we assume there are no more bits

PI_THREAD (waitForData0)
{

  (void)piHiPri (10) ;  // Set this thread to be high priority

  for (;;)
  {
    if (waitForInterrupt (D0_PIN, -1) > 0)  // Got it
    {
//    printf ("0") ; fflush (stdout) ;
    bitCount++;
    flagDone = 0;
    wiegand_counter = WIEGAND_WAIT_TIME;
    }

  }
}

PI_THREAD (waitForData1)
{

  (void)piHiPri (10) ;  // Set this thread to be high priority

  for (;;)
  {
  if (waitForInterrupt (D1_PIN, -1) > 0)  // Got it
    {
//    printf ("1") ; fflush (stdout) ;
    databits[bitCount] = 1;
    bitCount++;
    flagDone = 0;
    wiegand_counter = WIEGAND_WAIT_TIME;
    }
  }
}

void setup(void)
{
  system ("gpio edge 2 falling") ;
  system ("gpio edge 3 falling") ;

  // Setup wiringPi

  wiringPiSetupSys () ;

  // Fire off our interrupt handler
  piThreadCreate (waitForData0) ;
  piThreadCreate (waitForData1) ;

  wiegand_counter = WIEGAND_WAIT_TIME;

  bitCount = 0;

}


int main(void)
{

  setup();

  unsigned char i;

  char useSiteCode = '\0'; // determines whether or no to use 26 bit site code.


  int validInput;
  bitCount = 0;
  int32_t uid = 0;

  for (;;)
  {
    // This waits to make sure that there have been no more data pulses before processing data
    if (!flagDone) {
      if (--wiegand_counter == 0)
        flagDone = 1;
    }

    // if we have bits and the wiegand counter reached 0
    if (bitCount > 0 && flagDone)
    {
      //unsigned char i;
      if (bitCount == 32 ) { //MiFare
        for  (i=0; i<bitCount; ++i) {
          uid = (uid << 1) + ( databits[i] );
        }
        uid =  __builtin_bswap32 (uid);
        printf( "UID (NFCID1): %08x\n", uid);
        exit(0);
      }

      // cleanup and get ready for the next card
      uid = 0;
      bitCount = 0;
      for (i=0; i<MAX_BITS; i++)
      {
        databits[i] = 0;
      }
    }
  }

  return 0 ;

}
