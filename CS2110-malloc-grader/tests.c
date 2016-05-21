#include "tests.h"

size_t total_freelist_block_size() {
    size_t total = 0;
    for (metadata_t *t = freelist; t; t = t->next)
        total += t->block_size;
    return total;
}
size_t total_freelist_count() {
    size_t total = 0;
    for (metadata_t *t = freelist; t; t = t->next)
        total++;
    return total;
}
/*int freelist_ordered_general(int is_size) {*/
    /*for (metadata_t *t = freelist; t; t = t->next)*/
        /*if (t->next && (is_size ? t->size > t->next->size : t > t->next))*/
            /*return 0;*/
    /*return 1;*/
/*}*/
/*int freelist_ordered_size() { return freelist_ordered_general(1); }*/
/*int freelist_ordered_addr() { return freelist_ordered_general(0); }*/
#define metadata_plus_canary_size (sizeof(metadata_t) + 2*sizeof(int))
#define to_metadata(i) ( (metadata_t*)(((char*)i)-(sizeof(metadata_t) + sizeof(int))) )

/* Thoroughly checks their my_malloc function */
void check_malloc_1() {
    char* tag = "check_malloc_1 - basic tests";

    int size1 = 10, size2 = 100, size3 = 500, size4 = 1200;

    /* Malloc a 64-byte chunk of memory. */
    metadata_t *arr1 = my_malloc(size1);

    /* Check in_use, size flags */
    if (NULL == arr1 || to_metadata(arr1)->request_size != size1) {
        deduct_points(M_REQUEST_SIZE_POINTS, tag, "the request_size field of the metadata was "
                "not set correctly when returned;");
    } else {
        add_points(M_REQUEST_SIZE_POINTS, tag, "the request_size field of the metadata was "
                "set correctly when returned;");
    }
    if (NULL == arr1 ||
            to_metadata(arr1)->block_size != size1+metadata_plus_canary_size) {
        deduct_points(M_SIZE_POINTS, tag, "the block of data was not malloced to "
                "the proper size;");
    }  else {
        add_points(M_SIZE_POINTS, tag, "the block of data was malloced to "
                "the proper size;");
    }

    /* Check the total free list size */
    {
        if (total_freelist_block_size() != SBRK_SIZE - (size1 + metadata_plus_canary_size)) {
            deduct_points(M_FREELIST_POINTS, tag, "the freelist is not what it "
                    "should be, which will throw off everything else in your "
                    "library;");
        } else {
            add_points(M_FREELIST_POINTS, tag, "the freelist is what it "
                    "should be");
        }
    }

    /* test 2 malloc (serious test) */
    metadata_t *arr2 = my_malloc(size2);
    metadata_t *arr3 = my_malloc(size3);
    metadata_t *arr4 = my_malloc(size4);
    my_free(arr2);
    my_free(arr4);
printList();
    {
        if (total_freelist_block_size(freelist) != SBRK_SIZE - (size1+size3 + metadata_plus_canary_size*2)) {
            deduct_points(M_FREELIST2_POINTS, tag, "the freelist is not what it "
                    "should be after a couple of calls to malloc;");
        } else {
            add_points(M_FREELIST2_POINTS, tag, "the freelist is what it "
                    "should be after a couple of calls to malloc;");
        }
    }

    /* malloc enough to make it allocate another block */
    arr2 = my_malloc(size3+size4);
printList();
    {
        if (total_freelist_block_size(freelist) != 2*SBRK_SIZE - (size1+size3*2 + size4 + metadata_plus_canary_size*3)) {
            deduct_points(M_FREELIST3_POINTS, tag, "the freelist is not what it "
                    "should be after a couple more calls to malloc;");
        } else {
            add_points(M_FREELIST3_POINTS, tag, "the freelist is what it "
                    "should be after a couple more calls to malloc;");
        }
    }
}

