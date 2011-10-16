
/* 8bpp greyscale bitmap representation of image */
typedef struct bitmap {
    uint8_t *data; /* bitmap data */
    uint32_t width; /* width of data */
    uint32_t height; /* height of data */
} bitmap;

bool create_gs_bitmap(const char *filename, bitmap *bm);
