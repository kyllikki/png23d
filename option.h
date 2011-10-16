/* options */

enum output_type {
    OUTPUT_PGM,
    OUTPUT_SCAD,
    OUTPUT_STL,
    OUTPUT_ASTL,
};

typedef struct options {
    enum output_type type; /* the type of output to produce */

    unsigned int transparent; /* the grey level value at which object is transparent */
    unsigned int levels; /* the number of levels to quantise into below transparent */

    char *infile; /* input fielname */
    char *outfile; /* output filename */

    float width; /* the target width */
    float height; /* the target height */
    float depth; /* the target depth */

} options;
