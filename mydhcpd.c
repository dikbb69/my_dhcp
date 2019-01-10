#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "mydhcpd.h"

int main (int argc, char *argv[]) {
    struct sockaddr_in sskt;
    struct in_addr addr;
    struct proctable *pt;

    signal(SIGALRM, alrm_func);

    //設定ファイルの読み込み
    init(argv[1]);

    //ソケット構造体の初期化
    memset(&sskt, 0, sizeof(sskt));
    sskt.sin_family = AF_INET;
    sskt.sin_port = htons(PORT);
    sskt.sin_addr.s_addr = htonl(INADDR_ANY);

    //ソケット記述子
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        return (-1);
    }

    //bind
    if (bind(sd, (struct sockaddr *)&sskt, sizeof(sskt)) == -1) {
        perror("bind");
        return (-1);
    }
    //初期のstatus
    status = WAIT_DSCV;

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
            printf("ERROR: incompatible sequence\n");
            status = WAIT_DSCV;
        }
        //error processing;
    }

    close(sd);

    return 0;
}

void init(char *name) {
    FILE *fp; // FILE型構造体
	char *fname = name;
    char addr[20], netmask[20];
    int ttl, i;

	fp = fopen(fname, "r"); // ファイルを開く。失敗するとNULLを返す。
	if(fp == NULL) {
		printf("%s file not open!\n", fname);
		exit(-1);
	} else {
        printf("---------- config file ----------\n");
        fscanf(fp, "%d", &ttl);
        printf("ttl: %d\n", ttl);
        for (i = 0; fscanf(fp, "%s %s", addr, netmask) != EOF; i++) {
            ip_list[i].in_use = 1; ip_list[i].ttl = (uint16_t)ttl;
            ip_list[i].address = inet_addr(addr);
            ip_list[i].netmask = inet_addr(netmask);
		    printf("addr:%s netmask:%s\n", addr, netmask);
	    }
        printf("\n");
        for (; i < 10; i++) {
            ip_list[i].in_use = 0;
        }
	}

    fclose(fp); // ファイルを閉じる

    client_list.fp = &client_list;
    client_list.bp = &client_list;
}

int wait_event(int sd) {
    if (status == IN_USE) {
        struct client *p;
        p = client_list.fp;
        pause();
        if (flag > 0) {
            flag = 0;
            while (p != &client_list) {
                p->ttlcounter--;
                struct in_addr addr = {p->addr};
                printf("-- client IP: %s, TTL: %d --\n", inet_ntoa(addr), p->ttlcounter);
                if (p->ttlcounter == 0) {
                    printf("send timeout\n");
                    return RCV_TIMEOUT;
                }
                p = p->fp;
            }
        }
        return IN_USE;
    } else {
        if (status == WAIT_DSCV) {
            printf("----- Wait for \"DISCOVER\" -----\n");
        } else {
            printf("\n----- Wait for event -----\n");
        }
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
    }
    return msg.type;
}

void msg_set(uint8_t type, uint8_t code, uint16_t ttl, in_addr_t address, in_addr_t netmask) {
    memset(&msg, 0, sizeof(msg));
    msg.type = type;
    msg.code = code;
    msg.ttl = ttl;
    msg.address = address;
    msg.netmask = netmask;
}

void s_act1() {
    int i;
    printf("Receive \"DISCOVER\"\n");
    status = WAIT_REQ;
    for (i = 0; ip_list[i].in_use != 0; i++) {
        if (ip_list[i].in_use == 1) {
            msg_set(OFFER, 0, ip_list[i].ttl, ip_list[i].address, ip_list[i].netmask);
            printf("Send \"OFFER\"\n");
            ip_list[i].in_use = 2;
            regist_cli(ip_list[i]);
            break;
        }
    }
    if (ip_list[i].in_use == 0) {
        msg_set(OFFER, 1, 0, 0, 0);
    }

    if (sendto(sd, &msg, sizeof(msg), 0, (struct sockaddr *)&skt, sizeof skt) < 0) {
        perror("sendto");
        exit(1);
    }
}