void check_malloc_2() {
    /* Malloc's edge cases. */
    char* tag = "check_malloc_2 - edge cases";
    metadata_t *it = my_malloc(SBRK_SIZE - metadata_plus_canary_size);
    if (it == NULL)
        deduct_points(MALLOC_EDGE_MAX, tag, "Did not allocate max size;");
    else
        add_points(MALLOC_EDGE_MAX, tag, "Did allocate max size;");
    my_free(it);

    it = my_malloc(SBRK_SIZE - metadata_plus_canary_size + 1);
    if (it != NULL)
        deduct_points(MALLOC_EDGE_MAX1, tag, "Allocated max size + 1;");
    else
        add_points(MALLOC_EDGE_MAX1, tag, "Did not allocat max size + 1;");

    it = my_malloc(1000000);
    if (it != NULL)
        deduct_points(MALLOC_EDGE_HUGE, tag, "Allocated 1000000 bytes;");
    else
        add_points(MALLOC_EDGE_HUGE, tag, "Did not allocate 1000000 bytes;");

    int i;
    void *mem[5];
    for (i = 0; i < 5; i++)
        mem[i] = my_malloc(SBRK_SIZE - metadata_plus_canary_size);
    if (mem[4] != NULL)
        deduct_points(MALLOC_OOM, tag, "Did not return NULL for OOM;");
    else
        add_points(MALLOC_OOM, tag, "Did return NULL for OOM;");
}

void check_malloc_3() {
    char* tag = "check_malloc_3 - stress test";
    char* last_data = NULL;
    for (size_t i = 0; i < 1000; i++) {
        size_t size = (i*i)%1000 + 1;
        char* data = my_malloc(size);
        for (size_t j = 0; j < size; j++)
            data[j] = i*j;
        if (last_data) {
            size_t old_size = ((i-1)*(i-1))%1000 + 1;
            for (size_t j = 0; j < old_size; j++)
                if (last_data[j] != (char)((i-1)*j)) {
                    deduct_points(MALLOC_STRESS, tag, "Did not pass stress test");
                    return;
                }
            my_free(last_data);
        }
        last_data = data;
    }
    my_free(last_data);
    add_points(MALLOC_STRESS, tag, "Did not pass stress test");
}

/*[> Thoroughly checks their my_free function <]*/
void check_free_1() {
    char* tag = "check_free_1 - basic";
    metadata_t *arr1, *arr2, *arr3, *arr4;
    arr1 = my_malloc(1);
    arr2 = my_malloc(1);
    arr3 = my_malloc(1);
    arr4 = my_malloc(1);

    my_free(arr2);

    /*[> Now, make sure that they didn't combine the nodes <]*/
    if (to_metadata(arr2)->block_size != (1+metadata_plus_canary_size)) {
        deduct_points(F_NO_COMBINE, tag, "Combined the size improperly;");
    } else {
        if (total_freelist_block_size() != SBRK_SIZE - 3*(1 + metadata_plus_canary_size)) {
            deduct_points(F_NO_COMBINE, tag, "freelist is not correct after one "
                    "free;");
        } else {
            add_points(F_NO_COMBINE, tag, "freelist is correct after one free;");
        }
    }

    /*[> Combine two nodes into one <]*/
    my_free(arr1);
    if (to_metadata(arr1)->block_size == 1+metadata_plus_canary_size && to_metadata(arr2)->block_size == 1+metadata_plus_canary_size) {
        deduct_points(F_COMBINE, tag, "Did not combine two nodes into one;");
    } else {
        if (total_freelist_block_size() != SBRK_SIZE - 2*(1 + metadata_plus_canary_size)) {
            deduct_points(F_COMBINE, tag, "the freelist is not what it should be "
                    "after combining one node;");
        } else {
            add_points(F_COMBINE, tag, "the freelist is what it should be "
                    "after combining one node;");
        }
    }

    /*[> Combine all of the way to 2048 <]*/
    my_free(arr3);
    my_free(arr4);
    {
        if (total_freelist_block_size() != SBRK_SIZE || total_freelist_count() != 1) {
            deduct_points(F_COMBINE_RECURSE, tag, "the freelist is not correct after"
                    " combining all to 2048;");
        } else {
            add_points(F_COMBINE_RECURSE, tag, "the freelist is correct after"
                    " combining all to 2048;");
        }
    }
}

