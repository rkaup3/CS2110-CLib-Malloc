// Teju Nareddy
// CS 2110 Malloc HW
// 4/27/16

#include "my_malloc.h"

/* You *MUST* use this macro when calling my_sbrk to allocate the
 * appropriate size. Failure to do so may result in an incorrect
 * grading!
 */
#define SBRK_SIZE 2048

/* Please use this value as your canary! */
#define CANARY 0x2110CAFE

/* our freelist structure - this is where the current freelist of
 * blocks will be maintained. failure to maintain the list inside
 * of this structure will result in no credit, as the grader will
 * expect it to be maintained here.
 * Technically this should be declared static for the same reasons
 * as above, but DO NOT CHANGE the way this structure is declared
 * or it will break the autograder.
 */
metadata_t* freelist;

// Static prototypes
static char* splitBlock(char* original, size_t blockSize);
static char* mergeBlocks(char* leftBlock, char* rightBlock);

void* my_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    short blockSize = size + sizeof(metadata_t) + 2 * sizeof(int);
    if (blockSize > SBRK_SIZE) {
        // SINGLE_REQUEST_TOO_LARGE error with the size the user provides
        // Cannot get enough memory for the user in a single block
        ERRNO = SINGLE_REQUEST_TOO_LARGE;
        return NULL;
    }

    // Check for space / the best match in the freelist
    int smallestMatch = 4 * SBRK_SIZE + 4;
    metadata_t* matchNode = NULL;

    // Traverse through the free list
    metadata_t* lastPrev = NULL;
    metadata_t* current = freelist;
    while (current != NULL) {
        short currBlockSize = current->block_size;
        if (currBlockSize >= blockSize && currBlockSize < smallestMatch) {
            // Store the node if it's a best fit
            smallestMatch = currBlockSize;
            matchNode = current;
        }

        // Move to the next node and set all temp variables
        lastPrev = current;
        current = current->next;
    }

    if (matchNode == NULL) {
        // Create a new node if a best match node was not found and add it to the end of the list
        // This means: not enough space in the list OR no nodes in the list
        char* newBlock = (char*) my_sbrk(SBRK_SIZE);
        if (newBlock == NULL) {
            // OUT_OF_MEMORY error with my_sbrk: No more heap space
            ERRNO = OUT_OF_MEMORY;
            return NULL;
        }

        // Set up metadata in the newly created block for the split function
        matchNode = (metadata_t*) newBlock;
        matchNode->block_size = SBRK_SIZE;
        matchNode->request_size = 0;
        matchNode->next = NULL;
        matchNode->prev = lastPrev;

        // Set prev's next pointer to add this to the list
        if (lastPrev != NULL) {
            lastPrev->next = matchNode;
        } else {
            freelist = matchNode;
        }
    }

    // Store the next and prev pointers in the original node for use later
    metadata_t* origMetaData = (metadata_t*) matchNode;
    short origBlockSize = origMetaData->block_size;
    metadata_t* origNext = origMetaData->next;
    metadata_t* origPrev = origMetaData->prev;

    // Pointer to the free space to return to the user
    void* freeSpace = NULL;

    char* matchBlock = (char*) matchNode;
    if (origBlockSize - blockSize < sizeof(metadata_t) + sizeof(int) * 2) {
        // Don't split if the size of the second block would not work out
        freeSpace = (void*)((char*)(origMetaData) + sizeof(metadata_t) + sizeof(int));

        // Set up metadeta
        origMetaData->request_size = size;
        origMetaData->next = NULL;
        origMetaData->prev = NULL;

        // Set up canary #1
        *(int*)((char*)(origMetaData) + sizeof(metadata_t)) = CANARY;

        // Set up canary #2
        *(int*)((char*)(origMetaData) + sizeof(metadata_t) + sizeof(int) + size) = CANARY;

        // Set prev's and next's references to remove this node
        if (origNext != NULL) {
            origNext->prev = origPrev;
        }
        if (origPrev != NULL) {
            origPrev->next = origNext;
        } else {
            freelist = origNext;
        }
    } else {
        // Split the best match node to get the proper user free space and store it to return later
        freeSpace = splitBlock(matchBlock, blockSize);
        origMetaData = (metadata_t*) (matchBlock + blockSize);

        // Set the prev's next pointer to the block
        if (origPrev == NULL) {
            freelist = origMetaData;
        } else {
            origPrev->next = origMetaData;
        }

        // Set next's prev pointer
        if (origNext != NULL) {
            origNext->prev = origMetaData;
        }
    }

    // Set the error status to successful
    ERRNO = NO_ERROR;

    return freeSpace;
}

