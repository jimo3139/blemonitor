// if zmq.h is not found, or won't compile run the below 2 lines.
// sudo apt-get install libzmq3-dev
// add -lzmq to the libs

#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <time.h>
#include <sys/mman.h>
#include <dirent.h> 
#include <pthread.h>
#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <zmq.h>
#include "common.h"

int bleScanDevices();

extern int *sharedInt;
extern char *sharedStr;
extern void traceLog(int type, char *s1, char *s2, int socket_fd);

static volatile int signal_received = 0;

typedef int bool;
#define true 1
#define false 0

#define EIR_FLAGS                   	0x01  /* flags */
#define EIR_UUID16_SOME            	0x02  /* 16-bit UUID, more available */
#define EIR_UUID16_ALL              	0x03  /* 16-bit UUID, all listed */
#define EIR_UUID32_SOME             	0x04  /* 32-bit UUID, more available */
#define EIR_UUID32_ALL              	0x05  /* 32-bit UUID, all listed */
#define EIR_UUID128_SOME            	0x06  /* 128-bit UUID, more available */
#define EIR_UUID128_ALL             	0x07  /* 128-bit UUID, all listed */
#define EIR_NAME_SHORT              	0x08  /* shortened local name */
#define EIR_NAME_COMPLETE           	0x09  /* complete local name */
#define EIR_TX_POWER                	0x0A  /* transmit power level */
#define EIR_DEVICE_ID               	0x10  /* device ID */
#define EIR_MANUFACT_ID             	0xff  /* Manufacturer ID */

#define FLAGS_AD_TYPE 			0x01
#define FLAGS_LIMITED_MODE_BIT 		0x01
#define FLAGS_GENERAL_MODE_BIT 		0x02



#define FLAG_LE_LIMITED_DISC_MODE       0x01   /* LE Limited Discoverable Mode. */
#define FLAG_LE_GENERAL_DISC_MODE       0x02   /* LE General Discoverable Mode. */
#define FLAG_BR_EDR_NOT_SUPPORTED       0x04   /* BR/EDR not supported. */
#define FLAG_LE_BR_EDR_CONTROLLER       0x08   /* Simultaneous LE and BR/EDR, Controller. */
#define FLAG_LE_BR_EDR_HOST             0x10   /* Simultaneous LE and BR/EDR, Host. */


typedef struct {
	void (*fn)(uint8_t *, int, uint8_t, void *);
	void *cb_data;
} adv_cb;

static void sigint_handler(int sig)
{
	signal_received = sig;
}


static void eir_parse_name(uint8_t *eir, size_t eir_len, char *buf, size_t buf_len);
static int read_flags(uint8_t *flags, const uint8_t *data, size_t size);
static int check_report_filter(uint8_t procedure, le_advertising_info *info);
static int scan_for_advertising_devices(int dd, uint8_t filter_type, adv_cb *cb);
static void cmd_lescan(int dev_id, adv_cb *cb);


static int read_flags(uint8_t *flags, const uint8_t *data, size_t size)
{
	size_t offset;

	if (!flags || !data)
		return -EINVAL;

	offset = 0;
	while (offset < size) {
		uint8_t len = data[offset];
		uint8_t type;

		/* Check if it is the end of the significant part */
		if (len == 0)
			break;

		if (len + offset > size)
			break;

		type = data[offset + 1];

		if (type == FLAGS_AD_TYPE) {
			*flags = data[offset + 2];
			return 0;
		}

		offset += 1 + len;
	}

	return -ENOENT;
}

static int check_report_filter(uint8_t procedure, le_advertising_info *info)
{
	uint8_t flags;

	/* If no discovery procedure is set, all reports are treat as valid */
	if (procedure == 0)
		return 1;

	/* Read flags AD type value from the advertising report if it exists */
	if (read_flags(&flags, info->data, info->length))
		return 0;

	switch (procedure) {
		case 'l': /* Limited Discovery Procedure */
			if (flags & FLAGS_LIMITED_MODE_BIT)
				return 1;
			break;
		case 'g': /* General Discovery Procedure */
			if (flags & (FLAGS_LIMITED_MODE_BIT | FLAGS_GENERAL_MODE_BIT))
				return 1;
			break;
		default:
			fprintf(stderr, "Unknown discovery procedure\n");
	}

	return 0;
}

