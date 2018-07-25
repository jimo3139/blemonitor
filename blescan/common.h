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

typedef enum {
	GREEN_LED = 0,
	RED_LED = 1
} Leds;

typedef unsigned long 	UINT32;
typedef unsigned char	UCHAR;
typedef unsigned char	UINT8;
typedef unsigned short	UINT16;

#define VERSION			66
#define BUFSIZE			8096
#define ERROR			42
#define MESSAGE			44
#define HOST			200
#define WSR			101
#define FORBIDDEN		403
#define NOTFOUND		404
#define FALSE			0
#define TRUE			1
#define DISABLE			0
#define ENABLE			1
#define NORMAL			0
#define SPECIAL			1
#define WEB_GET			2
#define WEB_POST		3
#define WEB_PUT			4
#define WEB_OPTIONS		5
#define JUST_CHECK		100
#define JUST_SEND		101
// exit() status codes.
#define EXIT_OK			0
#define EXIT_ERROR		1
#define EXIT_FORBIDDEN		3
#define EXIT_NO_DIR		4
