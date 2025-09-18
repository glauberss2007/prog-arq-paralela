#include "ppm.h"
#include <stdio.h>

void writePPMImage(int* data, int width, int height, const char *filename, int maxIterations) {
    FILE *fp = fopen(filename, "wb");
    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    
    for (int i = 0; i < width * height; ++i) {
        unsigned char color[3];
        if (data[i] == maxIterations) {
            color[0] = 0; color[1] = 0; color[2] = 0;
        } else {
            color[0] = data[i] % 256;
            color[1] = data[i] % 256;
            color[2] = data[i] % 256;
        }
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);
}