void check_free_2() {
    char* tag = "check_free_2 - more combining";
    /*[> The next check is to make sure we do combine with a different size. <]*/
    metadata_t *arr1 = my_malloc(64);
    metadata_t *arr3 = my_malloc(32);
    metadata_t *arr4 = my_malloc(32);
    my_free(arr3);
    my_free(arr1);
    if (arr1 == NULL || to_metadata(arr1)->block_size == 64+metadata_plus_canary_size) {
        deduct_points(F_DIFF_SIZE, tag, "Did not combine two nodes into one;");
    } else {
        if (total_freelist_block_size() != SBRK_SIZE - (32+metadata_plus_canary_size) || total_freelist_count() != 2) {
            deduct_points(F_DIFF_SIZE, tag, "the freelist is not correct after "
                    "freeing a buddy of different size;");
        } else {
            add_points(F_DIFF_SIZE, tag, "the freelist is correct after "
                    "freeing a buddy of different size;");
        }
    }
}
void check_free_3() {

    char* tag = "check_free_3 - test freelist";
    /* The next set of test cases is essentially test code for the linked list
     * free list implementations. So far, in all of the above, the frees only
     * removed the only element from the list. The next three tests will have
     * three free list nodes, one of which is the buddy being freed.
     */
    metadata_t *arr1 = my_malloc(1);
    metadata_t *arr2 = my_malloc(1);
    my_malloc(1); //[> Prevent recombination <]
    metadata_t *arr3 = my_malloc(1);
    my_malloc(1); //[> Prevent recombination <]
    metadata_t *arr4 = my_malloc(1);
    my_free(arr2);
    my_free(arr3);
    my_free(arr4);
    my_free(arr1);
    if (total_freelist_block_size() != SBRK_SIZE - 2*(1+metadata_plus_canary_size) || total_freelist_count() != 3) {
        deduct_points(FREE_MULTI_HEAD, tag, "freelist not correct if buddy is "
                "first in list;");
    } else {
        add_points(FREE_MULTI_HEAD, tag, "freelist correct if buddy is "
                "first in list;");
    }
}
void check_free_4() {
    char* tag = "check_free_4 - more recombination";
    metadata_t *arr1 = my_malloc(1);
    metadata_t *arr2 = my_malloc(1);
    my_malloc(1); //[> Prevent recombination <]
    metadata_t *arr3 = my_malloc(1);
    my_malloc(1); //[> Prevent recombination <]
    metadata_t *arr4 = my_malloc(1);
    my_free(arr3);
    my_free(arr2);
    my_free(arr4);

    my_free(arr1);
    {
        if (total_freelist_block_size() != SBRK_SIZE - 2*(1+metadata_plus_canary_size) || total_freelist_count() != 3) {
            deduct_points(FREE_MULTI_MID, tag, "freelist not correct if buddy is "
                    "second in list;");
        } else {
            add_points(FREE_MULTI_MID, tag, "freelist correct if buddy is "
                    "second in list;");
        }
    }
}

void check_free_5() {
    char* tag = "check_free_5 - more recombination";
    metadata_t *arr1 = my_malloc(1);
    metadata_t *arr2 = my_malloc(1);
    my_malloc(1); //[> Prevent recombination <]
    metadata_t *arr3 = my_malloc(1);
    my_malloc(1); //[> Prevent recombination <]
    metadata_t *arr4 = my_malloc(1);
    my_free(arr3);
    my_free(arr4);
    my_free(arr2);
    my_free(arr1);
    {
        if (total_freelist_block_size() != SBRK_SIZE - 2*(1+metadata_plus_canary_size) || total_freelist_count() != 3) {
            deduct_points(FREE_MULTI_TAIL, tag, "freelist not correct if buddy is "
                    "last in list;");
        } else {
            add_points(FREE_MULTI_TAIL, tag, "freelist correct if buddy is "
                    "last in list;");
        }
    }

    /*my_free(NULL);*/
}
void *c1,*c2,*c3,*c4,*c5,*c6,*c7,*c8;
enum my_malloc_err e1, e2, e3,e4,e5,e6,e7,e8;
void check_EC_1() {
    char* tag = "check_EC_1 - test REQUEST_TOO_LARGE";
    e1 = e2 = e3 = e4 = 999;

    printf("Testing 16\n");
    c1 = my_malloc(2048);
    e1 = ERRNO;

    printf("Testing 17\n");
    c2 = my_malloc(2048-metadata_plus_canary_size+1);
    e2 = ERRNO;
    printf("Testing 18\n");
    c3 = my_malloc(2048-metadata_plus_canary_size);
    e3 = ERRNO;

    printf("Testing 19\n");
    c4 = my_malloc(512);
    e4 = ERRNO;

    printf("Finished EC part 1\n");
    if(	((e1!=SINGLE_REQUEST_TOO_LARGE)||
                (e2!=SINGLE_REQUEST_TOO_LARGE)||
                (e3==SINGLE_REQUEST_TOO_LARGE)||
                (e4==SINGLE_REQUEST_TOO_LARGE))
      )
        deduct_points(ERR_SINGLE_REQUEST_TOO_LARGE_POINTS, tag, "Error code SINGLE_REQUEST_TOO_LARGE not used correctly.");
    else
        add_points(ERR_SINGLE_REQUEST_TOO_LARGE_POINTS, tag, "Error code SINGLE_REQUEST_TOO_LARGE used correctly.");

}

