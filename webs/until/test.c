#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>

int main(void)
{
    pid_t pid;
    sem_t *sem_id = NULL;

    sem_id = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    sem_init(sem_id, 1, 1);

    pid = fork();

    if (pid > 0)
    {
        while (1)
        {
            sem_wait(sem_id);
            printf("Parent process.\n");
            sleep(1);
        }
    }
    else if (pid == 0)
    {
        while (1)
        {
            printf("Chiled process.\n");
            sem_post(sem_id);
            sleep(5);
        }
    }
    else
        perror("fork.");

    return 0;
}
