#include "filter_test_allocator.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// De tests tellen allocaties en kunnen malloc laten mislukken
// Alleen het filter gebruikt deze extra geheugencontrole
// De union zorgt dat de buffer goed is uitgelijnd
typedef union
{
    max_align_t alignment;
    struct
    {
        size_t size;
        uint32_t magic;
    } data;
} AllocationHeader;

#define HEADER_MAGIC 0x13579BDFUL
#define GUARD_SIZE 16U
#define MAX_LIVE 4U

static AllocationHeader *live[MAX_LIVE];
static size_t blocks;
static size_t alloc_calls;
static size_t free_calls;
static size_t errors;
static bool fail_next;

static bool valid_guard(const AllocationHeader *header)
{
    if (header->data.magic != HEADER_MAGIC) return false;
    const unsigned char *tail = (const unsigned char *)(header + 1) + header->data.size;
    for (size_t i = 0; i < GUARD_SIZE; ++i)
    {
        if (tail[i] != 0xA5U) return false;
    }
    return true;
}

void *filter_test_malloc(size_t size)
{
    ++alloc_calls;
    if (fail_next)
    {
        fail_next = false;
        return NULL;
    }
    if (blocks == MAX_LIVE)
    {
        ++errors;
        return NULL;
    }
    AllocationHeader *header = malloc(sizeof(*header) + size + GUARD_SIZE);
    if (header == NULL) return NULL;
    header->data.size = size;
    header->data.magic = HEADER_MAGIC;
    // Geheugen: [header][buffer][16 controlebytes met 0xA5]
    // header + 1 wijst naar het begin van de buffer
    memset(header + 1, 0xCC, size);
    memset((unsigned char *)(header + 1) + size, 0xA5, GUARD_SIZE);
    live[blocks] = header;
    ++blocks;
    return header + 1;
}

void filter_test_free(void *pointer)
{
    if (pointer == NULL) return;
    for (size_t i = 0; i < blocks; ++i)
    {
        AllocationHeader *header = live[i];
        if ((void *)(header + 1) == pointer)
        {
            if (!valid_guard(header)) ++errors;
            memset(pointer, 0xDD, header->data.size);
            free(header);
            // Vul het lege vak met de laatste actieve allocatie
            --blocks;
            live[i] = live[blocks];
            ++free_calls;
            return;
        }
    }
    ++errors; // Onbekende pointer of dubbele free
}

void allocator_reset(void)
{
    // Laat bestaande buffers staan zodat een geheugenlek zichtbaar blijft
    alloc_calls = 0;
    free_calls = 0;
    errors = blocks != 0U ? 1U : 0U;
    fail_next = false;
}

void allocator_fail_next(void) { fail_next = true; }
bool allocator_guards_ok(void)
{
    for (size_t i = 0; i < blocks; ++i)
    {
        if (!valid_guard(live[i])) return false;
    }
    return true;
}
size_t allocator_live_blocks(void) { return blocks; }
size_t allocator_alloc_calls(void) { return alloc_calls; }
size_t allocator_free_calls(void) { return free_calls; }
size_t allocator_errors(void) { return errors; }
