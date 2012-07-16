
#ifndef _PNG_READER_
#define _PNG_READER_

#include <libpng12/png.h>

#ifdef __cplusplus
extern "C" {
#endif
  
  typedef struct {
    unsigned char *pixelData;
    int width, height;
    int channels;
    int has_alpha;
  } png_data_t;
  
  
  png_data_t *read_png(char *filename);
  void free_png(png_data_t *pd);
  void print_png(png_data_t *pd);
  
#ifdef __cplusplus
}
#endif
#endif
