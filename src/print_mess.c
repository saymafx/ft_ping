#include "ft_ping.h"

void print_statistics(t_ping *ping)
{
    ping->stats.packets_lost = 100 * (ping->stats.packets_sent - ping->stats.packets_received) / ping->stats.packets_sent;
    if (ping->stats.packets_received > 0)
        ping->stats.rtt_total /= ping->stats.packets_received;
    else
        ping->stats.rtt_total = 0.0;
    printf("\n--- Statistiques ft_ping ---\n");
    printf("%d paquets envoyés, %d reçus, %d%% perdus\n",
           ping->stats.packets_sent, ping->stats.packets_received, ping->stats.packets_lost);
    printf("RTT (ms) : min %.3f, moy %.3f, max %.3f\n",
           ping->stats.rtt_min, ping->stats.rtt_total, ping->stats.rtt_max);
}

void    print_message(t_ping *ping)
{
    printf("PING %s (%s) : %d octets de données\n",
            ping->target, inet_ntoa(ping->addr.sin_addr), PACKET_SIZE);
}

void print_usage(void)
{
    printf("Usage: ft_ping [options] <destination>\n");
    printf("Options:\n");
    printf("  -v    Enable verbose mode\n");
    printf("  -?    Show this help message\n");
}