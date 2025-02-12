#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FIFO_FILENAME       "./testfifo"

static void print_usage(char* progname)
{
    printf("%s (s|w)\n", progname);
    return;
}

static int do_reader()
{
    int fd;
    char buf[128];

    fd = open(FIFO_FILENAME, O_RDONLY);
    if (fd < 0) {
        perror("open()");
        return -1;
    }

    read(fd, buf, sizeof(buf));

    fprintf(stdout, "recv : %s \n", buf);
    fflush(stdout);

    close(fd);

    return 0;
}

static int do_writer()
{
    int fd;
    char buf[128];

    unlink(FIFO_FILENAME); // 개발 편의를 위해 전의 fifo는 삭제

    if (mkfifo(FIFO_FILENAME, 0644)) // rw r r
    {
        perror("mkfifo()");
        return -1;
    }

    fd = open(FIFO_FILENAME, O_WRONLY); // blocking (fifo 는 read, write 양쪽의 open이 있어야 blocking이 통과됨)
    if (fd < 0) {
        perror("open()");
        return -1;
    }

    strncpy(buf, "hello", sizeof(buf));
    write(fd, buf, sizeof(buf));

    close(fd);

    return 0;
}


// fifo (s|r)
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "r"))
    {
        // reader
        do_reader();
    } 
    else if (!strcmp(argv[1], "w"))
    {
        // writer
        do_writer();
    }
    else 
    {
        print_usage(argv[0]);
        return -1;
    }


    return 0;
}
