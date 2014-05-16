/*
 * Wiegand API Raspberry Pi
 * Based on previous code by Daniel Smith (www.pagemac.com) and Ben Kent (www.pidoorman.com)
 * and, most recently, Kyle Mallory (irishjesus.wordpress.com)
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <time.h>

#define TIMEOUT 3000000

static int32_t bitBag;
static int bitIndex;
static struct timespec lastBitTime;        // timestamp of the last bit received (used for timeouts)


void data0Pulse() {
    // The bit at bitIndex is already zero
    bitIndex++;
    clock_gettime(CLOCK_MONOTONIC, &lastBitTime);
}

void data1Pulse() {
    // Set the bit at bitIndex (starting from the left) to 1
    bitBag |= 1 << (31 - bitIndex);
    bitIndex++;
    clock_gettime(CLOCK_MONOTONIC, &lastBitTime);
}

// Return true if we have some bits and it's been more than TIMEOUT ns
// since we've recieved anything
int finishedReadingBits() {
    struct timespec now, delta;
    clock_gettime(CLOCK_MONOTONIC, &now);
    delta.tv_nsec = now.tv_nsec - lastBitTime.tv_nsec;

    if (delta.tv_nsec > TIMEOUT && bitIndex) {
        return 1;
    } else {
        return 0;
    }
}

void reset() {
    bitBag = 0;
    bitIndex = 0;
}

int init() {
    reset();

    system("gpio edge 2 falling");
    system("gpio edge 3 falling");

    wiringPiSetupSys();

    wiringPiISR(2, INT_EDGE_FALLING, data0Pulse);
    wiringPiISR(3, INT_EDGE_FALLING, data1Pulse);
}

void main(void) {
    init();

    while(1) {
        if (finishedReadingBits()) {
            int32_t swapped = __builtin_bswap32(bitBag);
            printf("%08x\n", swapped);
            reset();
        } else {
            usleep(5000);
        }
    }
}
