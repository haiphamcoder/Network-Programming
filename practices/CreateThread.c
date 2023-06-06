#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *thread_proc(void *);

int main(){
    pthread_t thread_id;
    if(pthread_create(&thread_id, NULL, thread_proc, NULL) != 0){
        printf("pthread_create() failed\n");
        return 1;
    }
    getchar();
    printf("Main thread done.\n");
    return 0;
}

void *thread_proc(void *param){
    printf("Child thread start\n");
    for(int i = 0; i < 10; i++){
        printf("Child thread running\n");
        sleep(1);
    }
    printf("Child thread done\n");
    return NULL;
}