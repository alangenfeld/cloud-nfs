#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  const int MAX = 64;
  int handle_id, num_bytes;
  char header[MAX];

  assert(argc == 2);

  FILE *file = fopen(argv[1], "r");
  assert(file);

  while(fgets(header, MAX, file))
    {
      int ret = sscanf(header, "%d %d", &handle_id, &num_bytes);
      assert(ret == 2);
      unsigned char *buffer = malloc(num_bytes);
      fread(buffer, 1, num_bytes, file);
      printf("ENTRY: %d, %d [%s]\n", handle_id, num_bytes, buffer);
      free(buffer);
    }
  exit(0);
}