void sigBreak(int sig)
{
	char die[16];
	
	strcpy (die, "pkill blescan\n");
    	system(die);

        printf("\nYou have presses Ctrl-C , please press again to exit\n");
	(void) signal(SIGINT, SIG_DFL);
}

static int scan_for_advertising_devices(int dd, uint8_t filter_type, adv_cb *cb)
{
	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
	struct hci_filter nf, of;
	struct sigaction sa;
	socklen_t olen;
	int len;
	char addr[18];
	char name[256];
	char myString[256];

	olen = sizeof(of);
	if (getsockopt(dd, SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
		printf("Could not get socket options\n");
		return -1;
	}

	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);

	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
		printf("Could not set socket options\n");
		return -1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);


	(void) signal(SIGINT, sigBreak); 

	while (1) {
		evt_le_meta_event *meta;
		le_advertising_info *info;
		//uint8_t rssi;
		int8_t rssi;

		while ((len = read(dd, buf, sizeof(buf))) < 0) {
			if (errno == EINTR && signal_received == SIGINT) {
				len = 0;
				goto done;
			}

			if (errno == EAGAIN || errno == EINTR)
				continue;
			goto done;
		}

		ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
		len -= (1 + HCI_EVENT_HDR_SIZE);

		meta = (void *) ptr;

		if (meta->subevent != 0x02)
			goto done;

		/* Ignoring multiple reports */
		info = (le_advertising_info *) (meta->data + 1);

		// Bits 0 to 4 indicate connectable, scannable, directed, scan rsp, and legacy respectively
		//printf("Event: 0x%02x\n",info->evt_type);


		if (check_report_filter(filter_type, info)) {			
			// the rssi is in the next byte after the packet
			rssi = info->data[info->length]; 

			ba2str(&info->bdaddr, addr);
			memset(name, 0, sizeof(name));
			eir_parse_name(info->data, info->length, name, sizeof(name) - 1);

			//int idx = 0;
			//for(idx = 0; idx < info->length; idx++) {
			//	printf("%02x ",info->data[idx]);
			//}
			//printf("\n");

			if(sharedInt[1] == 0) {
 				if(!strstr(name,"(unknown name)")) {
					printf("BLE Filtered Addr: %s Rssi:%d Name:%s\n",addr,rssi,name);
					strcpy(&sharedStr[0],addr);
					strcpy(&sharedStr[100],name);
					sharedInt[0] = (rssi&0x7f);
					(void)sprintf(myString,"RSSI Advertise value = %d",rssi);

					traceLog(MESSAGE,"MSG340: MSG_RSSI: ",myString,0);	
					traceLog(MESSAGE,"MSG341: MSG_ADDR: ",addr,0);	
					traceLog(MESSAGE,"MSG342: MSG_NAME: ",name,0);	
				}
			}
			else if(sharedInt[1] == 1) {
				printf("BLE Non filtered Addr: %s Rssi:%d Name:%s\n",addr,rssi,name);
				strcpy(&sharedStr[0],addr);
				strcpy(&sharedStr[100],name);
				sharedInt[0] = (rssi&0x7f);
			}

			cb->fn(info->data, info->length, rssi, cb->cb_data);
		}
	}

done:
	setsockopt(dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));

	if (len < 0)
		return -1;

	return 0;
}

