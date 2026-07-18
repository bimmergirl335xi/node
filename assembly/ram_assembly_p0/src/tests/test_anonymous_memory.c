#include <stdint.h>
#include <stdlib.h>

#define P0_MEMORY_BYTES (1024U * 1024U)

int main(void)
{
    uint8_t *memory = malloc(P0_MEMORY_BYTES);
    size_t index;
    uint8_t accumulator = 0U;

    if (memory == NULL) {
        return 2;
    }
    for (index = 0U; index < P0_MEMORY_BYTES; ++index) {
        memory[index] = (uint8_t)(index * 131U + 17U);
    }
    for (index = 0U; index < P0_MEMORY_BYTES; ++index) {
        const uint8_t expected = (uint8_t)(index * 131U + 17U);
        if (memory[index] != expected) {
            free(memory);
            return 2;
        }
        accumulator ^= memory[index];
    }
    free(memory);
    return accumulator == 0U ? 0 : 2;
}
