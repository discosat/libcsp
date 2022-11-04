#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/utsname.h>
#include <time.h>
#include <csp/csp_debug.h>
#include <string.h>
#include <csp/csp_cmp.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/interfaces/csp_if_zmqhub.h>
#include <csp/csp_yaml.h>

/* These three functions must be provided in arch specific way */
int router_start(void);
int client_start(void);

/* Server port, the port the server listens on for incoming connections from the client. */
#define MY_SERVER_PORT		10

/* Commandline options */
static uint8_t server_address = 7;

/* test mode, used for verifying that host & client can exchange packets over the loopback interface */
static bool test_mode = false;
//static unsigned int server_received = 0;

/* Client task sending requests to server task */
void client(void) {

	csp_print("\nClient task started\n\n");

	unsigned int count = 'A';

	while (1) {

		usleep(test_mode ? 200000 : 1000000);

		/* Send ping to server, timeout 1000 mS, ping size 100 bytes */
		int result = csp_ping(server_address, 1000, 100, CSP_O_CRC32);
		csp_print("Ping address: %u, result %d [mS]\n", server_address, result);
        (void) result;

		/* Send reboot request to server, the server has no actual implementation of csp_sys_reboot() and fails to reboot */
		csp_reboot(server_address);
		csp_print("reboot system request sent to address: %u\n", server_address);

		/* Send data packet (string) to server */

		/* 1. Connect to host on 'server_address', port MY_SERVER_PORT with regular UDP-like protocol and 1000 ms timeout */
		csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, server_address, MY_SERVER_PORT, 1000, CSP_O_NONE);
		if (conn == NULL) {
			/* Connect failed */
			csp_print("Connection failed\n");
			return;
		}

		/* 2. Get packet buffer for message/data */
		csp_packet_t * packet = csp_buffer_get(100);
		if (packet == NULL) {
			/* Could not get buffer element */
			csp_print("Failed to get CSP buffer\n");
			return;
		}

		/* 3. Copy data to packet */
        memcpy(packet->data, "Hello world ", 12);
        memcpy(packet->data + 12, &count, 1);
        memset(packet->data + 13, 0, 1);
        count++;

		/* 4. Set packet length */
		packet->length = (strlen((char *) packet->data) + 1); /* include the 0 termination */

		/* 5. Send packet */
		csp_send(conn, packet);

		/* 6. Close connection */
		csp_close(conn);

		csp_print("\n");
	}

	return;
}
/* End of client task */

void server(void) {
	return;
}

void csp_scan() {
	unsigned int begin = 0;
	unsigned int end = 16;

	printf("==============================\n");
	printf("Scanning for csp devices on address %u-%u.\n", begin, end);
	for (unsigned int i = begin; i <= end; i++) {
        	printf("trying %u: \r", i);
        	if (csp_ping(i, 20, 0, CSP_O_CRC32) >= 0) {
			printf("Found something on addr %u...\n", i);
			struct csp_cmp_message message;
			if (csp_cmp_ident(i, 100, &message) == CSP_ERR_NONE) {
				printf(
					"%s\n%s\n%s\n%s %s\n\n", 
					message.ident.hostname, 
					message.ident.model, 
					message.ident.revision, 
					message.ident.date, 
					message.ident.time);
			}
		}
        }

	printf("Scan finished.\n");
	printf("==============================\n");
	return;
}

/* main - initialization of CSP and start of server/client tasks */
int main(int argc, char * argv[]) {
	int c;
	int csp_version = 2;
	//char * rtable = NULL;
	char * yamlname = (char *)"../../examples/iflist.yaml";
	unsigned int dfl_addr = 0;
	
	while ((c = getopt(argc, argv, "+hpn:v:r:f:")) != -1) {
		switch (c) {
		case 'v':
			csp_version = atoi(optarg);
			break;
		case 'n':
			dfl_addr = atoi(optarg);
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}


	static char hostname[100];
	gethostname(hostname, 100);

	static char domainname[100];
	int res = getdomainname(domainname, 100);
	(void) res;

	struct utsname info;
	uname(&info);

	csp_conf.hostname = info.nodename;
	csp_conf.model = info.version;
	csp_conf.revision = info.release;
	csp_conf.version = csp_version;
	csp_conf.dedup = CSP_DEDUP_OFF;
	csp_init();

	//csp_debug_set_level(4, 1);
	//csp_debug_set_level(5, 1);

	csp_yaml_init(yamlname, &dfl_addr);

	//csp_rdp_set_opt(3, 10000, 5000, 1, 2000, 2);

	router_start();

	csp_print("Connection table\r\n");
	csp_conn_print_table();

	csp_print("Interfaces\r\n");
	csp_rtable_print();

	csp_print("Route table\r\n");
	csp_iflist_print();

	csp_scan();

	client_start();

	while(1) {
		sleep(3);
	}

	return 0;
}
