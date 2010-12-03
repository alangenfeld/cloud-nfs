#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

typedef struct _bucket
{
  u_int idx;
  u_int size;
  unsigned char *data;
  FILE *file;
} bucket_t;


char G_KEY[32];
char G_SECRET[64];

int cloud_this(bucket_t bucket, CURL* curl)
{
  struct curl_slist *header = NULL;
  char buf[12];

  sprintf(buf, "%d", bucket.size);

  header = curl_slist_append(header, "Date: Thu, 11 Nov 19:35");

  header = curl_slist_append(header, "Host: bucket.commondatastorage.googleapis.com\n");
  header = curl_slist_append(header, "Content-Type: text");
  header = curl_slist_append(header, "Content-Length:");
  header = curl_slist_append(header, buf);
  header = curl_slist_append(header, "\n");
  header = curl_slist_append(header, "Authorization: GOOG1");
  header = curl_slist_append(header, G_KEY);
  header = curl_slist_append(header, ":");
  header = curl_slist_append(header, G_SECRET);
  header = curl_slist_append(header, "\n");
  header = curl_slist_append(header, "x-goog-acl: public-read\n");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
  
  curl_easy_setopt(curl, CURLOPT_URL, "http://commondatastorage.googleapis.com/");
  
  curl_easy_setopt(curl, CURLOPT_PUT, 1L);

  curl_easy_setopt(curl, CURLOPT_READDATA, bucket.file);

  CURLcode curl_code = curl_easy_perform(curl);

  curl_slist_free_all(header);

  printf("%s\n", curl_easy_strerror(curl_code));

  return 0;
}

int main(int argc, char *argv[])
{
  const int MAX = 64;
  bucket_t b;
  char header[MAX];

  assert(argc == 2);

  FILE *file = fopen(argv[1], "r");
  assert(file);

  b.file = file;

  FILE *key = fopen("./secret", "r");
  assert(key);
  fread(G_KEY, 1, 32, key);
  fread(G_SECRET, 1, 32, key);

  CURL *curl = curl_easy_init();
  assert(curl);

  while(fgets(header, MAX, file))
    {
      int ret = sscanf(header, "%u %u", &b.idx, &b.size);
      assert(ret == 2);
      b.data = malloc(b.size);
      fread(b.data, 1, b.size, file);
      //      printf("ENTRY: %d, %d [%s]\n", handle_id, num_bytes, buffer);
      cloud_this(b, curl);
      free(b.data);
    }


  curl_easy_cleanup(curl);

  exit(0);
}
