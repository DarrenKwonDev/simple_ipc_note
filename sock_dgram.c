#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <unistd.h>

#define SOCK_PATH   "sock_stream_un"

static void print_usage(const char *progname)
{
    printf("%s (server|client) \n", progname);
}


static int do_server()
{
    int ret;
    int sock_fd;
    struct sockaddr_un addr;
    char buf[128];

    // create socket
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd == -1)
    {
        perror("socket()");
        return -1;
    }

    // option setting
    int enable = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        return -1;
    }

    // bind
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCK_PATH); // 편의를 위한 unlink
    if ( bind(sock_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1 )
    {
        perror("bind()");
        close(sock_fd);
        return -1;
    }


    // recvfrom
    memset(buf, 0, sizeof(buf));
    ret = recvfrom(sock_fd, buf, sizeof(buf), 0, NULL, NULL);
    if (ret == -1)
    {
        perror("recv()");
        close(sock_fd);
        return -1;
    }
    printf("client said [%s] \n", buf);


    close(sock_fd);

    // remove socket file
    if ( unlink(SOCK_PATH) != 0) {
        perror("unlink()");
        return -1;
    }

    return ret;
}


static int do_client()
{
    int sock_fd;
    struct sockaddr_un addr;
    char buf[128];
    int ret;

    // socket create
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket()");
        return -1;
    }

    // sendto (UDP 방식이라 partial read, write 가 없음)
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    memset(&buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%s", "hello world");

    ret = sendto(sock_fd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));

    if (ret < 0) {
        perror("send()");
        close(sock_fd);
        return -1;
    }

    close(sock_fd);

    return 0;
}


int main(int argc, char **argv)
{
    if (argc < 2) 
    {
        print_usage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "server"))
    {
        do_server();
    }
    else if (!strcmp(argv[1], "client")) 
    {
        do_client();
    }
    else
    {
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}
