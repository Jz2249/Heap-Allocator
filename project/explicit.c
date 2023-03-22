#include "./allocator.h"
#include "./debug_break.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_SIZE 24
#define RM_1BIT 0xFFFFFFFFFFFFFFF8


static char *segment_start;
static char *header_max;
static size_t segment_size;
static char *segment_free; // first free header
static size_t num_header;

size_t roundup(size_t sz, size_t mult);
bool is_free(void *ptr);
char *get_prev(char *ptr);
char *get_next(char *ptr);
void set_prev(char *ptr, char *prev);
void set_next(char *ptr, char *next);
size_t get_sz(char *n); 
void *get_right(void *ptr);
void *get_header(void *ptr);
void rm_free(char *curr, char *curr_next, char *curr_prev); 
void insert_size(void *ptr, size_t sz); 
void add_free(char *curr, size_t sz); 
void coalesc(void *ptr);
void insert_free(void *ptr); 
bool can_coalesc(void *ptr); 
void split(void *ptr, size_t sz, size_t total); 
void *grow(void *ptr, size_t needed); 


bool myinit(void *heap_start, size_t heap_size) {
    /* TODO(you!): remove the line below and implement this!
     * This must be called by a client before making any allocation
     * requests.  The function returns true if initialization was
     * successful, or false otherwise. The myinit function can be
     * called to reset the heap to an empty state. When running
     * against a set of of test scripts, our test harness calls
     * myinit before starting each new script.
     */
    if (heap_size >= MAX_REQUEST_SIZE) {
        segment_start = heap_start;
        *(size_t *)segment_start = heap_size - ALIGNMENT;
        segment_size = heap_size;
        segment_free = segment_start;
        header_max = segment_start + segment_size - MIN_SIZE;
        set_prev(segment_free, NULL);
        set_next(segment_free, NULL);
        num_header = 1;
        return true;
    }
    return false;
}


void *mymalloc(size_t requested_size) {
    // TODO(you!): remove the line below and implement this!
    if (requested_size == 0) return NULL;
    size_t needed = roundup(requested_size, ALIGNMENT);
    size_t new_alignment = ALIGNMENT << 1;
    if (needed < new_alignment) needed = new_alignment;
    char *curr = segment_free;
    int num = 0; // trace the num of head
    while (curr != NULL) {
        num++;
        size_t blksz = get_sz(curr);
        if (blksz < needed) {
            curr = get_next(curr); // go to the next freed head
            continue;
        }
        if (num >= num_header) {
            insert_size(curr, needed);
            add_free(curr, needed); // add free to the right, if it cannot add more header, update the current header
        }
        else { // position is within the previously used block
            char *prev = get_prev(curr);
            char *next = get_next(curr);
            rm_free(curr, next, prev); 
            insert_size(curr, blksz | 0x1);
            num_header--;
        }
        void *cur = curr + ALIGNMENT;
        return cur;
    }
    return NULL;
}



void myfree(void *ptr) {
    // TODO(you!): implement this!
    char *header = get_header(ptr);
    if (ptr == NULL || is_free(header)) return;
    *(size_t *)header &= RM_1BIT;
    insert_free(header);
    num_header++;
    coalesc(header);
  
}


void *myrealloc(void *old_ptr, size_t new_size) {

    if (new_size == 0) return NULL;
    if (old_ptr == NULL) return mymalloc(new_size);
    size_t needed = roundup(new_size,  ALIGNMENT);
    size_t new_alignment = ALIGNMENT << 1;
    if (needed < new_alignment) needed = new_alignment;
    char *old_header = get_header(old_ptr);
    size_t blksz = get_sz(old_header);
    void *new_ptr = NULL;
    void *temp = grow(old_header, needed);
    blksz = get_sz(temp);
    if (blksz >= needed) { // existing block is enough
        new_ptr = (char *)temp + ALIGNMENT;
    }
    else {
        new_ptr = mymalloc(needed);
        memcpy(new_ptr, old_ptr, blksz);
        blksz = get_sz((char *)new_ptr - ALIGNMENT);
        myfree(old_ptr);
    }
    // check if need split
    size_t extra = blksz - needed;
    if (extra >= MIN_SIZE) {
        split((char *)new_ptr - ALIGNMENT, needed, blksz);      
    }
      return new_ptr;

}

bool validate_heap() {
    /* TODO(you!): remove the line below and implement this to
     * check your internal structures!
     * Return true if all is ok, or false otherwise.
     * This function is called periodically by the test
     * harness to check the state of the heap allocator.
     * You can also use the breakpoint() function to stop
     * in the debugger - e.g. if (something_is_wrong) breakpoint();
     */
     if (num_header == 0 && segment_free != NULL) {
        printf("unmmatched number of free header and existing free header ");
        breakpoint();
        return false;
     }
     if (segment_free != NULL && !is_free(segment_free)) {
        printf("free header is not freed");
        breakpoint();
        return false;
     }
     if (segment_start > header_max) { // the largest position for a new header
        printf("last header has incorrect position");
        breakpoint();
        return false;
     }

     size_t n = 1;
     char *temp = get_next(segment_free);
     while (n < num_header) {
        if (temp == NULL) {
            printf("doesn't update the free header and number correctly");
            breakpoint();
            return false;
        }
        temp = get_next(temp);
        n++;
     }

    return true;
}

