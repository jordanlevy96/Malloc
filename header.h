typedef struct header header;

struct header {
    size_t size;                        // 4 or 8 bytes
    struct header *next;                // 4 or 8 bytes
    int free;                           // 4 bytes
    char c[2 * sizeof(void *) - 4];     // 4 or 12 bytes--pad the struct
};                                      // 16 or 32 bytes