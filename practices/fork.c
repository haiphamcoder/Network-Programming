#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

int main()
{
    int cid = fork();
    if (cid == 0) // Tiến trình con
    {
        printf("Child process %d started\n", getpid());
        sleep(5);
        printf("Child process %d finished\n", getpid());
        exit(EXIT_SUCCESS);
    }
    // Tiến trình cha
    printf("Waiting for child %d to finish\n", cid);
    int status;
    int pid = wait(&status);
    printf("Child %d finished with status %d\n", pid, status);
    return 0;
}