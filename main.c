#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "m_alloc.h"

void *thread_func(void *param){
    int error;
    void *p1, *p2, *p3, *p4;
    p1=m_alloc(16);
    
    error = m_free(p1);
    if(error != 0)
        printf("Error %i\n", error);
    
    p2=m_alloc(55);
    
    p3=m_alloc(128);
    
    error=m_free(p2);
    if(error != 0)
        printf("Error %i\n", error);
    
    p4=m_alloc(40);
    
    error=m_free(p4);
    if(error != 0)
        printf("Error %i\n", error);
    
    error=m_free(p3);
    if(error != 0)
        printf("Error %i\n", error);
    return 0;
}

int main(){
    int const NUM_THREADS = 4;
    pthread_t threads[NUM_THREADS];
    int error = init();
    if(error!=0){
        printf("Error during init\n");
        return -1;
    }

    for(int i=0; i<NUM_THREADS; i++){
        pthread_create(&threads[i], NULL, thread_func, NULL);
    }

    for(int i=0; i<NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }

    return 0;
}
