/*
 * jetsonGPIO.h
 *
 * Copyright (c) 2015 JetsonHacks
 * www.jetsonhacks.com
 *
 * Based on Software by RidgeRun
 * Originally from:
 * https://developer.ridgerun.com/wiki/index.php/Gpio-int-test.c
 * Copyright (c) 2011, RidgeRun
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the RidgeRun.
 * 4. Neither the name of the RidgeRun nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RIDGERUN ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL RIDGERUN BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JETSONGPIO_H_
#define JETSONGPIO_H_

 /****************************************************************
 * Constants
 ****************************************************************/

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64

typedef unsigned int jetsonGPIO ;
typedef unsigned int pinDirection ;
typedef unsigned int pinValue ;

enum pinDirections {
	inputPin  = 0,
	outputPin = 1
} ;

enum pinValues {
    low = 0,
    high = 1,
    off = 0,  // synonym for things like lights
    on = 1
}  ;

enum jetsonGPIONumber {
       gpio216 = 216,      // J21 - Pin 7
       gpio50 = 50,      // J21 - Pin 11
       gpio79 = 79,      // J21 - Pin 12
       gpio14 = 14,      // J21 - Pin 13
       gpio194 = 194,      // J21 - Pin 15
       gpio232 = 232,      // J21 - Pin 16
       gpio15 = 15,      // J21 - Pin 18
       gpio16 = 16,      // J21 - Pin 19
       gpio17 = 17,      // J21 - Pin 21
       gpio13 = 13,      // J21 - Pin 22
       gpio18 = 18,      // J21 - Pin 23
       gpio19 = 19,      // J21 - Pin 24
       gpio20 = 20,      // J21 - Pin 26
       gpio149 = 149,      // J21 - Pin 29
       gpio200 = 200,      // J21 - Pin 31
       gpio168 = 168,      // J21 - Pin 32
       gpio38 = 38,      // J21 - Pin 33
       gpio76 = 76,      // J21 - Pin 35
       gpio51 = 51,      // J21 - Pin 36
       gpio12 = 12,      // J21 - Pin 37
       gpio77 = 77,      // J21 - Pin 38
       gpio78 = 78      // J21 - Pin 40
} ;

int gpioExport ( jetsonGPIO gpio ) ;
int gpioUnexport ( jetsonGPIO gpio ) ;
int gpioSetDirection ( jetsonGPIO, pinDirection out_flag ) ;
int gpioSetValue ( jetsonGPIO gpio, pinValue value ) ;
int gpioGetValue ( jetsonGPIO gpio, unsigned int *value ) ;
int gpioSetEdge ( jetsonGPIO gpio, char *edge ) ;
int gpioOpen ( jetsonGPIO gpio ) ;
int gpioClose ( int fileDescriptor ) ;
int gpioActiveLow ( jetsonGPIO gpio, unsigned int value ) ;



#endif // JETSONGPIO_H_
