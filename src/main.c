
#include <stdlib.h>
#include <sdl/SDL_stdinc.h>
#include <assert.h>
#include "lifecycle.h"

//#include <stdio.h>
//#include "collections/list.h"
//#include "user.h"

//static int usersComparator(const User** a, const User** b) { return (*a)->id < (*b)->id ? -1 : ((*a)->id > (*b)->id ? 1 : 0); }

int main(int argc, const char** argv) {
//    User* a = userCreate(0, "1", 1, false, false);
//    User* b = userCreate(1, "2", 1, false, false);
//    User* c = userCreate(2, "3", 1, false, false);
//
//    List* list = listInit(&userDestroy);
//
//    listAddBack(list, a);
//    listAddBack(list, b);
//
//    printf("%d\n", ((const User*) listBSearch(list, &b, &usersComparator))->id);
//
//    userDestroy(c);
//    listDestroy(list);

    if (!lifecycleInit(argc, argv)) return EXIT_FAILURE;
    lifecycleLoop();
    lifecycleClean();
    assert(!SDL_GetNumAllocations());
    return EXIT_SUCCESS;
}
