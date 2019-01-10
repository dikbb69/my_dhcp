#ifndef MYDHCPC_H
#define MYDHCPC_H

#define WAIT_OFFER 2
#define WAIT_ACK 3
#define IN_USE 4
#define REQ_TOUT 5
#define TERMINATE 6

#define DSCV 11
#define REQ_ALLOC 12
#define REQ_EXT 13
#define RCV_ACK 14
#define RELEASE 15
#define UNEXPECTED_MSG 16
#define RCV_TIMEOUT 17
#define RCV_OFFER 18

#define PORT 51229

void msg_set(uint8_t, uint8_t, uint16_t, in_addr_t, in_addr_t);

int status, event, sd;
struct sockaddr_in skt;
socklen_t sktlen = sizeof skt;
struct sockaddr_in skt;
struct in_addr ipaddr;

int wait_event();
void init();
void msg_set(uint8_t, uint8_t, uint16_t, in_addr_t, in_addr_t);
void c_act1();
void c_act2();
void c_act3();


struct proctable {
	int status;
	int event;
	void (*func)();
} ptab[] = {
	{WAIT_OFFER, RCV_OFFER, c_act1},  //OK or NG
	{WAIT_OFFER, RCV_TIMEOUT, c_act2},
	{WAIT_ACK, RCV_ACK, c_act3},     //OK or NG
	{0, 0, NULL}
};


struct dhcph{
	uint8_t    type;
	uint8_t    code;
	uint16_t   ttl;
	in_addr_t  address;
	in_addr_t  netmask;
} msg;


#endif
