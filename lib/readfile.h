#include <stdio.h>
#include <sys/stat.h>

char* readFile(const char *filename) {
  FILE *file;
  struct stat st;

  file = fopen(filename, "r");

  if(file == NULL){
    fprintf( stderr, "ERROR: Cannot open shader file!");
    return 0;
  }

  stat(filename,&st);

  int bytesinfile = st.st_size;
  char *buffer = (char*)malloc(bytesinfile+sizeof(char));
  int bytesread = fread( buffer, 1, bytesinfile, file);
  buffer[bytesread] = 0; // Terminate the string with 0
  fclose(file);

  return buffer;
}