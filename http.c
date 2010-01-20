#include "curl/curl.h"
#include "string/string.h"
#include "version.h"

static size_t http_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct string *body = (struct string *)data;

	string_concatb(body, ptr, realsize);

	return realsize;
}

int http_request(char *url, struct string *response) {
	CURL *curl;
	CURLcode res;
	long code;
	char curlerror[CURL_ERROR_SIZE];
	char useragent[1024];

	snprintf(useragent, sizeof(useragent), "ChaosVPNclient/%s", VERSION);

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (!curl) {
		printf("unable to load libcurl\n");
		return 1;
	}

	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerror);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

	res = curl_easy_perform(curl);
	if (res) {
		printf("Unable to request URL - %s\n", curlerror);
		goto bail_out;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if (code != 200) {
		printf("Request deliver wrong response code - %ld\n", code);
		res = 1;
		goto bail_out;
	}

bail_out:
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return res;
}

