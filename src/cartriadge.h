#ifndef CARTRIADGE_H
#define CARTRIADGE_H

typedef struct {
    unsigned char* mem;
    int (*mapper)(int);
    int size;
} Cartriadge;

void loadCartriadge(char* filePath, Cartriadge* cart);

#endif