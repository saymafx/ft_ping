#include "ft_ping.h"
volatile sig_atomic_t stop_flag = 0;

double calculate_rtt(struct timeval *start, struct timeval *end)
{
    long sec = end->tv_sec - start->tv_sec;
    long usec = end->tv_usec - start->tv_usec;
    return (sec * 1000.0) + (usec / 1000.0);
}



uint16_t calculate_checksum(void *buffer, int length) {
    uint16_t *data = buffer;
    uint32_t sum = 0;

    for (int i = 0; i < length / 2; i++)
        sum += data[i];

    if (length % 2)
        sum += ((uint8_t *)buffer)[length - 1];

    sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

void handle_signal(int signal)
{
    if (signal == SIGINT)
    {
        printf("\nSignal SIGINT reçu. Arrêt du programme.\n");
        stop_flag = 1;
    }
}

void resolve_hostname(t_ping *ping)
{
    struct addrinfo hints;
    struct addrinfo *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;

    if (inet_pton(AF_INET, ping->target, &ping->addr.sin_addr) == 1)
    {
        ping->addr.sin_family = AF_INET;
        return ;
    }
    status = getaddrinfo(ping->target, NULL, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "Erreur de résolution DNS pour %s : %s\n",
                ping->target, gai_strerror(status));
        exit(EXIT_FAILURE);
    }
    memcpy(&(ping->addr), res->ai_addr, sizeof(struct sockaddr_in));
    printf("Adresse résolue avec succès !\n");
    freeaddrinfo(res);
}