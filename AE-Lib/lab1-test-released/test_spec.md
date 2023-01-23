
### T110 basic memory management test for RAM1 and RAM2
* Description: This test checks that the kernel can allocate and deallocate memory without any corruption to the written data

This testcase execute the following steps:

* Allocating the entire size of the RAM
* writing 32B of data with different offsets (start,middle,end) to the allocated memory blocks
* Deallocating all memory blocks and check for the dump value
* Execute a series of allocation and deallocation procedures and recheck for the written values

### T120 Test for Buddy algorithm
* Description: This test checks that the Buddy algorithm is implemented correctly. 

This testcase execute the following steps:

* Allocating/Deallocating full size of RAM
* Allocating block with different fractions of the memory size then deallocating them in different order
* Allocating 16 blocks with 0x100 bytes each , then deallocate them in reversed order
* Allocate 5 different block sizes ( not multiples of 8), then deallocate them in different order


### T141 complex memory management test for RAM1 and RAM2
* Description: This test checks the kernel can handle corner cases for allocation and deallocation.  

This testcase execute the following steps:

* Allocating/Deallocating two blocks with a size of 1B
* Write 1B to both blocks
* Check for the data written
* Deallocate both blocks

### T142 complex memory management test for RAM1 and RAM2
* Description: This test checks the kernel allocation and deallocation for a specific scenario. 

This testcase execute the following steps:

* Consume all memory with allocating with size less than MIN_BLK_SIZE
* Deallocate all blocks in memory address order
* Allocate memory with a sequence of different sizes
* Deallocate all blocks
* Allocate memory again with the same sequence


### T143 stress test 
* Description: This test checks whether the kernel will suffer from memory leak 

This testcase execute the following steps:

* Allocate all blocks with different size , deallocate blocks , repeat for 10 times
* Allocate all blocks with different size , write data, deallocate blocks , repeat for 10 times

### T241 Error handling
* Description: This test checks whether the kernel handle all errors by checking for the "errno" value

This testcase execute the following steps:

* Allocating with no available memory
* Deallocate a bad pointer


