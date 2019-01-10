#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "mydhcpc.h"


int main(int argc, char *argv[]) {
	int count, datalen;
    struct proctable *pt;


	if ((sd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

    skt.sin_family = AF_INET;
	skt.sin_port = htons(PORT);
	skt.sin_addr.s_addr = inet_addr(argv[1]);

    //dhcpサーバーにDISCOVER送信

    init(sd);

    //メインループ
    for (;;) {
        event = wait_event(sd);  //クライアントからのmsg受信待ち
        //状態遷移
        for (pt = ptab; pt->status; pt++) {
            if (pt->status == status && pt->event == event) {
                (*pt->func)();
                break;
            }
        }
        if (pt->status == 0) {
            printf("ERROR: incompatible sequence\nExiting\n");
            exit(1);
        }
        //error processing;
    }

	close(sd);

	return 0;
}

void msg_set(uint8_t type, uint8_t code, uint16_t ttl, in_addr_t address, in_addr_t netmask) {
    memset(&msg, 0, sizeof(msg));
    msg.type = type;
    msg.code = code;
    msg.ttl = ttl;
    msg.address = address;
    msg.netmask = netmask;
}

void init(int sd) {
    printf("send DISCOVER\n");
    msg_set(DSCV, 0, 0, 0, 0);
    if (sendto(sd, &msg, sizeof(msg), 0, (struct sockaddr *)&skt, sizeof skt) < 0) {
        perror("sendto");
        exit(1);
    }
    status = WAIT_OFFER;
}

int wait_event(int sd) {
    printf("\n----- wait for event -----\n");
    if ((recvfrom(sd, &msg, sizeof(msg), 0,
            (struct sockaddr *)&skt, &sktlen)) == -1) {
        perror("recv");
    }
    struct in_addr addr = {msg.address};
    char ad[20]; memcpy(ad, inet_ntoa(addr), 20);
    struct in_addr mask = {msg.netmask};
    char ms[20]; memcpy(ms, inet_ntoa(mask), 20);
    printf("recv: type: %d\n"
           "      code: %d\n"
           "      ttl: %d\n"
           "      addr: %s\n"
           "      netmask: %s\n",
            msg.type, msg.code, msg.ttl, ad, ms);
    return msg.type;
}

void c_act1() {
    if (msg.code == 1) {  //recv offer NG
        printf("Receive NG\nexiting\n");
        exit(1);
    } else {              //recv offer OK
        printf("Receive \"OFFER\"\n");
        status = WAIT_ACK;
        msg_set(REQ_ALLOC, 2, msg.ttl, msg.address, msg.netmask);
        printf("Send \"REQ_ALLOC\"\n");
        if (sendto(sd, &msg, sizeof(msg), 0, (struct sockaddr *)&skt, sizeof skt) < 0) {
            perror("sendto");
            exit(1);
        }
    }
}

void c_act2() {
}

void c_act3() {
    if (msg.code == 0) {          //ACK OK
        printf("Receive \"ACK\"\n");
        status = IN_USE;
    } else if (msg.code == 4) {  //ACK NG
        printf("Receive ACK NG\nExiting\n");
        exit(1);
    }
}
