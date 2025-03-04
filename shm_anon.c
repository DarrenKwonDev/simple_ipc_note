#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define SHARED_FILENAME     "shared"

struct login_info 
{
    int pid;
    int counter;
};

static void print_usage(const char *progname)
{
    printf("%s (monitor) \n", progname);
}

struct login_info *login_info_init(void)
{
    struct login_info *info;

    // void *mmap(void addr[.length], size_t length, int prot, int flags, int fd, off_t offset);
    info = mmap(NULL, sizeof(struct login_info), 
                PROT_READ | PROT_WRITE, 
                MAP_SHARED | MAP_ANONYMOUS, 
                -1, 0);
    if (info == MAP_FAILED)
    {
        perror("mmap()");
        return NULL;
    }
    memset(info, 0, sizeof(struct login_info));

    // dangling 포인터가 아님. mmap에 의해 프로세스 주소 공간에 직접 매핑됨.
    // munmap에 의해 해제될 때 까지 해당 포인터는 유효함
    return info;
}

static int do_monitoring(struct login_info *info)
{
    struct login_info local;
    int n;

    memset(&local, 0, sizeof(local));
    n = 0;
    while (1) 
    {
        if (memcmp(&local, info, sizeof(struct login_info)) != 0)
        {
            printf("pid : %d, counter : %d \n", info->pid, info->counter);
            memcpy(&local, info, sizeof(struct login_info));

            n++;
            if (n == 5) break;
        }

        usleep(10000);
    }

    //int munmap(void addr[.length], size_t length);
    munmap(info, sizeof(struct login_info));

    return 0;
}

static int do_login(int i, struct login_info *info)
{

    sleep(i);

    info->pid = getpid();
    info->counter++;

    return 0;
}

int main(int argc, char **argv)
{
    int i;
    int pid;
    struct login_info *info;

    info = login_info_init();
    if (!info)
    {
        perror("login_info_init");
        return -1;
    }

#define NUM_FORK     (5)
    for(i = 0; i < NUM_FORK; i++)
    {
        pid = fork();
        if (pid > 0)
        {
            /* parent */
        }
        else if (pid == 0)
        {
            /* child */
            do_login(i, info);
            munmap(info, sizeof(struct login_info));
        }
        else
        {
            perror("fork()");
            return -1;
        }
    }

    do_monitoring(info);

    for(i = 0; i < NUM_FORK; i++)
    {
        pid = wait(NULL);
        printf("pid is end : (%d) \n", pid);
    }


    return 0;
}
