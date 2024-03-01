/**
 * malloc
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAGIC_DEBUG 0
typedef struct _meta_data {
    // Number of bytes of heap memory the user requested from malloc
    size_t request_size;

    int is_allocated; //0 is free, 1 is allocated
    
    // Pointer to the previous instance of meta_data in the list
    struct _meta_data *prev;

    // Pointer to the next instance of meta_data in the list
    struct _meta_data *next;


#if MAGIC_DEBUG
    int magic;
#endif

} meta_data;

#define METATAG_SIZE  sizeof(meta_data) 

static size_t currently_freed = 0;
static meta_data* head = NULL;
static meta_data* tail = NULL;

void coalesce(meta_data* right) {
    //set the new_size 
    meta_data* left = right->prev;
    size_t new_size = left->request_size + METATAG_SIZE + right->request_size;

    // coalesce two blocks
    left->request_size = new_size;
    left->next = right->next;
    if (right->next != NULL) {
        right->next->prev = left;
    } else {
        tail = left;
    }
}


void split(meta_data* metaTag, size_t size) {
    size_t rmn_size = metaTag->request_size - size - METATAG_SIZE;
    //set the free part
    currently_freed += rmn_size;

    //set rmn
    meta_data* rmn_metaTag = (meta_data*)((char*)metaTag + METATAG_SIZE + size);
    rmn_metaTag->request_size = rmn_size;
    rmn_metaTag->is_allocated = 0;

    //set new metaTag
    metaTag->request_size = size;

    //set relation
    rmn_metaTag->prev = metaTag;
    rmn_metaTag->next = metaTag->next;

    metaTag->next = rmn_metaTag;
    
    if (rmn_metaTag->next != NULL) {
        rmn_metaTag->next->prev = rmn_metaTag;
    } else {
        tail = rmn_metaTag;
    }


    if (rmn_metaTag->next && rmn_metaTag->next->is_allocated == 0) {
        currently_freed += METATAG_SIZE;
        coalesce(rmn_metaTag->next);
    }

    
}

void *malloc(size_t size) {
    meta_data* block = NULL;
    if (currently_freed >= size) {
        meta_data* curr = head;
        while (curr) {
            if (size <= curr->request_size && curr->is_allocated == 0) {
                if (size == curr->request_size) {
                    currently_freed -= curr->request_size;
                    curr->is_allocated = 1;
                    return (void*)(curr + 1);
                }
                //when size < curr->request_size
                if (curr->request_size - size > METATAG_SIZE && curr->request_size - size - METATAG_SIZE >= 4096) { //can split into 2 //do I need to consider the situation if they are the same?
                    //currently_freed = currently_freed - size - METATAG_SIZE;
                    //currently_freed -= METATAG_SIZE;
                    currently_freed -= curr->request_size;
                    block = curr;
                    split(block, size);
                    break;
                }
            }
            curr = curr->next;
        }
    }

    if (block != NULL) { //found block
        block->is_allocated = 1;
        return (void*)(block + 1);
    }


    if (sbrk(0) == (void*) -1) {
        write(2, "ERROR IN SBRK IN MALLOC!", strlen("ERROR IN SBRK IN MALLOC!"));
        return NULL;
    }
    block = sbrk(0); //get current break(the location closest to the end of the heap)
    if (sbrk(METATAG_SIZE + size) == (void*) -1) { //push the break forward if successes.
        write(2, "ERROR IN SBRK IN MALLOC!", strlen("ERROR IN SBRK IN MALLOC!"));
        return NULL;
    }
    block->request_size = size;
    block->is_allocated = 1;
    block->prev = NULL;
    block->next = NULL;
    //block->magic = 0x3333;

    if (head == NULL) {
        head = block;
        tail = head;
    } else {
        tail->next = block;
        block->prev = tail;
        tail = block;
    }

    return tail + 1;
}


void free(void *ptr) {
    // implement free!
    if (ptr == NULL) {
        return;
    }
    meta_data* metaTag = (meta_data*)ptr - 1;
    if (metaTag->is_allocated == 0) {
        return;
    }
    metaTag->is_allocated = 0;
    currently_freed += metaTag->request_size;

    //combine the block if prev is free too
    if (metaTag->prev != NULL && (metaTag->prev)->is_allocated == 0) {
        currently_freed = currently_freed + METATAG_SIZE;
        coalesce(metaTag);
    }
    if (metaTag->next != NULL && (metaTag->next)->is_allocated == 0) {
        currently_freed = currently_freed + METATAG_SIZE;
        coalesce(metaTag->next);
    }
}




/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    void* cal = malloc(num * size);
    if (cal == NULL) {
        return NULL;
    }
    memset(cal, 0, num * size);
    return cal;
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    // implement realloc!
    if (ptr == NULL) {
        return malloc(size);
    }

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    meta_data* metaTag = (meta_data*)ptr - 1;
    size_t new_size = size;
    size_t old_size = metaTag->request_size;
    
    if (new_size == old_size) {
        return ptr;
    }

    if (new_size > old_size) {
        // if coalese has the same size
        if (metaTag->next != NULL && metaTag->next->is_allocated == 0 
            && metaTag->next->request_size + METATAG_SIZE + metaTag->request_size == new_size) {
            
            meta_data* right = metaTag->next;
            currently_freed += METATAG_SIZE;
            coalesce(right);
            return metaTag + 1;
        }
        if (metaTag->next != NULL && metaTag->next->is_allocated == 0 
            && metaTag->next->request_size + METATAG_SIZE + metaTag->request_size > new_size) {
            if (metaTag->next->request_size + METATAG_SIZE + metaTag->request_size - new_size > METATAG_SIZE) {
                meta_data* right = metaTag->next;
                currently_freed += METATAG_SIZE;
                coalesce(right);
                currently_freed = currently_freed - right->request_size;
                split(metaTag, new_size);
                return metaTag + 1;
            }
        } 

        void* bigger = malloc(new_size);
        memcpy(bigger, ptr, old_size);
        free(ptr);
        return bigger;
    }

    if (new_size < old_size) {
        if (metaTag->request_size - new_size >= METATAG_SIZE) {
            //currently_freed += (old_size - new_size); // did that in split
            split(metaTag, new_size);
            return metaTag + 1;
        } else {
            void* smaller = malloc(size);
            memcpy(smaller, ptr, new_size);
            free(ptr);
            return smaller;
        } 
        // void* smaller = malloc(size);
        // memcpy(smaller, ptr, new_size);
        // free(ptr);
        // return smaller;
    }

    return NULL;
}
