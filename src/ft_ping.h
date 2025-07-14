#ifndef FT_PING_H
#define FT_PING_H

#include <netdb.h>       // Pour gethostbyname, getaddrinfo
#include <sys/types.h>   // Pour les types comme u_int16_t
#include <netinet/in.h>  // Pour sockaddr_in, IPPROTO_ICMP
#include <arpa/inet.h>   // Pour inet_pton, inet_ntop
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <netinet/ip.h>
#include <errno.h>

#define PACKET_SIZE 64
#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0

extern struct timeval timeout;


// Structure ICMP (basée sur la norme RFC 792)
typedef struct s_icmp {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
    char data[PACKET_SIZE - sizeof(uint8_t) * 2 - sizeof(uint16_t) * 3];
} t_icmp;

typedef struct s_stats {
    int packets_sent;
    int packets_received;
    int packets_lost;
    double rtt_min;
    double rtt_max;
    double rtt_total;
} t_stats;

typedef struct s_ping {
    int verb;
    char *target;
    int sockfd;
    int sequence;
    struct timeval start;
    struct timeval end;
    struct sockaddr_in addr;
    t_stats stats;
    t_icmp icmp;
    uint16_t pid;
} t_ping;

extern volatile sig_atomic_t stop_flag;
void send_ping(t_ping *ping);
void receive_ping(t_ping *ping);
double calculate_rtt(struct timeval *start, struct timeval *end);
void handle_signal(int sig);
void setup_ping(t_ping *ping, char *target);
void resolve_hostname(t_ping *ping);
void print_statistics(t_ping *ping);
void print_message(t_ping *ping);
void print_usage(void);
uint16_t calculate_icmp_checksum(uint8_t *data, int length);

#endif
