//
// Created by Mato on 06.01.2022.
//
#include "Vlakno.h"

void initThreads(Vlakno* t, size_t initialSize) {
    t->arrayOfThreads = (pthread_t*)malloc(initialSize * sizeof(pthread_t));
    t->used = 0;
    t->size = initialSize;
}

void freeThreads(Vlakno* t) {
    free(t->arrayOfThreads);
    t->arrayOfThreads = NULL;
    t->used = t->size = 0;
}
