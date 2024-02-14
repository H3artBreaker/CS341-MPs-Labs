/**
 * mini_memcheck
 * CS 341 - Spring 2024
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>
size_t total_memory_requested;
size_t total_memory_freed;
size_t invalid_addresses;
meta_data *head;

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    if (request_size == 0) {  // If the requested size is 0, malloc's behavior is undefined. 
        return NULL;    //In this case, you may just return a NULL pointer.
    }
    meta_data* mm = (meta_data*)malloc(sizeof(meta_data) + request_size); //metadata + user requested size
    if (mm == NULL) {
        return NULL;
    }
    mm->request_size = request_size;
    mm->filename = filename;
    mm->instruction = instruction;
    mm->next = NULL;

    if (head == NULL) {
        head = mm;
    } else {
        mm->next = head;
        head = mm;
    }
    total_memory_requested += request_size;
    return (void*)(mm + 1); 
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    size_t request_size = num_elements * element_size;
    meta_data* mc_at_user_mem = (meta_data*)mini_malloc(request_size, filename, instruction); //already at user-requested memory addr
    if (mc_at_user_mem != NULL) {
        memset(mc_at_user_mem, 0, request_size); 
        return (void*)mc_at_user_mem;
    }
    
    return NULL;
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {   //(What does this mean?? ->)If the pointer is NULL and the size is 0, the behavior is undefined.
    if (payload == NULL) { // If the given pointer is NULL, you should treat this like a call to mini_malloc.
        return mini_malloc(request_size, filename, instruction);
    } 
    if (request_size == 0) {
        mini_free(payload); // if payload is invalid, we increase the invalid_addresses in free; ?
        return NULL;
    }
    setvbuf(stdout, NULL, _IONBF, 0);
    //check if the payload is valid 
    meta_data* curr = head;
    while (curr != NULL) {
        if ((void*)(curr + 1) == payload) {
            break;
        }
        curr = (meta_data*)curr->next;
    }
    if (curr == NULL) {
        invalid_addresses++;
        return NULL;
    }

    //if it is valid
    meta_data* payload_md = (meta_data*)payload - 1;
    meta_data* new_md = realloc(payload_md, request_size + sizeof(meta_data)); //use realloc to resize an existing allocation,  

    if (new_md == NULL) {
        return NULL;
    }
    //?how does reallloc work? does it free in it? 
    //-answer: if reallocation succeeds, it will automatocally free the original blocks;
    //if it fails, it will leave the original block untouched.

    // if request_size is bigger
    if (new_md->request_size < request_size) {
        total_memory_requested = total_memory_requested - new_md->request_size + request_size;
    } else if (new_md->request_size > request_size) {
        total_memory_freed = total_memory_freed + (new_md->request_size - request_size);
    }

    new_md->request_size = request_size;

    return (void*)(new_md + 1);
}

void mini_free(void *payload) {
    if (payload == NULL) {
        return;
    }
    
    //check if it is a valid ptr
    meta_data* curr = head;
    meta_data* prev = NULL;

    while (curr != NULL) {
        if ((void*)(curr + 1) == payload) {
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    if (curr == NULL) {
        invalid_addresses++;
        return;
    }

    
    meta_data* new_next = curr->next;
    if (curr == head) {
        head = new_next;
    } else {
        prev->next = new_next;
    }
    total_memory_freed += curr->request_size;
    free(curr);
}

