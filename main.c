#include <curl/curl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include "hls/hls.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t write_data(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    perror("hlsget");
    return 0;
  }
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = '\0';
  return realsize;
}

static CURLcode get_data(CURL *curl, char *url, struct MemoryStruct *chunk) {
  int retries = 6;
  CURLcode res;
  curl_easy_setopt(curl, CURLOPT_URL, url);
  while (retries -= 1) {
    chunk->size = 0;
    if ((res = curl_easy_perform(curl))) {
      fprintf(stderr, "hlsget: %s\nhlsget: Retrying\n", curl_easy_strerror(res));
      continue;
    }
    break;
  }
  return res;
}

static void handle_errors(void) {
  ERR_print_errors_fp(stderr);
  abort();
}

static int decrypt_data(EVP_CIPHER_CTX *ctx, uint8_t *ciphertext, int ciphertext_len, uint8_t *key, uint8_t *iv, uint8_t *plaintext) {
  int len;
  int plaintext_len;
  if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
    handle_errors();
  }
  if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
    handle_errors();
  }
  plaintext_len = len;
  if (!EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
    handle_errors();
  }
  plaintext_len += len;
  return plaintext_len;
}

int main(int argc, char *argv[]) {
  char url[2048];
  char *path;
  char *filename = NULL;
  FILE *output;
  struct MemoryStruct chunk;
  CURL *curl;
  char *src;
  int i = 0;
  char *ptr;
  struct MediaPlaylist media_playlist;
  EVP_CIPHER_CTX *ctx;
  uint8_t key[16];
  if (argc < 2) {
    fputs("usage: hlsget playlist.m3u8 [output.ts]\n", stderr);
    return 1;
  }
  strcpy(url, argv[1]);
  path = strrchr(url, '/');
  if (!path) {
    fputs("hlsget: Invalid URL\n", stderr);
    return 1;
  }
  path += 1;
  if (argc > 2) {
    if (!strstr(argv[2], ".ts")) {
      fputs("hlsget: Invalid filename\n", stderr);
      return 1;
    }
    filename = argv[2];
    if ((output = fopen(filename, "rb"))) {
      fprintf(stderr, "hlsget: %s: File exists\n", filename);
      fclose(output);
      return 1;
    }
  }
  chunk.memory = malloc(1);
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  if (get_data(curl, url, &chunk)) {
    fprintf(stderr, "hlsget: %s: Failed\n", filename);
    return 1;
  }
  src = malloc(chunk.size);
  if (!src) {
    perror("hlsget");
    return 1;
  }
  strcpy(src, chunk.memory);
  if (get_playlist_type(src) == MASTER_PLAYLIST) {
    struct MasterPlaylist master_playlist;
    struct Stream *stream;
    if (parse_master_playlist(src, &master_playlist)) {
      fputs("hlsget: Invalid playlist\n", stderr);
      return 1;
    }
    stream = &master_playlist.streams[0];
    i = 1;
    while (i < master_playlist.len) {
      if (master_playlist.streams[i].bandwidth > stream->bandwidth) {
        stream = &master_playlist.streams[i];
      }
      i += 1;
    }
    i = 0;
    strcpy(strrchr(url, '/') + 1, stream->uri);
    master_playlist_cleanup(&master_playlist);
  }
  if (get_data(curl, url, &chunk)) {
    fprintf(stderr, "hlsget: %s: Failed\n", filename);
    return 1;
  }
  ptr = realloc(src, chunk.size);
  if (!ptr) {
    perror("hlsget");
    return 1;
  }
  src = ptr;
  strcpy(src, chunk.memory);
  path = strrchr(url, '/');
  path += 1;
  if (!filename) {
    filename = malloc(strlen(path) - 1);
    memcpy(filename, path, strlen(path) - 4);
    filename[strlen(path) - 4] = '\0';
    strcat(filename, "ts");
    if ((output = fopen(filename, "rb"))) {
      fprintf(stderr, "hlsget: %s: File exists\n", filename);
      fclose(output);
      return 1;
    }
  }
  if (parse_media_playlist(src, &media_playlist)) {
    fputs("hlsget: Invalid playlist\n", stderr);
    return 1;
  }
  output = fopen(filename, "wb");
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    handle_errors();
  }
  while (i < media_playlist.len) {
    if (media_playlist.segments[i].key.method == AES_128) {
      if (!(i && !strcmp(media_playlist.segments[i - 1].key.uri, media_playlist.segments[i].key.uri))) {
        strcpy(path, media_playlist.segments[i].key.uri);
        if (get_data(curl, url, &chunk)) {
          fprintf(stderr, "hlsget: %s: Failed\n", filename);
          return 1;
        }
        memcpy(key, chunk.memory, 16);
      }
    }
    strcpy(path, media_playlist.segments[i].uri);
    if (get_data(curl, url, &chunk)) {
      fprintf(stderr, "hlsget: %s: Failed\n", filename);
      return 1;
    }
    if (media_playlist.segments[i].key.method == AES_128) {
      chunk.size = (size_t)decrypt_data(ctx, (uint8_t *)chunk.memory, (int)chunk.size, key, media_playlist.segments[i].key.iv, (uint8_t *)chunk.memory);
    }
    fwrite(chunk.memory, sizeof(char), chunk.size, output);
    printf("\rhlsget: %s (%d%%)", filename, i * 100 / (media_playlist.len - 1));
    fflush(stdout);
    i += 1;
  }
  EVP_CIPHER_CTX_free(ctx);
  putchar('\n');
  fclose(output);
  if (argc == 2) {
    free(filename);
  }
  free(chunk.memory);
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  free(src);
  media_playlist_cleanup(&media_playlist);
  return 0;
}
