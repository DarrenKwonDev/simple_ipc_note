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


int stream_recv(int sock, void *buf, size_t buflen, int flag)
{
    int written = 0;
    int ret;

    while (written < buflen) // 덜 쓰여졌다면
    {
        ret = recv(sock, (char *)buf + written, buflen - written, flag);

        if (ret == -1)
        {
            return ret;
        }

        written += ret;
    }

    return ret;
}


static int do_server()
{
    int ret;
    int sock_fd;
    struct sockaddr_un addr;
    int peer_fd;
    char buf[128];

    // create socket
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
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

    unlink(SOCK_PATH);
    if ( bind(sock_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1 )
    {
        perror("bind()");
        close(sock_fd);
        return -1;
    }

    // listen
    listen(sock_fd, 5);

    // accept
    // 보통 연결되어 socket_fd 와 addr를 반환하지만 
    // 여기서는 편의를 위해 받아오지 않는 것으로 한다.
    peer_fd = accept(sock_fd, NULL, NULL);
    if (peer_fd < 0)
    {
        perror("accept()");
        close(peer_fd);
        return -1;
    }

    // send, recv
    memset(buf, 0, sizeof(buf));

#if 0
    ret = recv(peer_fd, buf, sizeof(buf), 0);
#else
    ret = stream_recv(peer_fd, buf, sizeof(buf), 0);
#endif
    if (ret == -1)
    {
        perror("recv()");
        close(peer_fd);
        return -1;
    }
    printf("client said [%s] \n", buf);

    // close
    close(peer_fd);

    // remove socket file
    if ( unlink(SOCK_PATH) != 0) {
        perror("unlink()");
        return -1;
    }

    return ret;
}

int stream_send(int sock, void *buf, size_t buflen, int flag)
{
    int written = 0;
    int ret;

    while (written < buflen) // 덜 쓰여졌다면
    {
        // buf + written 부터 쓰되 buflen - written 만큼만 써라.
        // 하지만 이건 희망사항이고 실제로 쓰여진 bytes 수는 ret에 반환된다.
        ret = send(sock, (char *)buf + written, buflen - written, flag);

        if (ret == -1)
        {
            return ret;
        }

        written += ret;
    }

    return 0;
}

static int do_client()
{
    int sock_fd;
    struct sockaddr_un addr;
    char buf[128];
    int ret;

    // socket create
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket()");
        return -1;
    }

    // connect
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if ( connect(sock_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1 )
    {
        perror("connect()");
        close(sock_fd);
        return -1;
    }

    // send
    memset(&buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%s", "hello world");

#if 0
    ret = send(sock_fd, buf, sizeof(buf), 0);
#else
    ret = stream_send(sock_fd, buf, sizeof(buf), 0);
#endif

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
