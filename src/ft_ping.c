#include "ft_ping.h"

void send_ping(t_ping *ping)
{
    ping->icmp.checksum = 0;
    ping->icmp.seq = ping->sequence;
    if (ping->addr.sin_addr.s_addr == 0)
    {
        fprintf(stderr, "Adresse IP non initialisée\n");
        exit(EXIT_FAILURE);
    }

    if (ping->sockfd < 0)
    {
        perror("Erreur lors de la création du socket");
        free(ping->target);
        exit(EXIT_FAILURE);
    }
    ping->icmp.checksum = calculate_icmp_checksum((uint8_t *)&ping->icmp, sizeof(t_icmp));
    gettimeofday(&ping->start, NULL);
    int bytes_sent = sendto(ping->sockfd, &ping->icmp, sizeof(t_icmp), 0, (struct sockaddr *)&ping->addr, sizeof(ping->addr));
    if (bytes_sent < 0)
    {
        perror("Erreur lors de l'envoi");
        close(ping->sockfd);
        free(ping->target);
        exit(EXIT_FAILURE);
    }
    ping->sequence++;
    ping->stats.packets_sent++;
}

void receive_ping(t_ping *ping)
{
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    char response[1024];

    ssize_t n = recvfrom(ping->sockfd, response, sizeof(response), 0, NULL, NULL);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            printf("Request timeout for icmp_seq %d\n", ping->sequence - 1);
        else
            perror("recvfrom");
        return;
    }
    if (n < 64)
    {
        fprintf(stderr, "Erreur : paquet ICMP trop court (%zd bytes)\n", n);
        return;
    }

    uint8_t *ip_header = (uint8_t *)response;
    uint8_t ihl = ip_header[0] & 0x0F;
    uint8_t header_length = ihl * 4;

    if (header_length < 20)
    {
        fprintf(stderr, "Erreur : en-tête IP invalide\n");
        return;
    }

    t_icmp *icmp_header = (t_icmp *)(response + header_length);

    if (icmp_header->type != ICMP_ECHOREPLY)
    {
        if (ping->verb)
            fprintf(stderr, "Reçu ICMP type %d, non-echo reply\n", icmp_header->type);
        return;
    }

    if (icmp_header->id != ping->icmp.id)
    {
        if (ping->verb)
            fprintf(stderr, "ID ICMP incorrect (reçu: %d, attendu: %d)\n",
                    icmp_header->id, ping->icmp.id);
        return;
    }

    if (icmp_header->seq != ping->sequence - 1)
    {
        if (ping->verb)
            fprintf(stderr, "Séquence ICMP incorrecte (reçu: %d, attendu: %d)\n",
                    icmp_header->seq, ping->sequence - 1);
        return;
    }

    size_t icmp_length = n - header_length;
    uint16_t received_checksum = icmp_header->checksum;
    // if (ping->verb)
    // {
    //     printf("Type: %d, Code: %d, ID: %d, Seq: %d, Checksum: 0x%x\n",
    //            icmp_header->type, icmp_header->code,
    //            icmp_header->id, icmp_header->seq, received_checksum);
    // }

    uint16_t checksum_backup = icmp_header->checksum;
    icmp_header->checksum = 0;
    uint16_t calculated_checksum = calculate_icmp_checksum((uint8_t *)icmp_header, icmp_length);
    icmp_header->checksum = checksum_backup;

    // if (ping->verb)
    //     printf("Reçu: 0x%x, Calculé: 0x%x\n", received_checksum, calculated_checksum);

    if (calculated_checksum != received_checksum)
    {
        if (ping->verb)
        {
            fprintf(stderr, "Checksum incorrect (diff=0x%x), paquet accepté quand même\n",
                    (unsigned int)(received_checksum - calculated_checksum));
        }
    }
    // else if (ping->verb)
    //     printf("Checksum vérifié avec succès\n");

    gettimeofday(&ping->end, NULL);
    double rtt = calculate_rtt(&ping->start, &ping->end);
    ping->stats.rtt_total += rtt;
    if (rtt < ping->stats.rtt_min)
        ping->stats.rtt_min = rtt;
    if (rtt > ping->stats.rtt_max)
        ping->stats.rtt_max = rtt;

    printf("%d bytes from %s: icmp_seq=%d ttl=64 time=%.1f ms\n",
       (int)(n - header_length),
       inet_ntoa(ping->addr.sin_addr),
       icmp_header->seq,
       rtt);
    ping->stats.packets_received++;
}


