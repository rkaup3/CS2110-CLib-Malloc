#include <stdlib.h>
#include <time.h>
#include "my_malloc.h"

#define CANARY 0x2110CAFE

int main() {
    // ================ T1 =============== //
    printf("\n\n========== T1 ===========\n");
    int* space = my_malloc(2048 - 40);
    if (space == NULL) {
        printf("Returned NULL properly\n");
        return 0;
    }
    char* checkSpace = (char*) space;
    int count = 0;
    while (*space != CANARY) {
        space++;
        count++;
    }
    printf("%d bytes allocated\n", (int) (count * sizeof(int)));

    checkSpace = checkSpace - sizeof(int);
    printf("CANARY 1 = %X\n", *(int*)checkSpace);

    checkSpace = checkSpace - 24;
    printf("Block size metadata = %d\n", *(short*)checkSpace);
    printf("Request size metadata = %d\n", *((short*)checkSpace + 1));

    // ================ T2 =============== //
    printf("\n\n========== T2 ===========\n");
    int* space2 = my_malloc(sizeof(int) * 100);
    if (space2 == NULL) {
        printf("Returned NULL properly\n");
        return 0;
    }
    char* checkSpace2 = (char*) space2;
    count = 0;
    while (*space2 != CANARY) {
        space2++;
        count++;
    }
    printf("%d bytes allocated\n", (int) (count * sizeof(int)));

    checkSpace2 = checkSpace2 - sizeof(int);
    printf("CANARY 1 = %X\n", *(int*)checkSpace2);

    checkSpace2 = checkSpace2 - 24;
    printf("Block size metadata = %d\n", *(short*)checkSpace2);

    // ================ T3 =============== //
    printf("\n\n========== T3 ===========\n");
    int* space3 = my_malloc(sizeof(int) * 25);
    if (space3 == NULL) {
        printf("Returned NULL properly\n");
        return 0;
    }
    char* checkSpace3 = (char*) space3;
    count = 0;
    while (*space3 != CANARY) {
        space3++;
        count++;
    }
    printf("%d bytes allocated\n", (int) (count * sizeof(int)));

    checkSpace3 = checkSpace3 - sizeof(int);
    printf("CANARY 1 = %X\n", *(int*)checkSpace3);

    checkSpace3 = checkSpace3 - 24;
    printf("Block size metadata = %d\n", *(short*)checkSpace3);

    // ================ F1 =============== //
    printf("\n\n========== F1 ===========\n");
    my_free(checkSpace + 24 + sizeof(int));

    // ================ F2 =============== //
    printf("\n\n========== F2 ===========\n");
    my_free(checkSpace2 + 24 + sizeof(int));

    // ================ F3 =============== //
    printf("\n\n========== F3 ===========\n");
    my_free(checkSpace3 + 24 + sizeof(int));

	return 0;
}
