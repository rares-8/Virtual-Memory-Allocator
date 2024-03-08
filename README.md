## Virtual Memory Allocator

### Description:

* Virtual Memory Allocator that simulates malloc, calloc, and frees memory areas.
  Implemented using:
    1. doubly linked list of blocks (the arena)
    2. blocks contain linked lists of miniblocks
    3. miniblocks contain permission and buffers for reading and writing

* The project supports the following operations:
    - alloc/dealloc memory for the arena
    - alloc/free blocks
    - read/write/change permissions for miniblocks
    - print information about the arena

* ALLOC_ARENA:
    - initialize the arena

* DEALLOC_ARENA:
    - free all memory used by the program and quit

* ALLOC_BLOCK:
    - create a new miniblock/block inside the arena
    - if the miniblock/block has no neighbors, it creates a new block
    - if miniblock/block has neighbors, then it forms a bigger block

* FREE_BLOCK:
    - free the miniblock at the given address

* READ:
    - print the n bytes of data starting at the given adress
    - print only if permissions allow it

* WRITE
    - write n bytes of data inside the block at the given address

* PMAP
    - print data about all blocks and miniblocks

* MPROTECT
    - set permissions on the miniblocks rw buffers