void setup_ping(t_ping *ping, char *target) 
{
    if (!ping || !target)
    {
        printf("%s", "Error setup ping");
        exit(EXIT_FAILURE);
    }

    //stats
    ping->stats.rtt_min = 999999.0;
    ping->stats.rtt_max = 0.0;
    ping->stats.rtt_total = 0.0;
    ping->stats.packets_sent = 0;
    ping->stats.packets_received = 0;
    ping->stats.packets_lost = 0;

    //ping
    ping->target = strdup(target);
    if (ping->target == NULL)
    {
        perror("strdup failed");
        exit(EXIT_FAILURE);
    }
    ping->sockfd = -1;
    ping->sequence = 0;
    ping->pid = getpid() & 0xFFFF;
    ping->start.tv_sec = 0;
    ping->start.tv_usec = 0;
    ping->end.tv_sec = 0;
    ping->end.tv_usec = 0;

    //icmp
    ping->icmp.type = ICMP_ECHO;
    ping->icmp.code = 0;
    ping->icmp.checksum = 0;
    ping->icmp.seq = 0;
    ping->icmp.id = getpid() & 0xFFFF;
    for (size_t i = 0; i < sizeof(ping->icmp.data); i++)
        ping->icmp.data[i] = (char)(i % 256);

    memset(&ping->addr, 0, sizeof(ping->addr));
    ping->addr.sin_family = AF_INET;
    ping->sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
}


int main(int ac, char **av)
{
    t_ping ping;
    int i = 1;
    
    if (geteuid() != 0)
    {
        fprintf(stderr, "ft_ping: Operation not permitted\n");
        return 1;
    }
    
    if (ac < 2)
    {
        fprintf(stderr, "ft_ping: missing host operand\n");
        print_usage();
        return 1;
    }
    if (ac > 2 && av[1][0] != '-')

    {
        fprintf(stderr, "ft_ping: too many arguments\n");
        print_usage();
        return 1;
    }
    
    signal(SIGINT, handle_signal);
    memset(&ping, 0, sizeof(t_ping));
    ping.verb = 0;
    
    while (i < ac && av[i][0] == '-')
    {
        if (strcmp(av[i], "-v") == 0)
        {
            ping.verb = 1;
            printf("Verbose output enabled\n");
        }
        else if (strcmp(av[i], "-?") == 0)
        {
            print_usage();
            return 0;
        }
        else
        {
            fprintf(stderr, "ft_ping: invalid option -- '%s'\n", av[i]);
            print_usage();
            return 1;
        }
        i++;
    }
    
    if (i >= ac)
    {
        fprintf(stderr, "ft_ping: missing host operand\n");
        print_usage();
        return 1;
    }
    
    setup_ping(&ping, av[i]);
    resolve_hostname(&ping);
    
    print_message(&ping);
    while (!stop_flag)
    {
        send_ping(&ping);
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(ping.sockfd, &readfds);
        
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int ready = select(ping.sockfd + 1, &readfds, NULL, NULL, &tv);
        if (ready > 0)
            receive_ping(&ping);
        else if (ready == 0)
        {
            printf("Request timeout for icmp_seq=%d\n", ping.sequence - 1);
        }
        sleep(1);
    }
    print_statistics(&ping);
    close(ping.sockfd);
    free(ping.target);
    return 0;
}
