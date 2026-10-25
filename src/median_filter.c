#include "median_filter.h"

#include <stdlib.h>
#include <stddef.h>

// De pc-tests gebruiken extra geheugencontrole en kunnen malloc laten mislukken
// De filterberekening blijft hetzelfde
#ifdef MEDIAN_FILTER_TEST_ALLOCATOR
#include "filter_test_allocator.h"
#define malloc filter_test_malloc
#define free filter_test_free
#endif

#define FILTER_MAX_SIZE 15

typedef struct
{
    uint16_t value;
    uint8_t age; // 0 is de nieuwste meting
} FilterSample;

static FilterSample *samples = NULL;
static uint8_t filter_size = 0;
static uint8_t sample_count = 0;

static int compare_value(const void *a, const void *b)
{
    const FilterSample *first = a;
    const FilterSample *second = b;

    if (first->value < second->value) return -1;
    if (first->value > second->value) return 1;

    return 0;
}

static int compare_age(const void *a, const void *b)
{
    const FilterSample *first = a;
    const FilterSample *second = b;

    if (first->age < second->age) return -1;
    if (first->age > second->age) return 1;

    return 0;
}

bool median_filter_resize(uint8_t new_size)
{
    // Alleen oneven groottes van 1 t/m 15
    if (new_size < 1 ||
        new_size > FILTER_MAX_SIZE ||
        new_size % 2 == 0)
    {
        return false;
    }

    if (samples != NULL && filter_size == new_size)
    {
        return true;
    }

    FilterSample *new_samples =
        malloc((size_t)new_size * sizeof(FilterSample));

    if (new_samples == NULL)
    {
        // Bij te weinig geheugen blijven de oude metingen bewaard
        return false;
    }

    uint8_t keep_count = sample_count;

    if (keep_count > new_size)
    {
        keep_count = new_size;
    }

    if (samples != NULL && sample_count > 0)
    {
        // Nieuwste metingen vooraan zetten
        qsort(samples, sample_count,
              sizeof(FilterSample), compare_age);

        // Alleen de nieuwste metingen bewaren
        for (uint8_t i = 0; i < keep_count; i++)
        {
            new_samples[i] = samples[i];
            new_samples[i].age = i;
        }
    }

    free(samples);

    samples = new_samples;
    filter_size = new_size;
    sample_count = keep_count;

    return true;
}

uint16_t median_filter_add(uint16_t value)
{
    if (samples == NULL || filter_size == 0)
    {
        // Zonder beschikbare buffer niet filteren
        return value;
    }

    for (uint8_t i = 0; i < sample_count; i++)
    {
        samples[i].age++;
    }

    if (sample_count < filter_size)
    {
        samples[sample_count].value = value;
        samples[sample_count].age = 0;

        sample_count++;
    }
    else
    {
        // Oudste meting zoeken
        uint8_t oldest = 0;

        for (uint8_t i = 1; i < sample_count; i++)
        {
            if (samples[i].age > samples[oldest].age)
            {
                oldest = i;
            }
        }

        samples[oldest].value = value;
        samples[oldest].age = 0;
    }

    // Sorteer een kopie zodat de meetvolgorde bewaard blijft
    FilterSample sorted[FILTER_MAX_SIZE];

    for (uint8_t i = 0; i < sample_count; i++)
    {
        sorted[i] = samples[i];
    }

    // Na toevoegen zijn altijd 1..filter_size elementen gevuld
    // Cppcheck geeft hier onterecht een waarschuwing
    // cppcheck-suppress uninitvar
    qsort(sorted, sample_count,
          sizeof(FilterSample), compare_value);

    // Kies bij een even aantal metingen de laagste middelste waarde
    return sorted[(sample_count - 1) / 2].value;
}

uint8_t median_filter_get_size(void)
{
    return filter_size;
}

void median_filter_free(void)
{
    free(samples);

    samples = NULL;
    filter_size = 0;
    sample_count = 0;
}