/* Function: dump_heap
 * -------------------
 * This function prints out the the block contents of the heap.  It is not
 * called anywhere, but is a useful helper function to call from gdb when
 * tracing through programs.  It prints out the total range of the heap, and
 * information about each block within it.
 */
void dump_heap() {
    // TODO(you!): Write this function to help debug your heap.
     printf("Heap segment starts at address %p, ends at %p. ",
        segment_start, header_max + 2 * ALIGNMENT);
    char *temp = segment_start;
    while (temp != NULL) {
        int signal = *(size_t *)temp & 0x1;
        printf("\n%p %zu %i", temp, *(size_t *)temp, signal);
        size_t blksz = get_sz(temp);
        if (temp + blksz + ALIGNMENT >= header_max) break;
        temp = get_right(temp);
    }
}

size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

bool is_free(void *ptr) {
    if ((char *)ptr >= header_max) return false;
    int status = (*(size_t *)ptr) & 0x1;
    if (status == 0) return true;
    return false;
}

char *get_prev(char *ptr) {
    if (ptr == NULL) return NULL;
    char *temp = (char *)*(size_t *)(ptr + ALIGNMENT);
    return temp;
}
char *get_next(char *ptr) {
    if (ptr == NULL) return NULL;
    char *temp = (char *)*(size_t *)(ptr + 2 *ALIGNMENT);
    return temp;
}
void set_prev(char *ptr, char *prev) {
    if (ptr == NULL) return;
    *(size_t *)(ptr + ALIGNMENT) = (size_t)prev;
}
void set_next(char *ptr, char *next) {
    if (ptr == NULL) return;
    *(size_t *)(ptr + 2 * ALIGNMENT) = (size_t)next;
}
size_t get_sz(char *n) {
    size_t size = (*(size_t *)n) & RM_1BIT;
    return size;
}

void *get_header(void *ptr) {
    return (char *)ptr - ALIGNMENT;
}
void *get_right(void *ptr) {
    size_t blksz = get_sz(ptr);
    void *right = (char *)ptr + ALIGNMENT + blksz;
    return right;
}

/*  remove the free header in the linked list
 */
void rm_free(char *curr, char *curr_next, char *curr_prev) {
        set_prev(curr_next, curr_prev);
        set_next(curr_prev, curr_next);
    if (curr == segment_free) {
        segment_free = curr_next;
    }
}

void insert_size(void *ptr, size_t sz) {
    *(size_t *)ptr = sz;
}

/*  add free header or update the current header if no more header can be added
 */
void add_free(char *curr, size_t sz) {
    char *temp = header_max - MIN_SIZE; // the last head that can add a new head to the right
    size_t blksz = get_sz(curr);
    char *new = curr + ALIGNMENT + blksz;
    size_t blk_left = segment_start + segment_size - new - ALIGNMENT;
    char *prev = get_prev(curr);
    char *next = get_next(curr);
    if (curr <= temp) { // can add free new header
        insert_size(new, blk_left);
        rm_free(curr, new, prev);
        set_next(new, NULL);
    }
    else {
        *(size_t *)curr += blk_left;
        rm_free(curr, next, prev);
    }
    insert_size(curr, sz | 0x1);
}

void coalesc(void *ptr) {
    char *right = get_right(ptr);
    if (is_free(right)) {
        size_t right_blksz = get_sz(right);
        *(size_t *)ptr += (right_blksz + ALIGNMENT); 
        char *right_next = get_next(right);
        rm_free(right, right_next, ptr);
        num_header--;
    }
}

/* insert the free header and update segment_free if necessary
 */
void insert_free(void *ptr) {
    char *temp = segment_free;
    void *prev = NULL;
    int i = 1;
    while (i <= num_header) {
        prev = get_prev(temp);
        if (temp > (char *)ptr) { // temp is the next
            set_next(prev, ptr);
            set_prev(temp, ptr);
            break;
        }
        i++;
        prev = temp;
        temp = get_next(temp);
    }
    set_next(prev, ptr);
    set_next(ptr, temp);
    set_prev(ptr, prev); 
    if ((char *)ptr < segment_free || segment_free == NULL) segment_free = ptr;
} 
bool can_coalesc(void *ptr) {
    char *right = get_right(ptr);
    if (is_free(right)) {
        return true;
    }
    return false;
}
/* split the block if it is large enough
 */
void split(void *ptr, size_t sz, size_t total) {
    size_t extra = total - sz - ALIGNMENT;
    char *new_header= (char *)ptr + ALIGNMENT + sz;
    *(size_t *)new_header = extra;
    insert_free(new_header);
    insert_size(ptr, sz | 0x1);
    num_header++;
}

/* coalesc all the free block to the right of ptr or until the block size reaches needed 
 */
void *grow(void *ptr, size_t needed) {
    char *right = get_right(ptr);
    size_t blksz = get_sz(ptr);
    char *tp = right;
    if (is_free(right)) {
        tp = right;
        while (can_coalesc(right)) {
            coalesc(right);
        }
        size_t right_sz = get_sz(tp);
        size_t total = blksz + ALIGNMENT + right_sz;
        char *next = get_next(tp);
        char *prev = get_prev(tp);
        rm_free(tp, next, prev);
        num_header--;
        insert_size(ptr, total | 0x1);
    }
    return ptr;
}