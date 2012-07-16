
#include "png_reader.h"
#include <stdlib.h>

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

#define PNG_BYTES_TO_CHECK 8

png_data_t *read_png(char *filename) {
  
  png_data_t *pd;
  unsigned char *pb;
  
  png_structp png_ptr;
  png_infop info_ptr;
  unsigned int sig_read = 0;
  char header[PNG_BYTES_TO_CHECK];
  int numChannels;
  
  int r;
  
  FILE *fp = fopen(filename, "rb");
  if( !fp ){
    fprintf( stderr, "Can't open PNG texture file '%s'.\n", filename );
    return 0;
  }

  fread( header, 1, PNG_BYTES_TO_CHECK, fp );
  if( png_sig_cmp( (png_byte*) &header[0], 0, PNG_BYTES_TO_CHECK) ){
    fprintf( stderr, "Texture file '%s' is not in PNG format.\n", filename );
    fclose(fp);
    return 0;
  }

  png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
  if( png_ptr == NULL ){
    fprintf( stderr, "Can't initialize PNG file for reading: %s\n", filename );
    fclose(fp);
    return 0;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if( info_ptr == NULL ){
    fclose(fp);
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    fprintf( stderr, "Can't allocate memory to read PNG file: %s\n", filename );
    return 0;
  }
  
  if( setjmp(png_jmpbuf(png_ptr)) ){
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);
    fprintf( stderr, "Exception occurred while reading PNG file: %s\n", filename );
    return 0;
  }

  png_init_io(png_ptr, fp);

  png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

  png_read_png( png_ptr, info_ptr,
                PNG_TRANSFORM_STRIP_16 |
                PNG_TRANSFORM_PACKING |
                PNG_TRANSFORM_EXPAND, NULL );

  numChannels = png_get_channels(png_ptr, info_ptr);
  
  if( png_get_bit_depth(png_ptr, info_ptr) != 8 ){
    fprintf( stderr, "Can't handle PNG files with bit depth other than 8.  '%s' has %d bits per pixel.\n",
             filename, png_get_bit_depth(png_ptr, info_ptr) );
    fclose(fp);
    return 0;
  }
  if( numChannels == 2 ){
    fprintf( stderr, "Can't handle a two-channel PNG file: %s.\n", filename );
    fclose(fp);
    return 0;
  }
  
  pd = (png_data_t*)malloc( sizeof(png_data_t) );
  pd->channels = numChannels;
  
  pd->width = png_get_image_width(png_ptr,info_ptr);
  pd->height = png_get_image_height(png_ptr,info_ptr);

  pd->pixelData = pb = (unsigned char*)malloc( sizeof(unsigned char)*( numChannels * pd->width * pd->height ) );

  for( r = (int)info_ptr->height - 1 ; r >= 0 ; r-- ){
    png_bytep row = info_ptr->row_pointers[r];
    int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    int c;
    for( c = 0 ; c < rowbytes ; c++ )
      switch( numChannels ){
      case 1:
        *(pb)++ = row[c];
        break;
      case 2:
        fprintf( stderr, "Can't handle a two-channel PNG file: %s\n", filename );
        free(pd);
        fclose(fp);
        return 0;
      case 3:
      case 4:
        *(pb)++ = row[c];
        break;
      }
  }

  pd->has_alpha = (numChannels == 4);

  png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
  fclose(fp);
  
  return pd;
}


void free_png(png_data_t *pd){
  free(pd);
}

void print_png(png_data_t *pd){
  fprintf( stderr, "PNG[Size=%dx%d,Channels=%d%s]\n",
           pd->width, pd->height, pd->channels, pd->has_alpha?",Alpha=TRUE":"" );
}
