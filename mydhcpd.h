#ifndef MYDHCPS_H
#define MYDHCPS_H

#define WAIT_DSCV 1
#define WAIT_REQ 2
#define IN_USE 3
#define REQ_TOUT 4
#define TERMINATE 5

#define RCV_DSCV 11
#define RCV_REQ_ALLOC 12
#define RCV_REQ_EXT 13
#define ACK 14
#define RCV_RELEASE 15
#define RCV_UNEXPECTED_MSG 16
#define RCV_TIMEOUT 17
#define OFFER 18

#define PORT 51229

struct dhcph{
	uint8_t    type;
	uint8_t    code;
	uint16_t   ttl;
	in_addr_t  address;
	in_addr_t  netmask;
} msg;

void init();
int wait_event();
void alrm_func();
void msg_set(uint8_t, uint8_t, uint16_t, in_addr_t, in_addr_t);
void s_act1();
void s_act2();
void s_act3();
void s_act4();
void s_act5();


int status, event, sd, flag;
struct sockaddr_in skt;
socklen_t sktlen = sizeof skt;
struct timeval interval = { 1, 0 };
struct itimerval itimer = {1,0,1,0};

struct ip {
	int in_use;         //0:割り当てなし 1:未使用 2:使用中
	uint16_t   ttl;
	in_addr_t  address;
	in_addr_t  netmask;
} ip_list[10];

struct proctable {
	int status;
	int event;
	void (*func)();
} ptab[] = {
	{WAIT_DSCV, RCV_DSCV, s_act1},
	{WAIT_REQ, RCV_REQ_ALLOC, s_act2},
	{IN_USE, IN_USE, s_act3},
	{IN_USE, RCV_TIMEOUT, s_act4},
	{IN_USE, RCV_RELEASE, s_act5},
	{0, 0, NULL}
};

struct client {
	struct client *fp;       // for client management
	struct client *bp;
	int stat;                  // client state  0:ACK待ち 1:IN_USE
	int ttlcounter;            // start time
	// below: network byte order
	in_addr_t id;         // client ID (IP address)
	in_addr_t addr;       // allocated IP address
	in_addr_t netmask;    // allocated netmask
	in_port_t port;            // client port number
	uint16_t ttl;              // time to live
};

struct client client_list;  // client list head

void regist_cli(struct ip);
void free_cli();

#endif