void check_EC_2() {
    char* tag = "check_EC_2 - test OUT_OF_MEMORY";
    /*[>check out of memory<]*/
    printf("Testing 20\n");
    c1 = my_malloc(2000);
    c2 = my_malloc(2000);
    /*[> need more calls <]*/
    c2 = my_malloc(2000);
    c2 = my_malloc(2000);
    c3 = my_malloc(1);

    e1 = ERRNO;
    if(e1!=OUT_OF_MEMORY)
        deduct_points(ERR_OUT_OF_MEMORY_POINTS, tag, "Error code OUT_OF_MEMORY not used correctly.");
    else
        add_points(ERR_OUT_OF_MEMORY_POINTS, tag, "Error code OUT_OF_MEMORY used correctly.");

    printf("Finished EC part 2\n");
}
void check_EC_3() {
    char* tag = "check_EC_3 - test NO_ERROR";
    c1 = my_malloc(48);
    e1 = ERRNO;
    my_free(c1);
    /*[>Quick No_ERROR test<]*/
    if(e1!=NO_ERROR)
        deduct_points(ERR_NO_ERROR_POINTS, tag, "Error code NO_ERROR not used correctly.");
    else
        add_points(ERR_NO_ERROR_POINTS, tag, "Error code NO_ERROR used correctly.");
}
void check_EC_4_canary1() {
    char* tag = "check_EC_4_canary1 - test CANARY_CORRUPTED for beginning of buffer";
    c1 = my_malloc(48);
    ((int*)c1)[-1] = 0xDEADBEEF;
    my_free(c1);
    e1 = ERRNO;
    /*[>Quick No_ERROR test<]*/
    if(e1!=CANARY_CORRUPTED)
        deduct_points(ERR_CANARY_CORRUPTED/2, tag, "Error code CANARY_CORRUPTED not used correctly for corruption before beginning of buffer.");
    else
        add_points(ERR_CANARY_CORRUPTED/2, tag, "Error code CANARY_CORRUPTED used correctly for corruption before beginning of buffer.");
}
void check_EC_4_canary2() {
    char* tag = "check_EC_4_canary2 - test CANARY_CORRUPTED for end of buffer";
    c1 = my_malloc(48);
    ((char*)c1)[48] = 'A';
    my_free(c1);
    e1 = ERRNO;
    /*[>Quick No_ERROR test<]*/
    if(e1!=CANARY_CORRUPTED)
        deduct_points(ERR_CANARY_CORRUPTED/2, tag, "Error code CANARY_CORRUPTED not used correctly for corruption at end of buffer.");
    else
        add_points(ERR_CANARY_CORRUPTED/2, tag, "Error code CANARY_CORRUPTED used correctly for corruption at end of buffer.");
}
/*[>prints out what the freeList should look like<]*/
void printList() {
    metadata_t *temp;
    printf("------LIST-----\n");
    for (temp = freelist; temp != NULL; temp = temp->next) {
        printf("(%d,%d)->", temp->block_size, temp->request_size);
    }
    printf("NULL\n");
    printf("------END------\n");
}
