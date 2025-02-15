#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>

#define POSIX_MQ_NAME               "/testmq"


#if 0
       mqd_t mq_open(const char *name, int oflag);
       mqd_t mq_open(const char *name, int oflag, mode_t mode,
                     struct mq_attr *attr);
#endif


struct msgbuf {
#define MSGBUF_STR_SIZE  8192
    char string[MSGBUF_STR_SIZE];
};

//---------------------------------------------------

static void print_usage(const char *progname)
{
    printf("%s (send|recv) \n", progname);
}

static int init_msgq(void)
{
    mqd_t mqd; // fd
    struct mq_attr attr;
    

    mqd = mq_open(POSIX_MQ_NAME, O_RDWR | O_CREAT, 0644, NULL); // NULL means, system default
    if (mqd == -1) {
        perror("mq_open()");
        return -1;
    }

    // get system default attr
    memset(&attr, 0, sizeof(attr));
    if (mq_getattr(mqd, &attr) == -1) {
        close(mqd);
        return -1;
    }
    printf("[init_msgq] mq_flags : %lu | mq_maxmsg, : %lu | mq_msgqsize : %lu | mq_curmsgs : %lu \n", 
           attr.mq_flags, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);

    return mqd; 
}


static int do_send(void)
{
    int mqd;
    struct msgbuf mbuf;
    char buf[MSGBUF_STR_SIZE];

    mqd = init_msgq();
    if (mqd == -1)
    {
        perror("init_msgq");
        return -1;
    }

    memset(&buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "hello world from pid : %d", getpid());
    if (mq_send(mqd, buf, sizeof(buf), 0) == -1)
    {
        perror("mq_send()");
        close(mqd);
        return -1;
    }

    return 0;
}

static int do_recv(void)
{
    int mqd;
    char buf[MSGBUF_STR_SIZE];

    mqd = init_msgq();
    if (mqd == -1)
    {
        perror("init_msgq");
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    if ( mq_receive(mqd, buf, sizeof(buf), NULL) == -1)
    {
        perror("mq_receive()");
        close(mqd);
        return -1;
    }
    printf("recev : [%s] \n", buf);

    return 0;
}

int main(int argc, char **argv)
{
    int ret;

    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "send"))
    {
        ret = do_send();
    }
    else if (!strcmp(argv[1], "recv"))
    {
        ret = do_recv();
    }
    else 
    {
        print_usage(argv[0]);
        return -1;
    }

    return ret;
}
