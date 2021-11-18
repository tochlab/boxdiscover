#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <limits.h>
#include <signal.h>

#define MULTICAST_PORT 16384
#define MULTICAST_GROUP "225.0.11.11"

static char *iface_string(char *name, char *addr, char *mask) {
    const char interface_format[] = "\"%s\":{\"addr\":\"%s\",\"mask\":\"%s\"}";
    size_t result_len = strlen(interface_format) + strlen(name) + strlen(addr) + strlen(mask);
    char *result = calloc(result_len, sizeof(char));
    sprintf(result, interface_format, name, addr, mask);
    return result;
}

static char *append(char *dest, const char *src) {
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    dest = realloc(dest, dest_len + src_len + 1);
    return strcat(dest, src);
}

static char *get_address_line() {
    struct ifaddrs *ifap = NULL;
    if (getifaddrs(&ifap) != 0) {
        return NULL;
    }

    char *interfaces_string = (char *) calloc(0, sizeof(char));
    interfaces_string[0] = '\0';
    struct ifaddrs *cur_iface = ifap;
    char str_addr[INET_ADDRSTRLEN];
    char str_mask[INET_ADDRSTRLEN];
    int iface_cnt = 0;
    while (cur_iface != NULL) {
        if (cur_iface->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *inaddr = (struct sockaddr_in *) cur_iface->ifa_addr;
            struct sockaddr_in *nmask = (struct sockaddr_in *) cur_iface->ifa_netmask;
            inet_ntop(AF_INET, &inaddr->sin_addr.s_addr, str_addr, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &nmask->sin_addr.s_addr, str_mask, INET_ADDRSTRLEN);
            char *tmp_result = iface_string(cur_iface->ifa_name, str_addr, str_mask);
            if (iface_cnt != 0) {
                const char *krapka = ",";
                interfaces_string = append(interfaces_string, krapka);
            }
            interfaces_string = append(interfaces_string, tmp_result);
            free(tmp_result);
            iface_cnt++;
        }
        cur_iface = cur_iface->ifa_next;
    }
    freeifaddrs(ifap);
    return interfaces_string;
}

static char *get_hostname_line() {
    char *host = calloc(HOST_NAME_MAX + 1, sizeof(char));
    gethostname(host, HOST_NAME_MAX + 1);

    const char hostname_format[] = "\"hostname\":\"%s\"";
    char *result = calloc(strlen(host) + sizeof(hostname_format), sizeof(char) + 1);
    sprintf(result, hostname_format, host);
    free(host);

    return result;
}

static char *prepare_send_buffer() {
    char *addr_line = get_address_line();
    char *hostname = get_hostname_line();

    const char send_format[] = "{%s,%s}";
    char *result = calloc(strlen(addr_line) + strlen(hostname) + sizeof(send_format) + 1, sizeof(char));
    sprintf(result, send_format, hostname, addr_line);
    free(addr_line);
    free(hostname);
    return result;
}

int running = 0;

void sig_handler(int signum){
    fprintf(stderr,"Got signal %d exiting\n", signum);
    //Return type of the handler function should be void
    running = 0;
}

int main() {
    struct sockaddr_in addr;
    int addrlen, sock, cnt;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }
    bzero((char *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MULTICAST_PORT);
    addrlen = sizeof(addr);

    addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    running = 1;
    while(running) {
        char *send_buffer = prepare_send_buffer();
        cnt = sendto(sock, send_buffer, strlen(send_buffer), 0, (struct sockaddr *) &addr, addrlen);
        if (cnt < 0) {
            perror("sendto");
            exit(1);
        }
        free(send_buffer);
        usleep(60000000);
    }

    return EXIT_SUCCESS;
}
