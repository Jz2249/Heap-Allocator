#include "./allocator.h"
#include "./debug_break.h"
#include <stdio.h>
#include <string.h>

#define RM_1BIT 0xFFFFFFFFFFFFFFF8

static char *segment_start;
static char *segment_end;
static size_t segment_size;
static size_t num_header;

size_t in_used(size_t sz);
void insert_size(void *ptr, size_t sz);
bool is_free(void *ptr);
size_t roundup(size_t sz, size_t mult);
size_t get_sz(char *n);
char *right_move(char *ptr);
char *left_move(char *ptr);

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
        segment_size = heap_size;
        num_header = 1;
        segment_end = heap_start;
        insert_size(segment_start, heap_size - ALIGNMENT);
        return true;
    }
    return false;
}

void *mymalloc(size_t requested_size) {
    // TODO(you!): remove the line below and implement this!
    size_t needed = roundup(requested_size, ALIGNMENT);
    char *header = segment_start;
    int n = 0;
    while (header <= segment_end) {
        n++; // track the number of header;
        size_t blk_size = get_sz(header); // change the lsb to 0 if it is 1
        if (!is_free(header) || blk_size < needed) {
            header += ALIGNMENT + blk_size;
            continue;
        }
        *(size_t *)header = in_used(blk_size);
        // find the correct position, check if it is the last header
        if (n == num_header) {
            *(size_t *)header = in_used(needed);
            segment_end = header + ALIGNMENT + needed;
            char *end_blk = left_move(segment_start + segment_size);
            size_t end_sz = end_blk - header;
            if (segment_end == end_blk) { // no more space
                *(size_t *)header = in_used(end_sz);
            }
            // has more space for the next header
            insert_size(segment_end, end_sz);
            num_header++;
        }
        void *curr = right_move(header);
        return curr;
    }
    return NULL;
}



void myfree(void *ptr) {
    // TODO(you!): implement this!
    char *header = left_move(ptr);
    if (ptr == NULL || is_free(header)) {
        return;
    }
    size_t blk_size = get_sz(header);
    insert_size(header, blk_size);
}

void *myrealloc(void *old_ptr, size_t new_size) {
    // TODO(you!): remove the line below and implement this!
    size_t needed = roundup(new_size, ALIGNMENT);
    void *new_ptr = mymalloc(needed);
    if (new_ptr != NULL && old_ptr != NULL) {
        void *old_header = left_move(old_ptr);
        size_t old_size = get_sz(old_header);
        if (old_size <= needed) {
            memcpy(new_ptr, old_ptr, old_size);
        }
        else {
            memcpy(new_ptr, old_ptr, needed);
        }
        void *new_header = left_move(old_ptr);
        *(size_t *)new_header |= 0x1;
        myfree(old_ptr);
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
    if (segment_end > segment_start + segment_size - ALIGNMENT) {
        breakpoint();
        printf("segment_end exceeds its largest position");
        return false;
    }
    size_t n = 1;
    char *temp = segment_start;
    while (n < num_header) {
        size_t sz = get_sz(temp);
        temp += ALIGNMENT + sz;
        n++;
    }
    if (temp != segment_end) {
        breakpoint();
        printf("num_header doesn't count correctly");
        return false;
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
    segment_start, segment_start + segment_size);
    char *temp = segment_start;
    while (temp != NULL) {
        size_t blksz = get_sz(temp);
        int signal = *(size_t *)temp & 0x1;
        printf("\n%p %zu %i", temp, *(size_t *)temp, signal);
        if (temp + blksz + ALIGNMENT > segment_start + segment_size - 2 * ALIGNMENT) break;
        temp += blksz + ALIGNMENT;
    }
}

char *right_move(char *ptr) {
    return ptr + ALIGNMENT;
}

char *left_move(char *ptr) {
    return ptr - ALIGNMENT;
}
/* return the size with used flag
 */
size_t in_used(size_t sz) {
    return sz | 0x1;
}

void insert_size(void *ptr, size_t sz) {
    *(size_t *)ptr = sz;
}
/* Check the header to see if it is freed
 */
bool is_free(void *ptr) {
    int status = (*(size_t *)ptr) & 0x1;
    if (status == 0) return true;
    return false;
}

size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

size_t get_sz(char *n) {
    size_t size = (*(size_t *)n) & RM_1BIT;
    return size;
}