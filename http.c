#include "curl/curl.h"
#include <string.h>
#include <stdlib.h>
#include "main.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

static void *myrealloc(void *ptr, size_t size);

static void *myrealloc(void *ptr, size_t size) {
  if(ptr)
    return realloc(ptr, size);
  else
    return malloc(size);
}

static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)data;

  mem->memory = myrealloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory) {
    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
  }
  return realsize;
}

extern struct buffer;

int http_request(char *url, struct buffer *response) {
	CURL *curl;
	CURLcode res;
	struct MemoryStruct chunk;

  chunk.memory=NULL; /* we expect realloc(NULL, size) to work */
  chunk.size = 0;    /* no data at this point */


	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if(!curl) {
		return 1;
	}

	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "ChaosVPNclient/2.0");

	res = curl_easy_perform(curl);

	curl_easy_cleanup(curl);

	//memcpy(, chunk.memory, chunk.size);
	response->text = chunk.memory;


	return 0;
}

