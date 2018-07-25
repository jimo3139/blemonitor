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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <time.h>
#include <sys/mman.h>
#include <dirent.h> 
#include <pthread.h>
#include <bcm2835.h>

#include "common.h"
#include "blescan.h"
#include "jsmn/jsmn.h"

int sha1B64( char *key, char *B64);

static int	dbgMessages =FALSE; 
	

int *sharedInt;
// sharedInt[0] = RSSI value.
// sharedInt[1] = 0 = Record entries only if they have a name. 1 = Record all entries. 

char *sharedStr;

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}


volatile unsigned 		*timerValue;
volatile unsigned 		*interruptBase;

struct miscControlStruct 	miscPtr;
struct miscControlStruct* 	misc = &miscPtr;


typedef struct vector {
    double x;
    double y;
    double z;
} vector_t;

typedef struct all_vectors {
    vector_t *gyro;
    vector_t *accel;
    vector_t *magnet;
} vectors_t;

vectors_t *vectors;

	pthread_t th1;
	pthread_mutex_t mutex;



void readBleDatas(void *vectors)
{   
	bleScanDevices();
	// Should never het here unless bleScanDevices() fails.
	pthread_exit(NULL);
}

void traceLog(int type, char *s1, char *s2, int socket_fd)
{
	int fd ;
	char logbuffer[BUFSIZE*2];

	switch (type)
	{
		case ERROR: (void)sprintf(logbuffer,"ERROR    : %s%s Errno=%d exiting pid=%d",s1, s2, errno,getpid());
			    break;
		case FORBIDDEN:
			(void)write(socket_fd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n",271);
			(void)sprintf(logbuffer,"FORBIDDEN: %s%s",s1, s2);
			break;
		case NOTFOUND:
			(void)write(socket_fd, "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);

			(void)sprintf(logbuffer,"NOT FOUND: %s%s",s1, s2);
			break;
		case HOST:
			(void)write(socket_fd, "HTTP/1.1 200 Successful \nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>200 Successful</title>\n</head><body>\n<h1>Successful</h1>\nThe requested command completed successfully.\n</body></html>\n",226);


			(void)sprintf(logbuffer,"HOST Message: %s%s",s1, s2);
			break;
		case MESSAGE: {
			if(dbgMessages == TRUE) { 
				(void)sprintf(logbuffer,"MESSAGE  : %s%s\r\n",s1, s2); 
				break;
			}
			else {
				return;
			}	
		}
	}
	/* No checks here, nothing can be done with a failure anyway */
	if((fd = open("blescan.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0)
	{
		(void)write(fd,logbuffer,strlen(logbuffer));

		(void)write(fd,"\n",1);

		(void)close(fd);
	} 
        if(type == ERROR || type == NOTFOUND || type == FORBIDDEN) exit(EXIT_FORBIDDEN);
}

/* this is a child web server process, so we can exit on errors */
static void webService(int fd, int hit, int port)
{
	jsmntok_t 	tokens[256];
	jsmn_parser 	parser;
	char		*sEnd;
	long		tag;
	int 		r;
	int		value;
	char		str[3];
	int		j, file_fd, buflen;
	long		i, ret, len, length;
	char		*fstr;
	char		jsonModel[100];
	char		myString[1000];
	char		*messSegment;
	char		*keySegment;
	int		results;
	int     	requestType = NORMAL;
	char		str33[3]="abc"; 
	char		*temp;
	char		addr[100];
	char		name[100];
	unsigned char 	*keyCode;
	unsigned char 	B64[100];
	unsigned char 	frmHdr[100];
	int		webSocketCmd = FALSE;
	char		*magicNum = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	static char 	rxBuffer[BUFSIZE+1]; 	/* static so zero filled */
	static char 	cmdBuffer[100]; 	/* static so zero filled */

	ret =read(fd,rxBuffer,BUFSIZE);   /* read Web request in one go */

	replaceAllStr(rxBuffer, "%20", " ");
	replaceAllStr(rxBuffer, "%22", "\"");
  	temp = strtok(rxBuffer, "}");
	temp = strcat(temp,"}");

	traceLog(MESSAGE,"MSG666: Read RX buffer -> ",temp,fd);
	if(strstr(temp,"Upgrade: websocket")) {
		webSocketCmd = TRUE;
		keySegment = strstr(temp, "Sec-WebSocket-Key: ");

		keyCode = strtok (keySegment," \n\r");
		keyCode = strtok (NULL, " \n\r");
		traceLog(MESSAGE,"MSG300: Key:",keyCode,fd);	

		strcat(keyCode,magicNum); 
		traceLog(MESSAGE,"MSG555: Key:",keyCode,fd);	

		results = sha1B64(keyCode,B64);
		traceLog(MESSAGE,"MSG553: accept:",B64,fd);	
	}

	if(ret == 0 || ret == -1)
	{
		/* read failure stop now */
		traceLog(FORBIDDEN,"ERR004: failed to read browser request","",fd);
	}

	/* return code is valid chars */
	if(ret > 0 && ret < BUFSIZE) rxBuffer[ret]=0;    /* terminate the rxBuffer */
	else rxBuffer[0]=0;

	if((strncmp(rxBuffer,"POST ",5) == FALSE))
	{
		requestType = WEB_POST;

		strcpy(myString,"Missing packet ????");
		traceLog(MESSAGE,"MSG511: standard Message= ",rxBuffer,fd);
		messSegment = strstr(rxBuffer, "{");
		traceLog(MESSAGE,"MSG512: standard Message= ",messSegment,fd);
		buflen=strlen(messSegment);

		if(buflen > 1000)
		{
			traceLog(MESSAGE,"ERR432: PASS message length exceeded. Message = ",messSegment,fd);
		}
		else
		{
			traceLog(MESSAGE,"MSG566: Buffer length OK","",fd);

			strcpy(myString, messSegment);
			traceLog(MESSAGE,"MSG501: Command Message = ",myString,fd);

			jsmn_init(&parser);
			r = jsmn_parse(&parser, myString, strlen(myString), tokens, 256);
			if (r < 0) {
				if(r == -1) { 
					traceLog(MESSAGE,"ERR300: jsmn_parse() Not enough tokens were provided = ",myString,fd);
				}
				else if(r == -2) {
					traceLog(MESSAGE,"ERR301: jsmn_parse() Invalid character inside JSON string = ",myString,fd);
				}
				else if(r == -3) {
					traceLog(MESSAGE,"ERR302: jsmn_parse() The string is not a full JSON packet, more bytes expected = ",myString,fd);
				}
				traceLog(MESSAGE,"ERR772: Failed to parse JSON. str = ",myString,fd);
			}
			else {
				traceLog(MESSAGE,"MSG774: JSON parse Complete = ",myString,fd);
			}

			sprintf(str, "%d", r);
			traceLog(MESSAGE,"MSG778: JSON element count in string = ",str,fd);

			for (i = 1; i < r; i++) {

				if (jsoneq(myString, &tokens[i], "model") == 0) {
					for(j = 0; j < 100; j++) jsonModel[j]=0;
					strncpy(&jsonModel[0], myString + tokens[i+1].start, (tokens[i+1].end-tokens[i+1].start));
					traceLog(MESSAGE,"MSG799: myString model json structure = ",myString,fd);
					traceLog(MESSAGE,"MSG800: JSON model token = ",jsonModel,fd);

					i++;
				} 

			}

			bcm2835_gpio_write(PIN18_OUT, LOW); // LED on.
			setLedRpi(GREEN_LED, HIGH);  // Onboard green LED ON
			setLedRpi(RED_LED, LOW); // Onboard red LED OFF

			if(strstr(myString, "macro")) {
				if((results = macroCommand(myString,fd) == FALSE)) {
					traceLog(MESSAGE,"ERR019: macroCommand() command has failed.","",fd);
				}
			}
			else {
				if((results = decodeCommand(myString,fd) == FALSE)) {
					traceLog(MESSAGE,"ERR009: decodeCommand() command has failed.","",fd);
				}
			}

			bcm2835_gpio_write(PIN18_OUT, HIGH); // LED off.			
			setLedRpi(GREEN_LED, LOW);  // Onboard green LED OFF
			setLedRpi(RED_LED, HIGH); // Onboard red LED ON

		}
	}
	else if((strncmp(rxBuffer, "GET ",4) == FALSE))
	{
		requestType = WEB_GET;
	}
	else if((strncmp(rxBuffer,"OPTIONS ",8) == FALSE))
	{
		requestType = WEB_OPTIONS;
	}
	else traceLog(MESSAGE,"ERR001: Web page did not find GET/PUT/POST message -> ",rxBuffer,fd);

	/******************************************************************************
	* Send a response back to the client.
	******************************************************************************/

	// If we are executing a GET, we need to open a file to get content.
	if(requestType == WEB_POST)	{
		traceLog(MESSAGE,"MSG394: Web Post data processing response ","200 Successful!",fd);	
		requestType = NORMAL;

		(void)write(fd, "HTTP/1.1 200 Successful \nContent-Length: 136\nConnection: close\nContent-Type: \
				text/html\n\n<html><head>\n<title>200 Successful</title>\n</head><body>\n<h1>Successful</h1>\n \
				blesc Server requested command completed successfully.\n</body></html>\n",244);
		usleep(500000); // 1/2 second, allow socket to drain before signaling the socket is closed */
		close(fd);

	}
	else if(requestType == WEB_OPTIONS)	{
		traceLog(MESSAGE,"MSG374: Web OPTIONS data processing response ","200 Successful!",fd);	
		requestType = NORMAL;

		(void)write(fd, "HTTP/1.1 200 Successful \nAccess-Control-Allow-Origin: *\nContent-Length: 136\nConnection: close\nContent-Type: \
				text/html\n\n<html><head>\n<title>200 Successful</title>\n</head><body>\n<h1>Successful</h1>\n \
				blesc Server requested command completed successfully.\n</body></html>\n",275);
		usleep(500000); // 1/2 second, allow socket to drain before signaling the socket is closed */
		close(fd);

	}
	else if(requestType == WEB_GET)	{
		traceLog(MESSAGE,"MSG000: Data:",rxBuffer,fd);	

		requestType = NORMAL;
		/* null terminate after the second space to ignore extra stuff */
		for(i=4;i<BUFSIZE;i++)
		{ 
			if(rxBuffer[i] == ' ') /* string is "GET URL " +lots of other stuff */
			{
				rxBuffer[i] = 0;
				break;
			}
		}

		traceLog(MESSAGE,"MSG111: Data:",rxBuffer,fd);	

		/* check for illegal parent directory use .. */
		for(j=0;j<i-1;j++)	{
			if(rxBuffer[j] == '.' && rxBuffer[j+1] == '.')
			{
				traceLog(FORBIDDEN,"ERR005: Parent directory (..) path names not supported",rxBuffer,fd);
			}
		}

		traceLog(MESSAGE,"MSG222: Data:",rxBuffer,fd);	

		// If the GET is empty, set the path based on the port number being requested. */
		if( !strncmp(&rxBuffer[0],"GET /\0",6) || !strncmp(&rxBuffer[0],"get /\0",6) ) 
		{
			switch(port)
			{
				case 80: (void)strcpy(rxBuffer,"GET //home/blescan/set.html");break;
				case 9997: (void)strcpy(rxBuffer,"GET //home/blescan/set.html");break;
			}
		}

		if(webSocketCmd == TRUE) {
			traceLog(MESSAGE,"MSG120: Websocket GET sequence","",fd);	

			if(strstr(rxBuffer,"/ble_")) {
				traceLog(MESSAGE,"MSG121: Websocket GET read command sequence","",fd);	

				traceLog(MESSAGE,"MSG123: Key:",keyCode,fd);	
				results = sha1B64(keyCode,B64);
				traceLog(MESSAGE,"MSG124: accept:",B64,fd);	

				memset(cmdBuffer,0,100);
				(void)sprintf(cmdBuffer,"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n",B64);

				traceLog(MESSAGE,"MSG126: MSG:",cmdBuffer,fd);	
				(void)write(fd,cmdBuffer,strlen(cmdBuffer));

				memset(cmdBuffer,0,100);
				if(strstr(rxBuffer,"/ble_read")) {
					strcpy(addr,&sharedStr[0]);
					strcpy(name,&sharedStr[100]);
					(void)sprintf(cmdBuffer,"{\"rssi\":\"%d\",\"name\": \"%s\",\"address\": \"%s\"}",sharedInt[0],name,addr);
				}
				else if(strstr(rxBuffer,"/ble_name_only")) {
					sharedInt[1] = 0;		
					(void)sprintf(cmdBuffer,"{\"namersp\":\"only\"}");
				}
				else if(strstr(rxBuffer,"/ble_name_all")) {
					sharedInt[1] = 1;		
					(void)sprintf(cmdBuffer,"{\"namersp\":\"all\"}");
				}

				memset(frmHdr,0x41,100);
				frmHdr[0] = 0x81; // Last frame, opcode = text.
				frmHdr[1] = strlen(cmdBuffer); // payload length

				(void)write(fd,frmHdr,2);

				traceLog(MESSAGE,"MSG722: Sending JSON string to client = ",cmdBuffer,fd);	
				(void)write(fd,cmdBuffer,strlen(cmdBuffer));

				memset(frmHdr,0,200);
				frmHdr[0] = 0x88; // Last frame, opcode = close.
				frmHdr[1] = 0x00; // payload length
				(void)write(fd,frmHdr,2);
			}
		}
		else if(strstr(rxBuffer,"/file")) {
			traceLog(MESSAGE,"MSG122: HTTP GET file sequence","",fd);	
			/* open the file for reading */
			if(( file_fd = open(&rxBuffer[5],O_RDONLY)) == -1) {  
				traceLog(NOTFOUND, "ERR837: failed to open file ",&rxBuffer[5],fd);
			}

			if(strstr(rxBuffer, "")) {
				traceLog(MESSAGE,"MSG993: Data:",rxBuffer,fd);
			}

			len = (long)lseek(file_fd, (off_t)0, SEEK_END); /* lseek to the file end to find the length */
			(void)lseek(file_fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */
			(void)sprintf(rxBuffer,"HTTP/1.1 200 OK\nServer: blescan/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n", VERSION, len, fstr); /* Header + a blank line */

			traceLog(MESSAGE,"MSG990: Header",rxBuffer,fd);

			(void)write(fd,rxBuffer,strlen(rxBuffer));

			/* send file in 8KB block - last block may be smaller */
			while (  (ret = read(file_fd, rxBuffer, BUFSIZE)) > 0 ) {
				(void)write(fd,rxBuffer,ret);
			}
			usleep(500000); // 1/2 second, allow socket to drain before signaling the socket is closed */
			close(file_fd);
		}
		else {
			traceLog(MESSAGE,"ERR111: Unknown GET sequence","",fd);	
		}
		close(fd);
	}

	exit(EXIT_OK);
}

int main(int argc, char **argv)
{
	char hostname[50];
	int i, port, pid, listenfd, socketfd, hit, results; 
	int map = 0;
	socklen_t length;

	static struct sockaddr_in cli_addr; /* static = initialized to zeros */
	static struct sockaddr_in serv_addr; /* static = initialized to zeros */

	if( argc < 3  || argc > 3 || !strcmp(argv[1], "-?") ) {
		(void)printf("Syntax: ./blescan Port-Number Top-Directory\n");
		exit(EXIT_OK);
	} 

	if(chdir(argv[2]) == -1) {
		(void)printf("ERR008: Can't Change to directory %s\n",argv[2]);
		exit(EXIT_NO_DIR);
	}


	hostname[49] = '\0';
	gethostname(hostname, 50);
	printf("MSG022: Hostname for this machine is : %s\n", hostname);
	traceLog(MESSAGE,"MSG022: Hostname for this machine is ",hostname,getpid());
	if(strstr(hostname, "blackbird32")){
		printf("MSG023: System type is a Raspberry pi2\n");
		traceLog(MESSAGE,"MSG023: System type is a raspberry ","pi2",getpid());
		map = 2;
	}
	else if(strstr(hostname, "stealth32")) {
		printf("MSG024: System type is a Raspberry pi3\n");
		traceLog(MESSAGE,"MSG024: System type is a raspberry ","pi3",getpid());
		map = 3;
	}
	
	initBcm2835();		// Init the BCM2835 options,

	if((results = setupTimerInt(map) == FALSE))	{ // Setup the timer and interrupts. 
		printf("ERR024: setupTimerInt has failed.\n");
		return FALSE;
	}

	pthread_mutex_init(&mutex, NULL);
	pthread_create(&th1, NULL, (void *) &readBleDatas, (void *) vectors);

	// onboard LED test.
	//while(1) {
	//	for (i = 0; i < 8; i++) {
	//		setLedRpi(GREEN_LED, (i&1)? 1 : 0);
	//		setLedRpi(RED_LED, (i&2)? 1 : 0);
	//		sleep(2);
	//	}
	//}

	/* Become deamon + unstopable and no zombies children (= no wait()) */
	if(fork() != 0) {
		printf("MSG502: Parent process is all set, but loop here so that the child process doesn't get killed.\n");
		// Don't exit because it will kill ther child process.
  
		while(1)
    		{
      			delay(3000);
    		}
		return 0; /* parent returns OK to shell */
	}

	(void)signal(SIGCLD, SIG_IGN); /* ignore child death */
	(void)signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */

	// Comment this out if you want more printf for debug.
	//for(i=0;i<32;i++) (void)close(i);    /* close open files */
	

	(void)setpgrp();    /* break away from process group */

	traceLog(MESSAGE,"MSG400: blescan server starting on port ",argv[1],getpid());

	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
		traceLog(ERROR, "ERR822: system call","socket",getpid());

	port = atoi(argv[1]);

	
	if(port < 0 || port >60000)
		traceLog(ERROR,"ERR823: Invalid port number (try 1->60000)",argv[1],getpid());
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		traceLog(ERROR,"ERR663: system call:bind Port ",argv[1],getpid());
	
	if( listen(listenfd,64) <0)
		traceLog(ERROR,"ERR825: system call","listen",getpid());

	for(hit=1; ;hit++) {
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			traceLog(ERROR,"ERR827: system call","accept",getpid());

		if((pid = fork()) < 0) {
			traceLog(ERROR,"ERR828: system call","fork",getpid());
		}
		else {
			if(pid == 0)
			{   /* child */
				(void)close(listenfd);
				webService(socketfd,hit,port); /* never returns */
			}
			else
			{   /* parent */
				(void)close(socketfd);
			}
		}
	}
}

static int decodeCommand(char *myString,int fd)
{
	return TRUE;
}

static int macroCommand(char *myString,int fd) {
	return TRUE;
}
/*****************************************************************************
* Use the timer to delay "time" microseconds. Works with interrupts disabled.
******************************************************************************/
static void localTimer(unsigned int time)
{	
	unsigned int timeEnd = *timerValue + time;

	while((((*timerValue) - timeEnd) & 0x80000000) != 0);
}

/**********************************************************************
* Enable/Disable interrupts.
**********************************************************************/
static int sysIrq(int flag)
{
	static unsigned int 	saveIntReg132 = 0;
	static unsigned int 	saveIntReg133 = 0;
	static unsigned int 	saveIntReg134 = 0;
	static unsigned int 	cpyReg128 = 0;
	static unsigned int 	cpyReg129 = 0;
	static unsigned int 	cpyReg130 = 0;

	if(flag == 0)    // disable
	{
		if(saveIntReg132 != 0) // Set to zero in the enable section.
		{
				// Interrupts already disabled so avoid printf()
			return(0);
		}

		cpyReg128 = *(interruptBase+128);
		cpyReg129 = *(interruptBase+129);
		cpyReg130 = *(interruptBase+130);

		if( (*(interruptBase+128) | *(interruptBase+129) | *(interruptBase+130)) != 0)
		{
			printf("MSG999: Pending interrupts, could be ok, just slow? 128=%d, 129=%d, 130=%d\n"
				   ,cpyReg128,cpyReg129,cpyReg130);// may be OK but probably
			return(0);                       	// better to wait for the
		}                                		// pending interrupts to
													// clear
		saveIntReg134 = *(interruptBase+134);
		*(interruptBase+137) = saveIntReg134;

		saveIntReg132 = *(interruptBase+132);  // save current interrupts
		*(interruptBase+135) = saveIntReg132;  // disable active interrupts

		saveIntReg133 = *(interruptBase+133);
		*(interruptBase+136) = saveIntReg133;
	}
	else            // flag = 1 enable
	{
		if(saveIntReg132 == 0)
		{
			printf("MSG552: Interrupts are already enabled. Could be ok, just slow?\n");
			return(0);
		}

		*(interruptBase+132) = saveIntReg132;    // restore saved interrupts
		*(interruptBase+133) = saveIntReg133;
		*(interruptBase+134) = saveIntReg134;
		saveIntReg132 = 0;                 	// indicates interrupts enabled
	}
	return(1);
}

/******************************************************************************
init BCM system
*******************************************************************************/
static int initBcm2835()
{
	if (!bcm2835_init())
	{
		printf("ERR250: Can not init the BCM2835 port\n");
		return(FALSE);
	}
	// Set the pin 12 direction to input, and input with a pullup
	bcm2835_gpio_fsel(PIN12_IN,BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(PIN12_IN, BCM2835_GPIO_PUD_UP);
	printf("MSG290: Configuring GPIO Input  pin # to 12 for IR\n");	

	// Set the pin 11 direction to output (IR).
	bcm2835_gpio_fsel(PIN11_OUT,BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(PIN11_OUT, HIGH);
	printf("MSG292: Configuring GPIO Output pin # to 11 for IR\n");	

	// Set the pin 18 direction to output (Blink LED).
	bcm2835_gpio_fsel(PIN18_OUT,BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(PIN18_OUT, HIGH); //led off
	setLedRpi(GREEN_LED, LOW); // Onboard green LED OFF
	setLedRpi(RED_LED, HIGH); // Onboard red LED ON
	printf("MSG293: Configuring GPIO Output pin # to 18 for LED Blink\n");	

	return(TRUE);
}

/******************************************************************************
Setup the timer and interrupts
*******************************************************************************/
static int setupTimerInt(int map)
{
	int		memfd;
	void		*timer_map,*int_map;
	unsigned int	timeStart = 0;

/******************************************************************************
Sets timer and interrupt pointers for future use. Does not disable interrupts
*******************************************************************************/
	printf("MSG001: Running setup for the Raspberry board.. \n");

	memfd  = open("/dev/mem",O_RDWR|O_SYNC);
	if(memfd < 0)	{ 
		printf("ERR001: SetupRasp Mem open error\n");
		return(0);
	}

	sharedStr= (char *)mmap(NULL,400,PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1,0);
	sharedInt= mmap(NULL,400,PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1,0);
	sharedInt[0]=0;
	//strcpy(sharedStr,"Ready");
	//printf("My Response = %s\n",sharedStr);
	//munmap(shared_mem,4096); // Get rid of memory

	if(map == 2) {
		timer_map = mmap(NULL,4096,PROT_READ|PROT_WRITE, MAP_SHARED,memfd,TIMER_BASE2);
		int_map   = mmap(NULL,4096,PROT_READ|PROT_WRITE, MAP_SHARED,memfd,INT_BASE2);
		printf("MSG001: SetupRasp Mem open successful for model 2.\n");
	}
	else if(map == 3) {
		timer_map = mmap(NULL,4096,PROT_READ|PROT_WRITE, MAP_SHARED, memfd, TIMER_BASE3);
		int_map   = mmap(NULL,4096,PROT_READ|PROT_WRITE, MAP_SHARED, memfd, INT_BASE3);
		printf("MSG001: SetupRasp Mem open successful for model 3.\n");
	}
	else {
		return(0);
	}

	close(memfd);

	// interrupt pointer
	interruptBase = (volatile unsigned *)int_map;

	if(timer_map == MAP_FAILED || int_map == MAP_FAILED)	{ 
		printf("ERR002: SetupTimerInt Map failed\n");
		return(0);
	}
	else {
		printf("MSG002: SetupTimerInt Map successful.\n");
	}


	// timer pointer
	timerValue = (volatile unsigned *)timer_map;

	timeStart = *timerValue;
	printf("MSG050: Timer Low 32 bit startup count = 0x%08x\n",timeStart);

	++timerValue;    	// timer lo 4 bytes
						// timer hi 4 bytes available via *(timer+1)
	//sprintf(str, "%d", map);
	//traceLog(MESSAGE,"MSG001: setupTimerInt() complete with Map = ",str,getpid());
	return(TRUE);
}

static int setLedRpi(Leds led, uint8_t value) {
	int led_handle = -1;
	char value_string[4];

	// Open file
	switch (led) {
		case GREEN_LED:
			led_handle = open("/sys/class/leds/led0/brightness", O_WRONLY);
			break;
		case RED_LED:
			led_handle = open("/sys/class/leds/led1/brightness", O_WRONLY);
			break;
	}
	if (led_handle == -1) {
		printf("ERR200: setLedRpi(%d, %d) failed to open file: %d\n\r", (uint8_t)led, value, errno);
		return FALSE;
	}
	// Write file
	int length = snprintf(value_string, sizeof(value_string), "%d", value);
	if (write(led_handle, value_string, length * sizeof(char)) == -1) {
		printf("ERR201: setLedRpi(%d, %d) failed to write to file: %d\n\r", (uint8_t)led, value, errno);
		close(led_handle);
		return FALSE;
	}
	close(led_handle);
	printf("MSG200: setLedRpi(%d, %d) succeed\n\r", (uint8_t)led, value);
	return TRUE;
}

static char *replaceOneStr(char *str, char *orig, char *rep)
{
  static char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}

static void replaceAllStr(char *target, const char *needle, const char *replacement)
{
    char buffer[1024] = { 0 };
    char *insert_point = &buffer[0];
    const char *tmp = target;
    size_t needle_len = strlen(needle);
    size_t repl_len = strlen(replacement);

    while (1) {
        const char *p = strstr(tmp, needle);

        // walked past last occurrence of needle; copy remaining part
        if (p == NULL) {
            strcpy(insert_point, tmp);
            break;
        }

        // copy part before needle
        memcpy(insert_point, tmp, p - tmp);
        insert_point += p - tmp;

        // copy replacement string
        memcpy(insert_point, replacement, repl_len);
        insert_point += repl_len;

        // adjust pointers, move on
        tmp = p + needle_len;
    }

    // write altered string back to target
    strcpy(target, buffer);
}


