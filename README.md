# Allocator assignement


## Allocation
The allocator follows a policy of first-fit. The allocator keeps a list of all the allocated and free blocks in the heap. During an allocation, it goes over the free blocks and searches for a block that would fit the requested size starting from the start. However if there is no free block big enough to fit the required size, it calls sbrk() to increase the size of the heap.

## De-allocation
The mechanism for freeing checks where is placed the block to free. If possible it will coalesce it with neighboring free blocks.
I did not implement a decrement of the memory region using sbrk() with a negative integer because I think it would simply slow the application while the memory recovered would be minimal. Thanks to the coalescing mechanism, subsequent calls will reuse the already increased heap thus avoiding some yo-yo effect of growing the heap up and down everytime we allocate a new object.
