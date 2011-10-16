
/* 8bpp greyscale bitmap representation of image */
typedef struct bitmap {
    uint8_t *data; /* bitmap data */
    uint32_t width; /* width of data */
    uint32_t height; /* height of data */
} bitmap;

bitmap *create_bitmap(const char *filename);

void free_bitmap(bitmap *bm);
