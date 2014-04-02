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

#define D0_PIN 0
#define D1_PIN 1

#define MAX_BITS 100                 // max number of bits
#define WIEGAND_WAIT_TIME 300000      // time to wait for another wiegand pulse.

static unsigned char databits[MAX_BITS];    // stores all of the data bits
static unsigned char bitCount;              // number of bits currently captured
static unsigned int flagDone;              // goes low when data is currently being captured
static unsigned int wiegand_counter;        // countdown until we assume there are no more bits

static unsigned long facilityCode=0;        // decoded facility code
static unsigned long cardCode=0;            // decoded card code


PI_THREAD (waitForData0)
{

  (void)piHiPri (10) ;	// Set this thread to be high priority

  for (;;)
  {
    if (waitForInterrupt (D0_PIN, -1) > 0)	// Got it
    {
		//printf ("0") ; fflush (stdout) ;
		bitCount++;
		flagDone = 0;
		wiegand_counter = WIEGAND_WAIT_TIME;
    }

  }
}

PI_THREAD (waitForData1)
{

  (void)piHiPri (10) ;	// Set this thread to be high priority

  for (;;)
  {
	if (waitForInterrupt (D1_PIN, -1) > 0)	// Got it
    {
		//printf ("1") ; fflush (stdout) ;
		databits[bitCount] = 1;
		bitCount++;
		flagDone = 0;
		wiegand_counter = WIEGAND_WAIT_TIME;
    }
  }
}

void setup(void)
{
  system ("gpio edge 0 falling") ;
  system ("gpio edge 1 falling") ;

  // Setup wiringPi

  wiringPiSetupSys () ;

// Fire off our interrupt handler

  piThreadCreate (waitForData0) ;
  piThreadCreate (waitForData1) ;

  wiegand_counter = WIEGAND_WAIT_TIME;
}

void printBits()
{
      // Prints out the results
      printf ("Read %d bits\n", bitCount) ; fflush (stdout) ;
	  printf ("Facility Code: %d\n", facilityCode) ; fflush (stdout) ;
      printf ("TagID: %d\n\n", cardCode) ; fflush (stdout) ;
	  return;
}

int main(void)
{

setup();

unsigned char i;

char useSiteCode = '\0'; // determines whether or no to use 26 bit site code.


int validInput;

printf ("\nWiegand test v0.1\nWritten for Raspberry Pi\nBy Ben Kent\n11/08/2012\n\n") ; fflush (stdout) ;

validInput = 0;
	while( validInput == 0 ) {
		printf("Use standard site code for 26bit numbers (Y/N)?\n");
		scanf("  %c", &useSiteCode );
		useSiteCode = toupper( useSiteCode );
		if((useSiteCode == 'Y') || (useSiteCode == 'N'))
		validInput = 1;
		else  printf("\nError: Invalid choice\n");
	}

 //printf("\n%c was your choice.\n", useSiteCode ); fflush (stdout) ;
 printf("\nReady.\nPresent card:\n\n"); fflush (stdout) ;


 for (;;)
  {
  // This waits to make sure that there have been no more data pulses before processing data
  if (!flagDone) {
    if (--wiegand_counter == 0)
	    flagDone = 1;
  }

  // if we have bits and the wiegand counter reached 0
  if (bitCount > 0 && flagDone) {
    //unsigned char i;

	//Full wiegand 26 bit
	if (bitCount == 26 & useSiteCode == 'N')
    {
	//unsigned char i;

      // card code = bits 2 to 25
      for (i=1; i<25; i++)
      {
         cardCode <<=1;
         cardCode |= databits[i];
      }

      printBits();
    }

	else if (bitCount == 26 & useSiteCode == 'Y')
    {
      // standard 26 bit format with site code
      // facility code = bits 2 to 9
      for (i=1; i<9; i++)
      {
         facilityCode <<=1;
         facilityCode |= databits[i];
      }

      // card code = bits 10 to 23
      for (i=9; i<25; i++)
      {
         cardCode <<=1;
         cardCode |= databits[i];
      }

      printBits();
    }

  // 35 bit HID Corporate 1000 format
  else if (bitCount == 35)
    {

      // facility code = bits 2 to 14
      for (i=2; i<14; i++)
      {
         facilityCode <<=1;
         facilityCode |= databits[i];
      }

      // card code = bits 15 to 34
      for (i=14; i<34; i++)
      {
         cardCode <<=1;
         cardCode |= databits[i];
      }

      printBits();
    }
	//HID Proprietary 37 Bit Format with Facility Code: H10304
	else if (bitCount == 37)
    {

      // facility code = bits 2 to 17
      for (i=2; i<17; i++)
      {
         facilityCode <<=1;
         facilityCode |= databits[i];
      }

      // card code = bits 18 to 36
      for (i=18; i<36; i++)
      {
         cardCode <<=1;
         cardCode |= databits[i];
      }

      printBits();
    }
	else {
      // Other formats to be added later.
     printf ("Unknown format\n") ; fflush (stdout) ;
	 //printf ("Wiegand Counter = %d\n", wiegand_counter) ; fflush (stdout) ;
	 //printf ("Flag done = %d\n", flagDone) ; fflush (stdout) ;
    }
	 // cleanup and get ready for the next card
     bitCount = 0;
     facilityCode = 0;
     cardCode = 0;
     for (i=0; i<MAX_BITS; i++)
     {
       databits[i] = 0;
     }
	}


}

  return 0 ;

}
