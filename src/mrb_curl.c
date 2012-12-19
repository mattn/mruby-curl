#include <memory.h>
#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/string.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <curl/curl.h>
#include <stdio.h>

#define REQ_GET(mrb, instance, name) \
  RSTRING_PTR(mrb_iv_get(mrb, instance, mrb_intern(mrb, name)))

static struct RClass *_class_curl;

typedef struct {
  char* data;   // response data from server
  size_t size;  // response size of data
} MEMFILE;

static MEMFILE*
memfopen() {
  MEMFILE* mf = (MEMFILE*) malloc(sizeof(MEMFILE));
  if (mf) {
    mf->data = NULL;
    mf->size = 0;
  }
  return mf;
}

static void
memfclose(MEMFILE* mf) {
  if (mf->data) free(mf->data);
  free(mf);
}

static size_t
memfwrite(char* ptr, size_t size, size_t nmemb, void* stream) {
  MEMFILE* mf = (MEMFILE*) stream;
  int block = size * nmemb;
  if (!mf) return block; // through
  if (!mf->data)
    mf->data = (char*) malloc(block);
  else
    mf->data = (char*) realloc(mf->data, mf->size + block);
  if (mf->data) {
    memcpy(mf->data + mf->size, ptr, block);
    mf->size += block;
  }
  return block;
}

static char*
memfstrdup(MEMFILE* mf) {
  char* buf;
  if (mf->size == 0) return NULL;
  buf = (char*) malloc(mf->size + 1);
  memcpy(buf, mf->data, mf->size);
  buf[mf->size] = 0;
  return buf;
}

static mrb_value
mrb_curl_get(mrb_state *mrb, mrb_value self)
{
  char error[CURL_ERROR_SIZE] = {0};
  CURL* curl;
  CURLcode res = CURLE_OK;
  MEMFILE* mf_header;
  MEMFILE* mf_body;

  mrb_value url = mrb_nil_value();
  mrb_get_args(mrb, "o", &url);

  if (mrb_type(url) != MRB_TT_STRING) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }
  mf_body = memfopen();
  mf_header = memfopen();
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, RSTRING_PTR(url));
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, mf_body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, mf_header);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, memfwrite);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    mrb_raise(mrb, E_RUNTIME_ERROR, error);
  }
  mrb_value str = mrb_str_new(mrb, mf_header->data, mf_header->size);
  mrb_str_cat(mrb, str, mf_body->data, mf_body->size);
  memfclose(mf_body);
  memfclose(mf_header);

  struct RClass* clazz = mrb_class_get(mrb, "HTTP");
  clazz = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(clazz), mrb_intern(mrb, "Parser")));
  mrb_value parser = mrb_class_new_instance(mrb, 0, NULL, clazz);
  mrb_value args[1];
  args[0] = str;
  mrb_value response = mrb_funcall_argv(mrb, parser, mrb_intern(mrb, "parse_response"), 1, args);
  return response;
}

static mrb_value
mrb_curl_post(mrb_state *mrb, mrb_value self)
{
  char error[CURL_ERROR_SIZE] = {0};
  CURL* curl;
  CURLcode res = CURLE_OK;
  MEMFILE* mf_header;
  MEMFILE* mf_body;

  mrb_value url = mrb_nil_value();
  mrb_value data = mrb_nil_value();
  mrb_get_args(mrb, "oo", &url, &data);

  if (mrb_type(url) != MRB_TT_STRING) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }
  if (mrb_type(data) != MRB_TT_STRING) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }
  // TODO: treating HASH/ARRAY.
  mf_body = memfopen();
  mf_header = memfopen();
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, RSTRING_PTR(url));
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, RSTRING_PTR(data));
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, mf_body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, mf_header);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, memfwrite);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    mrb_raise(mrb, E_RUNTIME_ERROR, error);
  }
  mrb_value str = mrb_str_new(mrb, mf_header->data, mf_header->size);
  mrb_str_cat(mrb, str, mf_body->data, mf_body->size);
  memfclose(mf_body);
  memfclose(mf_header);

  struct RClass* clazz = mrb_class_get(mrb, "HTTP");
  clazz = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(clazz), mrb_intern(mrb, "Parser")));
  mrb_value parser = mrb_class_new_instance(mrb, 0, NULL, clazz);
  mrb_value args[1];
  args[0] = str;
  mrb_value response = mrb_funcall_argv(mrb, parser, mrb_intern(mrb, "parse_response"), 1, args);
  return response;
}

#if 0
static mrb_value
mrb_curl_send(mrb_state *mrb, mrb_value self)
{
  char error[CURL_ERROR_SIZE] = {0};
  CURL* curl;
  CURLcode res = CURLE_OK;
  MEMFILE* mf;

  mrb_value url = mrb_nil_value();
  mrb_value req = mrb_nil_value();
  mrb_get_args(mrb, "o", &url, &req);

  mrb_value clazz = mrb_funcall(mrb, arg, "class", 0, NULL);
  mrb_value name = mrb_funcall(mrb, clazz, "to_s", 0, NULL);
  if (RSTRING_PTR(name) != "HTTP::Request") {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }

  mf = memfopen();
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, RSTRING_PTR(url));
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, RSTRING_PTR(data));
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, mf);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    mrb_raise(mrb, E_RUNTIME_ERROR, error);
  }
  mrb_value str = mrb_str_new(mrb, mf->data, mf->size);
  memfclose(mf);
  return str;
}
#endif

void
mrb_mruby_curl_gem_init(mrb_state* mrb)
{
  _class_curl = mrb_define_module(mrb, "Curl");
  mrb_define_class_method(mrb, _class_curl, "get", mrb_curl_get, ARGS_REQ(1));
  mrb_define_class_method(mrb, _class_curl, "post", mrb_curl_post, ARGS_REQ(2));
  //mrb_define_class_method(mrb, _class_curl, "send", mrb_curl_post, ARGS_REQ(2));
}

/* vim:set et ts=2 sts=2 sw=2 tw=0: */
