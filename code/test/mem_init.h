#ifndef MEM_INIT_H
#define MEM_INIT_H

void* mem_init(unsigned size);
void* mem_alloc(unsigned size);
void mem_free(void* ptr);

#endif