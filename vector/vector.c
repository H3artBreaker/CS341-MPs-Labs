/**
 * vector
 * CS 341 - Spring 2024
 */
#include "vector.h"
#include <assert.h>
#include <stdio.h>

struct vector {
    /* The function callback for the user to define the way they want to copy
     * elements */
    copy_constructor_type copy_constructor;

    /* The function callback for the user to define the way they want to destroy
     * elements */
    destructor_type destructor;

    /* The function callback for the user to define the way they a default
     * element to be constructed */
    default_constructor_type default_constructor;

    /* Void pointer to the beginning of an array of void pointers to arbitrary
     * data. */
    void **array;

    /**
     * The number of elements in the vector.
     * This is the number of actual objects held in the vector,
     * which is not necessarily equal to its capacity.
     */
    size_t size;

    /**
     * The size of the storage space currently allocated for the vector,
     * expressed in terms of elements.
     */
    size_t capacity;
};

/**
 * IMPLEMENTATION DETAILS
 *
 * The following is documented only in the .c file of vector,
 * since it is implementation specfic and does not concern the user:
 *
 * This vector is defined by the struct above.
 * The struct is complete as is and does not need any modifications.
 *
 * The only conditions of automatic reallocation is that
 * they should happen logarithmically compared to the growth of the size of the
 * vector inorder to achieve amortized constant time complexity for appending to
 * the vector.
 *
 * For our implementation automatic reallocation happens when -and only when-
 * adding to the vector makes its new  size surpass its current vector capacity
 * OR when the user calls on vector_reserve().
 * When this happens the new capacity will be whatever power of the
 * 'GROWTH_FACTOR' greater than or equal to the target capacity.
 * In the case when the new size exceeds the current capacity the target
 * capacity is the new size.
 * In the case when the user calls vector_reserve(n) the target capacity is 'n'
 * itself.
 * We have provided get_new_capacity() to help make this less ambigious.
 */

static size_t get_new_capacity(size_t target) {
    /**
     * This function works according to 'automatic reallocation'.
     * Start at 1 and keep multiplying by the GROWTH_FACTOR untl
     * you have exceeded or met your target capacity.
     */
    size_t new_capacity = 1;
    while (new_capacity < target) {
        new_capacity *= GROWTH_FACTOR;
    }
    return new_capacity;
}

vector *vector_create(copy_constructor_type copy_constructor,
                      destructor_type destructor,
                      default_constructor_type default_constructor) {
    // your code here
    // Casting to void to remove complier error. Remove this line when you are
    // ready.
    vector* vec = (vector*) malloc(sizeof(vector));
    if (vec == NULL) {
        return NULL;
    }
    vec->copy_constructor = copy_constructor ? copy_constructor : shallow_copy_constructor;
    vec->destructor = destructor ? destructor : shallow_destructor;
    vec->default_constructor = default_constructor? default_constructor : shallow_default_constructor;
    vec->size = 0;
    vec->capacity = INITIAL_CAPACITY;
    vec->array = (void**) calloc(INITIAL_CAPACITY, sizeof(void*));
    
    return vec;
}

void vector_destroy(vector *this) {
    assert(this);
    for (size_t i = 0; i < this->size; i++) {
        (this->destructor)((this->array)[i]);
        this->array[i] = NULL;
    }

    free(this->array);
    this->array = NULL;
    this->capacity = 0;
    this->size = 0;
    
    free(this);
    this = NULL;

}

void **vector_begin(vector *this) {
    return this->array + 0;
}

void **vector_end(vector *this) {
    return this->array + this->size;
}

size_t vector_size(vector *this) {
    assert(this);
    return this->size;
}

void vector_resize(vector *this, size_t n) {
    assert(this);

    if (n > this->size) {
        if (n > this->capacity) {
            this->capacity = get_new_capacity(n);
            this->array = realloc(this->array, this->capacity * sizeof(void*));
            if (this == NULL) {
                return;
            }
        } 
        for (size_t i = this->size; i < n; i++) {
            (this->array)[i] = (this->default_constructor)();
        }
        this->size = n;
    } else if (n < this->size) {
        for (size_t i = n; i < this->size; i++) { // 1 2 3 4 5
            (this->destructor)((this->array)[i]);
            (this->array)[i] = NULL; // do I need to do it?
        }
        this->size = n;
    }
}

size_t vector_capacity(vector *this) {
    assert(this);
    return this->capacity;
}

bool vector_empty(vector *this) {
    assert(this);
    return (this->size == 0);
}

void vector_reserve(vector *this, size_t n) {
    assert(this);
    if (n > this->capacity) {
        this->capacity = get_new_capacity(n);
        this->array = (void**) realloc(this->array, this->capacity * sizeof(void*));      //my question is when we are talking about arrays, if the length is 6, we allocate 5 elements + 1 \0; 
                                                   // Therefore, in initializing it with the default value, why we go to the size rather than size - 1 in condition? 
                                                    //Do we not need to manually add th \0 to be the last element? In that case, do we just reallocate the capacity but not + 1?
    }
}  

void **vector_at(vector *this, size_t position) {
    assert(this);
    assert(position < this->size);

    return &(this->array[position]);
}

void vector_set(vector *this, size_t position, void *element) {
    assert(this);
    assert(position < this->size);

    this->destructor(this->array[position]);
    this->array[position] = this->copy_constructor(element);
}

void *vector_get(vector *this, size_t position) {
    assert(this);
    assert(position < this->size);

    return this->array[position];
}

void **vector_front(vector *this) {
    assert(this);
    
    return this->array;
}

void **vector_back(vector *this) {
    assert(this);

    if (this->size > 0) {
        return &(this->array[this->size - 1]);
    }
    
    return NULL;
}

void vector_push_back(vector *this, void *element) {
    assert(this);
    
    this->size++;
    if (this->size > this->capacity) {
        this->capacity = get_new_capacity(this->capacity + 1);
        this->array = realloc(this->array, this->capacity * sizeof(void*));
    }
    this->array[this->size - 1] = this->copy_constructor(element);
}

void vector_pop_back(vector *this) {
    assert(this);
    
    if (this->size > 0) {
        this->destructor(this->array[this->size - 1]);
        this->size--;
    }

}

void vector_insert(vector *this, size_t position, void *element) {
    assert(this);
    //assert(position <= this->size);

    this->size++;
    if (this->size > this->capacity) {
        this->capacity = get_new_capacity(this->capacity + 1);
        this->array = realloc(this->array, this->capacity * sizeof(void*));
    }

    for (size_t i = this->size - 1; i > position; i--) {
        this->array[i] = this->array[i - 1];
    }
    this->array[position] = this->copy_constructor(element);
}

void vector_erase(vector *this, size_t position) {
    assert(this);
    assert(position < vector_size(this));
    // your code here
    if (position >= 0) {
        this->destructor(this->array[position]);
        for (size_t i = position; i < this->size - 1; i++) { // -1?
            this->array[i] = this->array[i + 1];
        }
        this->size--;
    }
}

void vector_clear(vector *this) {
    assert(this);

    for (size_t i = 0; i < this->size; i++) {
        (this->destructor)((this->array)[i]);
        this->array[i] = NULL;
    }

    this->size = 0;
    
}
