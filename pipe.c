#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char** argv)
{
    int pipe_fds[2]; // fd[0] reader, fd[1] writer
    pid_t pid;
    char buf[1024];
    int wstatus;
    
    memset(buf, 0, sizeof(buf));

    // pipe success return 0, error return else
    if (pipe(pipe_fds)) {
        perror("pipe()");
        return -1;
    }

    pid = fork();

    if (pid == 0)
    {
        /* child process */
        close(pipe_fds[1]); // close writer

        // child proc as reader(consumer)
        
        read(pipe_fds[0], buf, sizeof(buf));
        printf("child proc got : %s \n", buf);
        close(pipe_fds[0]);


    }
    else if (pid > 0)
    {
        /* parent process */
        close(pipe_fds[0]); // close reader

        // parent proc as writer(producer)
        
        strncpy(buf, "Hello from parent proc", sizeof(buf) - 1);
        write(pipe_fds[1], buf, strlen(buf));
        close(pipe_fds[1]);

        pid = wait(&wstatus); // blocking. wait child proc exit
    }
    else 
    {
        perror("fork()");
        goto err;
    }

    return 0;

err:
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return -1;
}
