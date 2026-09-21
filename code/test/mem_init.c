#include "syscall.h"
#include "mem_init.h"

typedef struct block_hdr {
    unsigned size; /* payload size in bytes */
    struct block_hdr* next; /* next free block */
} block_hdr;

static unsigned char* heap_base = 0;
static unsigned heap_size = 0;
static block_hdr* free_list = 0;

#define ALIGN_UP(sz, a) ((((sz) + ((a)-1)) / (a)) * (a))
#define HDR_SIZE ALIGN_UP(sizeof(block_hdr), 8)
#define PageSize 128

void* mem_init(unsigned size) {
    unsigned npages;
    int base;

    if (size == 0) {
        return 0;
    }

    npages = (size + PageSize - 1) / PageSize;

    base = Sbrk(npages);

    if (base == -1) {
        return 0;
    }

    heap_base = (unsigned char*) (unsigned) base;
    heap_size = (unsigned) npages * PageSize;

    if (heap_size > HDR_SIZE) {
        free_list = (block_hdr*) heap_base;
        free_list->size = heap_size - HDR_SIZE;
        free_list->next = 0;
    } else {
        free_list = 0;
    }

    return (void *) base;
}

void* mem_alloc(unsigned size) {
    if (size == 0 || free_list == 0) return 0;

    unsigned asize = ALIGN_UP((unsigned)size, 8);
    block_hdr* prev = 0;
    block_hdr* cur = free_list;

    while (cur) {
        if (cur->size >= asize) {
            if (cur->size >= asize + HDR_SIZE + 8) {
                unsigned char* cur_addr = (unsigned char*) cur;
                block_hdr* newb = (block_hdr*) (cur_addr + HDR_SIZE + asize);
                newb->size = cur->size - asize - HDR_SIZE;
                newb->next = cur->next;
                cur->size = asize;
                if (prev) prev->next = newb; else free_list = newb;
            } else {
                /* use whole block */
                if (prev) prev->next = cur->next; else free_list = cur->next;
            }

            return (void*) ((unsigned char*) cur + HDR_SIZE);
        }
        prev = cur;
        cur = cur->next;
    }

    /* no suitable block */
    return 0;
}

void mem_free(void* ptr) {
    if (ptr == 0) return;

    unsigned char* p = (unsigned char*) ptr;
    block_hdr* hdr = (block_hdr*) (p - HDR_SIZE);

    /* insert hdr into free_list keeping address order */
    if (free_list == 0 || (unsigned char*) hdr < (unsigned char*) free_list) {
        /* insert at head */
        hdr->next = free_list;
        free_list = hdr;
    } else {
        block_hdr* cur = free_list;
        while (cur->next && (unsigned char*) cur->next < (unsigned char*) hdr) {
            cur = cur->next;
        }
        hdr->next = cur->next;
        cur->next = hdr;
    }

    /* coalesce adjacent free blocks */
    block_hdr* cur = free_list;
    while (cur && cur->next) {
        unsigned char* cur_end = (unsigned char*) cur + HDR_SIZE + cur->size;
        if (cur_end == (unsigned char*) cur->next) {
            /* merge */
            cur->size = cur->size + HDR_SIZE + cur->next->size;
            cur->next = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

/* mem_init is a library for user programs; no main here */