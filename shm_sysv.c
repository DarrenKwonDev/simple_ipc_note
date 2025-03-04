#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/procfs.h>

#define SHARED_FILENAME     "shared"
#define SHMKEY_FILEPATH     "/tmp"
#define ShMKEY_PROJID       'r'

#define ROUNDUP(x)          (((x) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1))

struct login_info 
{
    int pid;
    int counter;
};

static void print_usage(const char *progname)
{
    printf("%s (monitor) \n", progname);
}

static int do_monitoring(void)
{
    // monitoring create file shm
    struct login_info local;
    struct login_info *info;
    int n; int shmid;
    key_t key;
    size_t size;


    //----------------------------------------
    // create shm
    key = ftok(SHMKEY_FILEPATH, ShMKEY_PROJID);
    if (key == -1)
    {
        perror("ftok()");
        return -1;
    }

    // man 페이지에 따르면, 
    // A  new  shared  memory  segment, with size equal to the value of size rounded up to a multiple of PAGE_SIZE
    size = ROUNDUP(sizeof(struct login_info));
    shmid = shmget(key, size, IPC_CREAT | 0644);
    if (shmid == -1)
    {
        perror("shmget()");
        return -1;
    }

    info = shmat(shmid, NULL, 0);
    if (info == (void *)-1)
    {
        perror("shmat()");
        return -1;
    }
    memset(info, 0, sizeof(struct login_info));

    //----------------------------------------
    // do monitoring  
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

        sleep(1);
    }

    shmdt(info);

    return 0;
}

static int do_login(void)
{
    struct login_info local;
    struct login_info *info;
    int n; int shmid;
    key_t key;
    size_t size;


    //----------------------------------------
    // connect shm
    key = ftok(SHMKEY_FILEPATH, ShMKEY_PROJID);
    if (key == -1)
    {
        perror("ftok()");
        return -1;
    }

    size = ROUNDUP(sizeof(struct login_info));
    shmid = shmget(key, size, 0644);
    if (shmid == -1)
    {
        perror("shmget()");
        return -1;
    }

    info = shmat(shmid, NULL, 0);
    if (info == (void *)-1)
    {
        perror("shmat()");
        return -1;
    }

    info->pid = getpid();
    info->counter++;

    shmdt(info);

    return 0;
}

int main(int argc, char **argv)
{
    int monitor = 0;

    if (argc >= 2 && !strcmp(argv[1], "monitor"))
    {
        monitor = 1;
    }

    if (monitor)
    {
        do_monitoring();
    }
    else 
    {
        do_login();
    }

    return 0;
}
