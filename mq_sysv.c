#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/msg.h>
#include <sys/ipc.h>

#define IPC_KEY_FILENAME            "/proc"
#define IPC_KEY_PROJ_ID             'a'

// search `man msgsend` for this definition
struct msgbuf {
    long mtype;

#define MSGBUF_STR_SIZE  64
    char string[MSGBUF_STR_SIZE];
};

//---------------------------------------------------

static void print_usage(const char *progname)
{
    printf("%s (send|recv) \n", progname);
}

static int init_msgq(void)
{
    int msgq;
    key_t key;

    // create key
    key = ftok(IPC_KEY_FILENAME, IPC_KEY_PROJ_ID);
    if (key == -1) {
        perror("ftok");
        return -1;
    }

    // return mq identifier
    msgq = msgget(key, 0644 | IPC_CREAT);
    if (msgq == -1) {
        perror("msgget()");
        return -1;
    }

    return msgq;
}


static int do_send(void)
{
    int msgq;
    struct msgbuf mbuf;

    msgq = init_msgq();
    if (msgq == -1)
    {
        perror("init_msgq");
        return -1;
    }

    // create mq msg
    memset(&mbuf, 0, sizeof(struct msgbuf));
    mbuf.mtype = 1; // send 하는 입장에서 mtype을 1로 지정
    strncpy(mbuf.string, "hello world", sizeof(mbuf.string) - 1);

    // actually send to mq
    if (msgsnd(msgq, &mbuf, sizeof(mbuf.string), 0) != 0) {
        perror("msgsnd failed");
        return -1;
    }

    return 0;
}

static int do_recv(void)
{
    int msgq;
    struct msgbuf mbuf;
    int ret;

    msgq = init_msgq();
    if (msgq == -1)
    {
        perror("init_msgq");
        return -1;
    }

    // create mq msg
    memset(&mbuf, 0, sizeof(struct msgbuf));

    // actually recv
    // msgtyp에 0을 넣었으므로 무엇이든 첫번째 메시지를 수신하게 됨.
    ret = msgrcv(msgq, &mbuf, sizeof(mbuf.string), 0, 0);
    if (ret == -1) {
        perror("msgrcv()");
        return -1;
    }
    printf("recv msg : mtype %d , msg [%s] \n", mbuf.mtype, mbuf.string);

    return 0;
}

int main(int argc, char **argv)
{
    int ret;

    if (argc != 2) {
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
