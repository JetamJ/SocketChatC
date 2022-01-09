//
// Created by Mato on 06.01.2022.
//
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


#ifndef POSSEMKASERVER_VLAKNO_H
#define POSSEMKASERVER_VLAKNO_H

typedef struct {
    pthread_t* arrayOfThreads;
    size_t used;
    size_t size;
} Vlakno;

void initThreads(Vlakno* t, size_t initialSize);
void freeThreads(Vlakno* t);

#endif //POSSEMKASERVER_VLAKNO_H
