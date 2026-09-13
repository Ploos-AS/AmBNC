#ifndef AMBNC_NET_H
#define AMBNC_NET_H

int ambnc_net_open(void);
void ambnc_net_close(void);
int ambnc_net_connect_ipv4(const char *host, unsigned short port);
int ambnc_net_listen_ipv4(unsigned short port);
int ambnc_net_accept(int listener);
int ambnc_net_send_all(int sock, const char *data, unsigned int length);
int ambnc_net_recv(int sock, char *buffer, unsigned int length);
int ambnc_net_wait_many(const int *socks,
                        unsigned int count,
                        unsigned long signal_mask,
                        unsigned long *signals,
                        unsigned long *ready_mask);
void ambnc_net_close_socket(int sock);

#endif