static void cmd_lescan(int dev_id, adv_cb *cb)
{
	int err, dd;
	uint8_t own_type = 0x00;
	//uint8_t scan_type = 0x00; // passive scan - not sending scan responses
	uint8_t scan_type = 0x01;   // active scan - sending scan responses
	uint8_t filter_type = 0;
	uint8_t filter_policy = 0x00;
	uint16_t interval = htobs(0x0010); 
	uint16_t window = htobs(0x0010);
	uint8_t filter_dup = 0;	// not filtering duplicates 

	if (dev_id < 0)
		dev_id = hci_get_route(NULL);

	dd = hci_open_dev(dev_id);

	if (dd < 0) {
		perror("Could not open device");
		exit(1);
	}

	err = hci_le_set_scan_parameters(dd, scan_type, interval, window,
									 own_type, filter_policy, 1000);
	if (err < 0) {
		perror("Set scan parameters failed");
		exit(1);
	}

	err = hci_le_set_scan_enable(dd, 0x01, filter_dup, 1000);
	if (err < 0) {
		perror("Enable scan failed");
		exit(1);
	}

	printf("LE Scan ...\n");

	err = scan_for_advertising_devices(dd, filter_type, cb);
	if (err < 0) {
		perror("Could not receive advertising events");
		exit(1);
	}

	err = hci_le_set_scan_enable(dd, 0x00, filter_dup, 1000);
	if (err < 0) {
		perror("Disable scan failed");
		exit(1);
	}

	hci_close_dev(dd);
}

static void adv_cb_print_fn(uint8_t *data, int data_length, uint8_t rssi, void *ignore) {
	int i;
	printf("ADV PACKET: ");
	for (i=0; i<data_length; i++) {
		printf("%x ", data[i]);
	}
	printf("\n");

	printf("RSSI: %d dBm\n", rssi);
	printf("\n");	
}

static void verify_all_sent(int should_send, int sent) {
	if (sent < 0) {
		perror("Error when sending data to socket");
		exit(1);
	}
	if (sent >= 0 && sent < should_send) {
		perror("Not all bytes sent");
		exit(1);
	}
}

static void send_to_socket(void *socket, void *data, int data_length, int flags) {
	verify_all_sent(data_length, zmq_send(socket, data, data_length, flags));
}

typedef struct {
	void *socket;
	char *hostname;
	int hostname_len;
} adv_cb_zmq_data;

static void adv_cb_zmq_fn(uint8_t *data, int data_length, uint8_t rssi, 
						  void *_cb_data) {

	adv_cb_zmq_data *cb_data = (adv_cb_zmq_data *) _cb_data;

	send_to_socket(cb_data->socket, cb_data->hostname, cb_data->hostname_len, ZMQ_SNDMORE);
	send_to_socket(cb_data->socket, data, data_length, ZMQ_SNDMORE);
	send_to_socket(cb_data->socket, &rssi, 1, 0);
}

static void report_zmq_version() {
	int major, minor, patch;
	zmq_version (&major, &minor, &patch);
	printf ("Running 0MQ version: %d.%d.%d\n", major, minor, patch);
}

static void eir_parse_name(uint8_t *eir, size_t eir_len, char *buf, size_t buf_len)
{
	size_t offset;

	offset = 0;
	while (offset < eir_len) {
		uint8_t field_len = eir[0];
		size_t name_len;

		/* Check for the end of EIR */
		if (field_len == 0)
			break;

		if (offset + field_len > eir_len)
			goto failed;

		switch (eir[1]) {
			case EIR_NAME_SHORT:
			case EIR_NAME_COMPLETE:
				name_len = field_len - 1;
				if (name_len > buf_len)
					goto failed;

				memcpy(buf, &eir[2], name_len);
				return;
		}

		offset += field_len + 1;
		eir += field_len + 1;
	}

failed:
	snprintf(buf, buf_len, "(unknown name)");
}

int bleScanDevices() {
	char hostname[100];
	gethostname(hostname, sizeof(hostname));
	printf("Hostname: %s\n", hostname);

	// flush stdout immediately
	setvbuf(stdout, NULL, _IONBF, 0);

	report_zmq_version();

	void *context = zmq_ctx_new();
	void *publisher = zmq_socket(context, ZMQ_PUB);
	int rc = zmq_bind(publisher, "tcp://*:8916");
	if (rc < 0) {
		perror("Bind failed");
		exit(1);
	}
	printf("Bound\n");

	adv_cb adv_cb_print = { &adv_cb_print_fn, 0 };
	adv_cb_zmq_data zmq_cb_data = { publisher, hostname, strlen(hostname) };
	adv_cb adv_cb_zmq = { &adv_cb_zmq_fn, &zmq_cb_data };

	cmd_lescan(-1, &adv_cb_zmq);

	printf("Closing ...");
	zmq_close (publisher);
	zmq_ctx_destroy (context);

	return 0;
}
