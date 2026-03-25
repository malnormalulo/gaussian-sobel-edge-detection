void print_2D_float_array(
    int height, 
    int width, 
    float array[height][width]
) {
    for (int i=0; i<height; i++) {
        printf("[");
        for (int j=0; j<width; j++) {
            printf("%.3f%s", array[i][j], j < width - 1 ? ", " : "");
        }
        printf("]\n");
    }
}