void s_act2() {
    struct client *p;
    printf("Receive \"REQ_ALLOC\"\n");
    for (p = client_list.fp; p != &client_list; p = p->fp) {
        if (p->id == skt.sin_addr.s_addr) break;
    }
    if (p == &client_list) {  //ACK NG
        printf("From unexpected IP\nExiting\n");
        msg_set(ACK, 4, 0, 0, 0);
        if (sendto(sd, &msg, sizeof(msg), 0, (struct sockaddr *)&skt, sizeof skt) < 0) {
            perror("sendto");
            exit(1);
        }
        free_cli();
        status = WAIT_DSCV;
    }
    if (msg.ttl > p->ttl) {  //ACK NG
        printf("incompatible TTL\nExiting\n");
        msg_set(ACK, 4, 0, 0, 0);
        if (sendto(sd, &msg, sizeof(msg), 0, (struct sockaddr *)&skt, sizeof skt) < 0) {
            perror("sendto");
            exit(1);
        }
        free_cli();
        status = WAIT_DSCV;
    }
    if (msg.netmask != p->netmask) {  //ACK NG
        printf("incompatible netmask\nExiting\n");
        msg_set(ACK, 4, 0, 0, 0);
        if (sendto(sd, &msg, sizeof(msg), 0, (struct sockaddr *)&skt, sizeof skt) < 0) {
            perror("sendto");
            exit(1);
        }
        free_cli();
        status = WAIT_DSCV;
    }
    if (msg.code == 2) {
        printf("Send \"ACK\"\n");
        msg_set(ACK, 0, p->ttl, p->addr, p->netmask);
        if (sendto(sd, &msg, sizeof(msg), 0, (struct sockaddr *)&skt, sizeof skt) < 0) {
            perror("sendto");
            exit(1);
        }
        // タイマ設定 1秒毎に通知
        int ret = setitimer(ITIMER_REAL, &itimer, NULL);
        if ( ret != 0 ) {
                perror("setitimer error");
        }
        status = IN_USE;
    }
}

void regist_cli(struct ip ip) {
    struct client *new, *p;
    new = (struct client*)malloc(sizeof(struct client));
    client_list.bp->fp = new;
    new->fp = &client_list;
    new->bp = client_list.bp;
    client_list.bp = new;
    new->ttlcounter = ip.ttl;
    new->stat = 0;
    new->id = skt.sin_addr.s_addr;
    new->addr = ip.address;
    new->netmask = ip.netmask;
    new->port = skt.sin_port;
    new->ttl = ip.ttl;
    struct in_addr str = {new->id};
    printf("new client creat, %s\n", inet_ntoa(str));
    printf("**client list**\n");
    for (p = client_list.fp; p != &client_list; p = p->fp) {
        str.s_addr = p->id;
        printf("    id:%s", inet_ntoa(str));
        str.s_addr = p->addr;
        printf(",  ip:%s\n", inet_ntoa(str));
    }
}

void free_cli() {
    struct client *p;
    for (p = client_list.fp; p != &client_list; p = p->fp) {
        if (p->id == skt.sin_addr.s_addr) break;
    }
    int i;
    for (i = 0; i < 10; i++) {
        if (ip_list[i].address == p->addr) {
            ip_list[i].in_use = 1;
            printf("Free ");
            struct in_addr str = {p->id};
            printf("    id:%s", inet_ntoa(str));
            str.s_addr = p->addr;
            printf(",  ip:%s\n", inet_ntoa(str));
        }
    }
    p->bp->fp = p->fp;
    p->fp->bp = p->bp;
    free(p);
    printf("**IP pool**\n");
    struct in_addr str;
    for (i = 0; i < 10; i++) {
        if (ip_list[i].in_use == 1) {
            str.s_addr = ip_list[i].address;
            printf("    %s\n", inet_ntoa(str));
        }
    }
    printf("**client list**\n");
    for (p = client_list.fp; p != &client_list; p = p->fp) {
        str.s_addr = p->id;
        printf("    id:%s", inet_ntoa(str));
        str.s_addr = p->addr;
        printf(",  ip:%s\n", inet_ntoa(str));
    }
}

void s_act3() {

}

void s_act4() {
    struct client *p;
    p = client_list.fp;
    while (p != &client_list) {
        if (p->ttlcounter == 0) break;
        p = p->fp;
    }
    if (p->ttlcounter == 0) {
        struct in_addr addr = {p->addr};
        printf("TTL timeout\n", inet_ntoa(addr));
        free_cli();
        status = WAIT_DSCV;
    }
}

void s_act5() {
    struct client *p;
    p = client_list.fp;
    while (p != &client_list) {
        if (p->id == skt.sin_addr.s_addr) {
            struct in_addr addr = {p->addr};
            printf("-- client IP: %s, Release --\n", inet_ntoa(addr));
            free_cli();
            status = WAIT_DSCV;
        }
        p = p->fp;
    }
}

void alrm_func() {
    flag++;
}
