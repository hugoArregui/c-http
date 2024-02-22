#ifndef __H_MAIN
#define __H_MAIN

#include <stdint.h>
#include <sys/stat.h>

typedef struct {
  size_t size;
  uint8_t *base;
  size_t used;
} MemoryArena;

typedef struct {
  MemoryArena *arena;
  size_t used;
} TemporaryMemory;

#endif
