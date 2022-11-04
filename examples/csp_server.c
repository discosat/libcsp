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
int server_start(void);

/* Server port, the port the server listens on for incoming connections from the client. */
#define MY_SERVER_PORT		10

/* test mode, used for verifying that host & client can exchange packets over the loopback interface */
//static bool test_mode = false;
static unsigned int server_received = 0;

/* Client task sending requests to server task */
void client(void) {
	return;
}
/* End of client task */

void server(void) {
	csp_print("Server task started\n");

	/* Create socket with no specific socket options, e.g. accepts CRC32, HMAC, XTEA, etc. if enabled during compilation */
	csp_socket_t sock = {0};

	/* Bind socket to all ports, e.g. all incoming connections will be handled here */
	csp_bind(&sock, CSP_ANY);

	/* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued */
	csp_listen(&sock, 10);

	/* Wait for connections and then process packets on the connection */
	while (1) {

		/* Wait for a new connection, 10000 mS timeout */
		csp_conn_t *conn;
		if ((conn = csp_accept(&sock, 10000)) == NULL) {
			/* timeout */
			continue;
		}

		csp_print("Connection opened succesfully.\n");
		/* Read packets on connection, timout is 100 mS */
		csp_packet_t *packet;
		while ((packet = csp_read(conn, 50)) != NULL) {
			switch (csp_conn_dport(conn)) {
			case MY_SERVER_PORT:
				/* Process packet here */
				csp_print("Packet received on MY_SERVER_PORT: %s\n", (char *) packet->data);
				csp_buffer_free(packet);
				++server_received;
				break;

			default:
				/* Call the default CSP service handler, handle pings, buffer use, etc. */
				csp_service_handler(packet);
				break;
			}
		}

		/* Close current connection */
		csp_close(conn);
		csp_print("Connection closed.\n");

	}
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

	server_start();

	while(1) {
		sleep(3);
	}

	return 0;
}
