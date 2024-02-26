/**
 * malloc
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAGIC_DEBUG 1
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

typedef struct _boundary_tag {
    // Number of bytes of heap memory the user requested from malloc
    size_t request_size;

    int is_allocated; //0 is free, 1 is allocated

#if MAGIC_DEBUG
    int magic;
#endif

} boundary_tag;

#define BTAG_SIZE  sizeof(boundary_tag) 
#define METATAG_SIZE  sizeof(meta_data) 

static size_t currently_freed;
static meta_data* head = NULL;


// void split(meta_data* metaTag, size_t size) {
//     meta_data* rmn_metaTag = (meta_data*)((char*)metaTag + METATAG_SIZE + size + BTAG_SIZE);
//     size_t rmn_size = metaTag->request_size - size - BTAG_SIZE - METATAG_SIZE;
//     rmn_metaTag->request_size = rmn_size;
//     rmn_metaTag->is_allocated = 0;
//     rmn_metaTag->magic = 0x1111;
//     //set remaining's boundary tag
//     boundary_tag* rmn_btag = (boundary_tag*)((char*)rmn_metaTag + METATAG_SIZE + rmn_size);
//     rmn_btag->request_size = rmn_size;
//     rmn_btag->is_allocated = 0;

//     metaTag->request_size = size;
//     metaTag->is_allocated = 0;
//     metaTag->next = rmn_metaTag;
//     metaTag->magic = 0x2222;
//     //set new btag
//     boundary_tag* new_btag = (boundary_tag*)((char*)metaTag + METATAG_SIZE + size);
//     new_btag->request_size = size;
//     new_btag->is_allocated = 0;

//     //set relation
//     rmn_metaTag->prev = metaTag;
//     rmn_metaTag->next = metaTag->next;

//     metaTag->next = rmn_metaTag;
    
//     if (rmn_metaTag->next != NULL) {
//         rmn_metaTag->next->prev = rmn_metaTag;
//     }


    
// }



/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */

void *malloc(size_t size) {
    meta_data* block = NULL;
    if (currently_freed >= size) {
        meta_data* curr = head;
        while (curr) {
            if (size <= curr->request_size && curr->is_allocated == 0) {
                if (size <= curr->request_size) {
                    currently_freed -= curr->request_size;
                    curr->is_allocated = 1;
                    boundary_tag* curr_btag = (boundary_tag*)((char*)(curr + 1) + curr->request_size);
                    //curr_btag->request_size = size;
                    curr_btag->is_allocated = 1;
                    return (void*)(curr + 1);
                }

                //for part 1
                // if (size < curr->request_size) {
                //     currently_freed -= size;
                //     curr->is_allocated = 1;
                //     boundary_tag* curr_btag = (boundary_tag*)((char*)(curr + 1) + curr->request_size);

                // }
                // if (curr->request_size - size > BTAG_SIZE + METATAG_SIZE) { //can split into 2 //do I need to consider the situation if they are the same?
                //     currently_freed -= size;
                //     block = curr;
                //     split(block, size);
                //     break;
                // }
            }
            curr = curr->next;
        }
    }

    if (block != NULL) { //found block
        block->is_allocated = 1;
        boundary_tag* btag = (boundary_tag*)((char*)(block + 1) + size);
        btag->is_allocated = 1;
        btag->request_size = size;
        return (void*)(block + 1);
    }


    if (sbrk(0) == (void*) -1) {
        perror("ERROR IN SBRK IN MALLOC!");
        return NULL;
    }
    block = sbrk(0); //get current break(the location closest to the end of the heap)
    if (sbrk(METATAG_SIZE + size + BTAG_SIZE) == (void*) -1) { //push the break forward if successes.
        perror("ERROR IN SBRK IN MALLOC!");
        return NULL;
    }
    block->request_size = size;
    block->is_allocated = 1;
    block->prev = NULL;
    block->next = NULL;
    block->magic = 0x3333;

    //set btag
    boundary_tag* btag = (boundary_tag*)((char*)(block + 1) + size);
    btag->request_size = size;
    btag->is_allocated = 1; 
    if (head == NULL) {
        head = block;
    } else {
        block->next = head;
        head->prev = block;
        head = block;
    }

    return (void*)(head + 1);
}

// void coalesce(meta_data* prev, meta_data* behind) {
//     //set the new_size 
//     size_t new_size = prev->request_size + BTAG_SIZE + METATAG_SIZE + behind->request_size;
//     //change the behind btag info
//     boundary_tag* beh_btag = (boundary_tag*)((char*)behind + METATAG_SIZE + behind->request_size);
//     beh_btag->is_allocated = 0;
//     beh_btag->request_size = new_size;
//     //printf("WOOWOW");

//     //coalesce two blocks
//     prev->request_size = new_size;
//     prev->next = behind->next;
//     if (behind->next != NULL) {
//         behind->next->prev = prev;
//     }
//     prev->is_allocated = 0;

// }




/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    // implement free!
    if (ptr == NULL) {
        return;
    }
    meta_data* metaTag = (meta_data*)ptr - 1;
    boundary_tag* btag = (boundary_tag*)((char*)metaTag + METATAG_SIZE + metaTag->request_size);
    // if (metaTag->is_allocated != btag->is_allocated) {
    //     perror("THERE IS A OVERFLOW!!!");
    //     return;
    // }
    if (metaTag->is_allocated == 0 || btag->is_allocated == 0) {
        return;
    }
    metaTag->is_allocated = 0;
    currently_freed += metaTag->request_size;

    //for part1
    btag->is_allocated = 0;

    //combine the block if prev is free too
    // if (metaTag->prev != NULL && (metaTag->prev)->is_allocated == 0) {
    //     currently_freed = currently_freed + BTAG_SIZE + METATAG_SIZE;
    //     coalesce(metaTag->prev, metaTag);
    //     //coalesce(metaTag);
    // }
    // if (metaTag->next != NULL && (metaTag->next)->is_allocated == 0) {
    //     currently_freed = currently_freed + BTAG_SIZE + METATAG_SIZE;
    //     coalesce(metaTag, metaTag->next);
    //     //coalesce(metaTag->next);
    // }
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
    // implement calloc!
    return NULL;
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
        void* bigger = malloc(size);
        memcpy(bigger, ptr, metaTag->request_size);
        free(ptr);
        return bigger;
    }

    if (new_size < old_size) {
        void* smaller = malloc(size);
        memcpy(smaller, ptr, new_size);
        free(ptr);
        return smaller;
    }



    
    return NULL;
}