void my_free(void* ptr) {
    char* blockPtr = (char*) ptr;
    if (*(int*)(blockPtr - sizeof(int)) != CANARY) {
        // Canary #1 is corrupted
        ERRNO = CANARY_CORRUPTED;
        return;
    }

    // Make a pointer to the metadata
    metadata_t* metaData = (metadata_t*) ((char*) ptr - sizeof(int) - sizeof(metadata_t));
    short requestSize = metaData->request_size;
    metaData->request_size = 0;

    blockPtr = (char*) metaData;
    if (*(int*)(blockPtr + sizeof(metadata_t) + sizeof(int) + requestSize) != CANARY) {
        // Canary #2 is corrupted
        ERRNO = CANARY_CORRUPTED;
        return;
    }

    // Move through the list until we get to the proper spot right before the next memory address
    metadata_t* current = freelist;
    metadata_t* prev = NULL;
    while (current != NULL && (char*) current < blockPtr) {
        prev = current;
        current = current->next;
    }

    // Add the node to the current spot (between prev and current)
    metaData = (metadata_t*) blockPtr;
    metaData->prev = prev;
    metaData->next = current;

    // Set the prev's next
    if (prev != NULL) {
        prev->next = metaData;
    } else {
        freelist = metaData;
    }

    // Set the current's prev
    if (current != NULL) {
        current->prev = metaData;
    }

    // Traverse through the freelist looking for consecutive blocks to merge with
    current = freelist;
    while (current != NULL) {

        // Merge a current block with its previous block if the memory addresses align
        if (current->prev != NULL && (char*)(current->prev) + current->prev->block_size == (char*) current) {
            current = (metadata_t*) mergeBlocks((char*) current->prev, (char*) current);
            if (current->next != NULL) {
                // Reset the next's prev pointer to current
                current->next->prev = current;
            }
        } else {
            // Move forward a block
            current = current->next;
        }
    }

    // Set the error status
    ERRNO = NO_ERROR;
}

/*  Splits the original block into two different blocks, and sets up all the proper metadata for each block
 *  @param original: A pointer to the beginning of the original block to split
 *  @param blockSize: The number of bytes to break off the block (not just user free space!)
 *  @return the address to the new user free space
 */
static char* splitBlock(char* original, size_t blockSize) {
    // Get the metadata for the original block and retrieve useful data
    metadata_t* origMetaData = (metadata_t*) original;
    short origSize = origMetaData->block_size;
    metadata_t* next = origMetaData->next;
    metadata_t* prev = origMetaData->prev;

    // === Create the new block at the beginning of the block === //
    // Set up the metadata properly
    metadata_t* newMetaData = (metadata_t*) original;
    newMetaData->block_size = blockSize;
    newMetaData->request_size = blockSize - 2*sizeof(int) - sizeof(metadata_t);
    newMetaData->next = NULL;
    newMetaData->prev = NULL;

    // Set up the first canary
    *(int*) (original + sizeof(metadata_t)) = CANARY;

    // Set up the last canary
    *(int*) (original + blockSize - sizeof(int)) = CANARY;

    // Store the address of the actual user free space (to return later)
    char* newFreeSpace = original + sizeof(metadata_t) + sizeof(int);

    // === Modify the original block's data === //
    original += blockSize;

    // Modify the metadata of the original block
    metadata_t* modifiedMetaData = (metadata_t*) original;
    modifiedMetaData->block_size = origSize - blockSize;
    modifiedMetaData->request_size = 0;
    modifiedMetaData->next = next;
    modifiedMetaData->prev = prev;

    // Set up the first canary
    *(int*) (original + sizeof(metadata_t)) = CANARY;

    // Set up the last canary
    *(int*) (original + (origSize - blockSize) - sizeof(int)) = CANARY;

    // Return the address to the user free space
    return newFreeSpace;
}

static char* mergeBlocks(char* leftBlock, char* rightBlock) {
    // Get the proper variables needed for later
    metadata_t* leftMetaData = (metadata_t*) leftBlock;
    short leftSize = leftMetaData->block_size;
    metadata_t* leftPrev = leftMetaData->prev;

    metadata_t* rightMetaData = (metadata_t*) rightBlock;
    short rightSize = rightMetaData->block_size;
    metadata_t* rightNext = rightMetaData->next;

    // === Modify the needed data to merge the blocks === //
    // Set up metadata
    leftMetaData->block_size = leftSize + rightSize;
    leftMetaData->request_size = 0;

    // Set up metadata pointers
    leftMetaData->next = rightNext;
    leftMetaData->prev = leftPrev;

    // Set up canary #1
    *(int*)(leftBlock + sizeof(metadata_t)) = CANARY;

    // Set up canary #2
    *(int*)(leftBlock + leftSize + rightSize - sizeof(int)) = CANARY;

    return leftBlock;
}
