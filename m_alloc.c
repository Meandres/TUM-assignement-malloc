#include "m_alloc.h"
#include "mem_blocks_types.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>


void *heap_bottom;
mem_block_t *first_block=NULL;
long int BASE_MAGIC;
int const MEM_ALIGNEMENT = 1;
// we align the blocks on boundaries to respect possible memory alignement constraints.
size_t const SIZEOF_BLOCK = sizeof(mem_block_t) % MEM_ALIGNEMENT == 0 ? sizeof(mem_block_t) : sizeof(mem_block_t) + (MEM_ALIGNEMENT - sizeof(used_block_t) % MEM_ALIGNEMENT);
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

mem_block_t* get_next_free(mem_block_t *origin){
    mem_block_t *current_block=origin;
    while(current_block!=NULL && current_block->type==USED){
        current_block = current_block->next;
    }
    return current_block;
}

mem_block_t* get_first_free(size_t size){
    mem_block_t *current_block=first_block;
    while(current_block!=NULL && (current_block->type==USED || current_block->size < size)){
        current_block=current_block->next;
    }
    if(current_block == first_block &&  (first_block==NULL || first_block->type==USED || current_block->size < size)){
        return NULL;
    }
    return current_block;
}

mem_block_t *get_last_block(){
    mem_block_t *current_block=first_block;
    while(current_block != NULL && current_block->next!=NULL){
        current_block=current_block->next;
    }
    return current_block; 
}

void print_error(char* error_str){
    printf("Error : %s\n", error_str);
}
void print_alloc_error(char *error_str, size_t size){
    printf("Error during allocation : %s for size %lu\n", error_str, size);
}
void print_free_error(char *error_str, void* p){
    printf("Error during free : %s at %p\n", error_str, p);
}
void print_memory_state(){
    mem_block_t *current_block=first_block;
    if(first_block==NULL){
        printf("No memory used\n");
        return;
    }
    while(current_block!=NULL){
        if(current_block->type==FREE){
            printf("Free block @ %p (size %li)\t", current_block, current_block->size);
        }else if(current_block->type==USED){
            printf("Used block @ %p (size %li)\t", current_block, current_block->size);
        }else{
            printf("No kind of block\t");
        }

        current_block=current_block->next;
    }
    printf("\n\n");
}

int init(){
    heap_bottom = sbrk(0);
    if(heap_bottom == (void*)-1){
        print_error("Init");
        return -1;
    }
    BASE_MAGIC = time(NULL);
    return 0;
}
void *m_alloc(size_t requested_size){
    pthread_mutex_lock(&mutex);
    size_t total_block_size = requested_size + SIZEOF_BLOCK;
    int missaligned_by = 0; // align the end of the block so that the next one will be aligned too.
    if(total_block_size % MEM_ALIGNEMENT != 0){
        missaligned_by = (MEM_ALIGNEMENT - total_block_size % MEM_ALIGNEMENT);
    }
    total_block_size = total_block_size + missaligned_by;
    pid_t tid = gettid();
    printf("Thread %i -- Requested_size : %li\n", tid, requested_size);

    mem_block_t *current_block=first_block, *pred=first_block;
    void *allocated_block;
    
    current_block = get_first_free(total_block_size);
    
    if(current_block == NULL){
        if(first_block!=NULL && first_block->type==FREE){ // the first block is free and can be extended
            current_block = sbrk(total_block_size - first_block->size);
            first_block->size = total_block_size;
            current_block=first_block;
        }else{ // we are adding a new block 
            current_block = sbrk(total_block_size);
            if(current_block == (void*) -1){
                print_alloc_error("sbrk error", requested_size);
                return NULL;
            }

            current_block->next=NULL;
            current_block->size=total_block_size;
            mem_block_t *last_block=get_last_block();
            current_block->pred=last_block;
            if(last_block != NULL){
                last_block->next=current_block;
            }
            
            if(first_block==NULL){ // very first allocation
                first_block=current_block;
                first_block->pred=NULL;
            }
        }
    }else{
        if(current_block->size - total_block_size > SIZEOF_BLOCK){ // we can split the block.
            mem_block_t *slice = (void*) current_block+total_block_size;
            slice->next=current_block->next;
            slice->pred=current_block;
            current_block->next=slice;
            slice->size = current_block->size - total_block_size;
            current_block->size = total_block_size;
        }
    }
    allocated_block = (void*)current_block+SIZEOF_BLOCK;
    current_block->type=USED;
    current_block->magic = (long int) allocated_block ^ BASE_MAGIC;
    printf("Thread %i -- allocated block @ %p\n", tid, current_block);
    print_memory_state();
    pthread_mutex_unlock(&mutex);
    return allocated_block;
}

int pointer_is_correct(mem_block_t *b){
    if(b->type==FREE){
        return 2; //double free
    }
    if((void*)first_block!=(void*)b){
        mem_block_t *current_block=first_block;
        while(current_block->next!=NULL && current_block->next!=b){
            current_block=current_block->next;
        }
        if (current_block->next==NULL){ // We reached the end of the list                  
            return 3; // illegal address in the middle of a block
        }
    }
    if(b->magic != (BASE_MAGIC ^ ((long)b+SIZEOF_BLOCK))){ // checks if magic number is correct
        return 4;
    }
    return 0;
}

int m_free(void *p){
    pthread_mutex_lock(&mutex);
    if(p==NULL){
        print_free_error("null pointer", p);
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    int error=0;
    mem_block_t *allocated_block=p-SIZEOF_BLOCK;
    pid_t tid = gettid();
    printf("Thread %i -- Freeing %p (block at %p)\n", tid, p, allocated_block);
    error = pointer_is_correct(allocated_block);
    if(error!=0){
        print_free_error("pointer error", p);
        printf("error : %i\n", error);
        pthread_mutex_unlock(&mutex);
        return -error;
    }
    allocated_block->type=FREE;
    if(allocated_block->next!=NULL && allocated_block->next->type == FREE){ // coalescing with the block after
        allocated_block->size=allocated_block->size+allocated_block->next->size;
        if(allocated_block->next->next != NULL){
            allocated_block->next->next->pred=allocated_block;
        }
        allocated_block->next = allocated_block->next->next;
    }
    if(allocated_block->pred!=NULL && allocated_block->pred->type==FREE){ // coalescing with the block before
        allocated_block->pred->size=allocated_block->pred->size+allocated_block->size;
        if(allocated_block->next != NULL){
            allocated_block->next->pred=allocated_block->pred;   
        }
        allocated_block->pred->next=allocated_block->next;
    }
    print_memory_state();
    pthread_mutex_unlock(&mutex);
    return 0;
}
