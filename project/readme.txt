File: readme.txt
Author: Jie Zhu
----------------------

1.  For explicit allocator, I represent my header by simply place the block size it can store with 64 bit. If the pointer is in used, I change the last bit of the header to 1
    to indicate the status. Since my header will be 8 bytes, the payload will start at the address of header + 8 bytes. If the header is free (the lsb is 0), the first 8 byte after
    the header represnets the address of present free header, and the second 8 byte represents the address of the next free header. To search for free blocks, 
    I start to search from the first free header and don't stop the loop until I find a large enough free block in the doubly linked list. If I cannot find any available free header, 
    it will return NULL. 

    Weak performance scenario: If the blocks that the user frees doesn't locate contiguous, my allocator will perform weak. Because there is no way to coalesc the free block
    to utilize the heap. e.g a 1 24
                             a 2 24
                             a 3 24
                             a 4 24
                             f 1
                             f 3
    Strong performance scenario: If the freed blocks are contiguous, or the user realloc many block in smaller size, the heap allocator will have strong peroformance because 
    many coalesc happened.

    To optimize my allocator, I initiate local variables to store the calculations which will be used many times. 

2.  1. We assume customer will free the pointer after he finished using it. If he doesn't free the pointer, our heap allocator will have fewer blocks for them to use.
       Although the allocator will not crash, the user will have less block size for other pointers.
    2. We assume customer will not realloc or free the pointer which is not the starting address of the block. Otherwise, it will crash the allocator.


Tell us about your quarter in CS107!
-----------------------------------
Challenge but interesting. Learnt a lot from this course. Thanks all of the teaching team!



