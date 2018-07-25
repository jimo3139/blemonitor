/*************************************************************************************************
 * Copyright (c) 2012-2018 Skreens Entertainment Technologies Incorporated - http://skreens.com
 *
 *  Redistribution and use in source and binary forms, with or without  modification, are
 *  permitted provided that the following conditions are met:
 *
 *  Redistributions of source code must retain the above copyright notice, this list of
 *  conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright  notice, this list of
 *  conditions and the following disclaimer in the documentation and/or other materials
 *  provided with the distribution.
 *
 *  Neither the name of Skreens Entertainment Technologies Incorporated  nor the names of its
 *  contributors may be used to endorse or promote products derived from this software without
 *  specific prior written permission.

 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
 *  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 *  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 *  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************************************/

static int decodeCommand(char *myString, int fd);
static int macroCommand(char *myString,int fd);
static void webSerice(int fd, int hit, int port);
static int sysIrq(int flag);
static void localTimer(unsigned int time);
static int initBcm2835();
static int setupTimerInt();
static int setLedRpi(Leds led, uint8_t value);
static char *replaceOneStr(char *str, char *orig, char *rep);
static void replaceAllStr(char *target, const char *needle, const char *replacement);
int sha1B64( char *key, char *b64);
void traceLog(int type, char *s1, char *s2, int socket_fd);
int bleScanDevices();

struct
{
	char *ext;
	char *filetype;
}
extensions [] =
{
	{"gif", "image/gif" },
	{"jpg", "image/jpg" },
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"ico", "image/ico" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{"json","text/html" },
	{0,0}
};

#define maxCaptureBuffer 	8192
#define maxPulseBuffer 		2000
#define samples			8

// Map for RPI2
#define GPIO_BASE2  	0x20200000
#define TIMER_BASE2 	0x20003000
#define INT_BASE2	0x2000B000

// Map for RPI3
#define GPIO_BASE3  	0x3f200000
#define TIMER_BASE3 	0x3f003000
#define INT_BASE3	0x3f00B000

/************************************************************
* GPIO Pins 
************************************************************/
#define IRC_GPIO_P1_07          4  /*!< Version 1, Pin P1-07 */
#define IRC_GPIO_P1_11         17  /*!< Version 1, Pin P1-11 */
#define IRC_GPIO_P1_12         18  /*!< Version 1, Pin P1-12, can be PWM channel 0 in ALT FUN 5 */
#define IRC_GPIO_P1_15         22  /*!< Version 1, Pin P1-15 */
#define IRC_GPIO_P1_16         23  /*!< Version 1, Pin P1-16 */

#define PIN11_OUT 		IRC_GPIO_P1_11 	// BCM-GPIO 17 (physical pin 11), which is Pin 0 for wiringPi.
#define PIN18_OUT 		RPI_GPIO_P1_18  // BCM-GPIO 24 (physical pin 18)
#define PIN12_IN		IRC_GPIO_P1_12 	// BCM-GPIO 18 (physical pin 12)
#define PIN16_IN 		IRC_GPIO_P1_16  // BCM-GPIO 23 (physical pin 16)

struct miscControlStruct
{
	int 	modelType;
	int 	rssiValue;
};	
