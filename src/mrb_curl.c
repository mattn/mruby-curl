#include <memory.h>
#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <curl/curl.h>
#include <stdlib.h>

#define REQ_GET(mrb, instance, name) \
  RSTRING_PTR(mrb_iv_get(mrb, instance, mrb_intern_cstr(mrb, name)))

typedef struct {
  char* data;   // response data from server
  size_t size;  // response size of data
  mrb_state* mrb;
  mrb_value proc;
  mrb_value header;
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

static size_t
memfwrite_callback(char* ptr, size_t size, size_t nmemb, void* stream) {
  MEMFILE* mf = (MEMFILE*) stream;
  int block = size * nmemb;

  mrb_value args[2];
  mrb_state* mrb = mf->mrb;

  int ai = mrb_gc_arena_save(mrb); \
  if (mf->data && mrb_nil_p(mf->header))  {
    mrb_value str = mrb_str_new(mrb, mf->data, mf->size);
    struct RClass* _class_http = mrb_module_get(mrb, "HTTP");
    struct RClass* _class_http_parser = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(_class_http), mrb_intern_cstr(mrb, "Parser")));
    mrb_value parser = mrb_obj_new(mrb, _class_http_parser, 0, NULL);
    args[0] = str;
    mf->header = mrb_funcall_argv(mrb, parser, mrb_intern_cstr(mrb, "parse_response"), 1, args);
  }

  args[0] = mf->header;
  args[1] = mrb_str_new(mrb, ptr, block);
  mrb_gc_arena_restore(mrb, ai);
  mrb_yield_argv(mrb, mf->proc, 2, args);
  return block;
}

static mrb_value
mrb_curl_get(mrb_state *mrb, mrb_value self)
{
  char error[CURL_ERROR_SIZE] = {0};
  CURL* curl;
  CURLcode res = CURLE_OK;
  MEMFILE* mf;
  struct RClass* _class_curl;
  int ssl_verifypeer;
  struct curl_slist* headerlist = NULL;
  mrb_value str;
  struct RClass* _class_http;
  struct RClass* _class_http_parser;
  mrb_value parser;
  mrb_value args[1];

  mrb_value url = mrb_nil_value();
  mrb_value headers = mrb_nil_value();
  mrb_value b = mrb_nil_value();
  mrb_get_args(mrb, "S|H&", &url, &headers, &b);

  if (!mrb_nil_p(headers) && mrb_type(headers) != MRB_TT_HASH) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }
  mf = memfopen();
  curl = curl_easy_init();
  _class_curl = mrb_module_get(mrb, "Curl");
  ssl_verifypeer = mrb_fixnum(mrb_const_get(mrb, mrb_obj_value(_class_curl), mrb_intern_cstr(mrb, "SSL_VERIFYPEER")));
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, ssl_verifypeer);
  curl_easy_setopt(curl, CURLOPT_URL, RSTRING_PTR(url));
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, mf);
  if (mrb_nil_p(b)) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
  } else {
    mf->mrb = mrb;
    mf->proc = b;
    mf->header = mrb_nil_value();
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite_callback);
  }
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, mf);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, memfwrite);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);

  if (!mrb_nil_p(headers)) {
    mrb_value keys = mrb_hash_keys(mrb, headers);
    int i, l = RARRAY_LEN(keys);
    for (i = 0; i < l; i++) {
      mrb_value key = mrb_ary_entry(keys, i);
      mrb_value header = mrb_str_dup(mrb, key);
      mrb_str_cat2(mrb, header, ": ");
      mrb_str_concat(mrb, header, mrb_hash_get(mrb, headers, key));
      headerlist = curl_slist_append(headerlist, RSTRING_PTR(header));
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
  }

  res = curl_easy_perform(curl);

  if (headerlist)
    curl_slist_free_all(headerlist);

  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    mrb_raise(mrb, E_RUNTIME_ERROR, error);
  }
  if (!mrb_nil_p(b)) {
    return mrb_nil_value();
  }

  str = mrb_str_new(mrb, mf->data, mf->size);
  memfclose(mf);

  _class_http = mrb_module_get(mrb, "HTTP");
  _class_http_parser = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(_class_http), mrb_intern_cstr(mrb, "Parser")));
  parser = mrb_obj_new(mrb, _class_http_parser, 0, NULL);
  args[0] = str;
  return mrb_funcall_argv(mrb, parser, mrb_intern_cstr(mrb, "parse_response"), 1, args);
}

static mrb_value
mrb_curl_post(mrb_state *mrb, mrb_value self)
{
  char error[CURL_ERROR_SIZE] = {0};
  CURL* curl;
  CURLcode res = CURLE_OK;
  MEMFILE* mf;
  struct RClass* _class_curl;
  int ssl_verifypeer;
  struct curl_slist* headerlist = NULL;
  mrb_value str;
  struct RClass* _class_http;
  struct RClass* _class_http_parser;
  mrb_value parser;
  mrb_value args[1];

  mrb_value url = mrb_nil_value();
  mrb_value data = mrb_nil_value();
  mrb_value headers = mrb_nil_value();
  mrb_value b = mrb_nil_value();
  mrb_get_args(mrb, "SS|H&", &url, &data, &headers, &b);

  // TODO: treating HASH/ARRAY.
  mf = memfopen();
  curl = curl_easy_init();
  _class_curl = mrb_module_get(mrb, "Curl");
  ssl_verifypeer = mrb_fixnum(mrb_const_get(mrb, mrb_obj_value(_class_curl), mrb_intern_cstr(mrb, "SSL_VERIFYPEER")));
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, ssl_verifypeer);
  curl_easy_setopt(curl, CURLOPT_URL, RSTRING_PTR(url));
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, RSTRING_PTR(data));
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, mf);
  if (mrb_nil_p(b)) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
  } else {
    mf->mrb = mrb;
    mf->proc = b;
    mf->header = mrb_nil_value();
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite_callback);
  }
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, mf);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, memfwrite);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);

  if (!mrb_nil_p(headers)) {
    mrb_value keys = mrb_hash_keys(mrb, headers);
    int i, l = RARRAY_LEN(keys);
    for (i = 0; i < l; i++) {
      mrb_value key = mrb_ary_entry(keys, i);
      mrb_value header = mrb_str_dup(mrb, key);
      mrb_str_cat2(mrb, header, ": ");
      mrb_str_concat(mrb, header, mrb_hash_get(mrb, headers, key));
      headerlist = curl_slist_append(headerlist, RSTRING_PTR(header));
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
  }

  res = curl_easy_perform(curl);

  if (headerlist)
    curl_slist_free_all(headerlist);

  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    mrb_raise(mrb, E_RUNTIME_ERROR, error);
  }
  if (!mrb_nil_p(b)) {
    return mrb_nil_value();
  }

  str = mrb_str_new(mrb, mf->data, mf->size);
  memfclose(mf);

  _class_http = mrb_module_get(mrb, "HTTP");
  _class_http_parser = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(_class_http), mrb_intern_cstr(mrb, "Parser")));
  parser = mrb_obj_new(mrb, _class_http_parser, 0, NULL);
  args[0] = str;
  return mrb_funcall_argv(mrb, parser, mrb_intern_cstr(mrb, "parse_response"), 1, args);
}

static mrb_value
mrb_curl_send(mrb_state *mrb, mrb_value self)
{
  char error[CURL_ERROR_SIZE] = {0};
  CURL* curl;
  CURLcode res = CURLE_OK;
  MEMFILE* mf;
  struct RClass* _class_curl;
  int ssl_verifypeer;
  struct curl_slist* headerlist = NULL;
  mrb_value body;
  mrb_value method;
  mrb_value _class_http_request;
  mrb_value name;
  mrb_value headers;
  mrb_value str;
  struct RClass* _class_http;
  struct RClass* _class_http_parser;
  mrb_value parser;
  mrb_value args[1];

  mrb_value url = mrb_nil_value();
  mrb_value req = mrb_nil_value();
  mrb_value b = mrb_nil_value();
  mrb_get_args(mrb, "So&", &url, &req, &b);

  _class_http_request = mrb_funcall(mrb, req, "class", 0, NULL);
  name = mrb_funcall(mrb, _class_http_request, "to_s", 0, NULL);
  if (strcmp(RSTRING_PTR(name), "HTTP::Request")) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }
  
  mf = memfopen();
  curl = curl_easy_init();
  _class_curl = mrb_module_get(mrb, "Curl");
  ssl_verifypeer = mrb_fixnum(mrb_const_get(mrb, mrb_obj_value(_class_curl), mrb_intern_cstr(mrb, "SSL_VERIFYPEER")));
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, ssl_verifypeer);
  curl_easy_setopt(curl, CURLOPT_URL, RSTRING_PTR(url));
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
  method = mrb_funcall(mrb, req, "method", 0, NULL);
  if (strcmp("GET", RSTRING_PTR(method))) {
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    body = mrb_funcall(mrb, req, "body", 0, NULL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, RSTRING_PTR(body));
  }
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, mf);
  if (mrb_nil_p(b)) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
  } else {
    mf->mrb = mrb;
    mf->proc = b;
    mf->header = mrb_nil_value();
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite_callback);
  }
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, mf);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, memfwrite);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);

  headers = mrb_funcall(mrb, req, "headers", 0, NULL);
  if (!mrb_nil_p(headers)) {
    mrb_value keys = mrb_hash_keys(mrb, headers);
    int i, l = RARRAY_LEN(keys);
    for (i = 0; i < l; i++) {
      mrb_value key = mrb_ary_entry(keys, i);
      mrb_value header = mrb_str_dup(mrb, key);
      mrb_str_cat2(mrb, header, ": ");
      mrb_str_concat(mrb, header, mrb_hash_get(mrb, headers, key));
      headerlist = curl_slist_append(headerlist, RSTRING_PTR(header));
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
  }

  res = curl_easy_perform(curl);

  if (headerlist)
    curl_slist_free_all(headerlist);

  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    mrb_raise(mrb, E_RUNTIME_ERROR, error);
  }
  if (!mrb_nil_p(b)) {
    return mrb_nil_value();
  }

  str = mrb_str_new(mrb, mf->data, mf->size);
  memfclose(mf);

  _class_http = mrb_module_get(mrb, "HTTP");
  _class_http_parser = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(_class_http), mrb_intern_cstr(mrb, "Parser")));
  parser = mrb_obj_new(mrb, _class_http_parser, 0, NULL);
  args[0] = str;
  return mrb_funcall_argv(mrb, parser, mrb_intern_cstr(mrb, "parse_response"), 1, args);
}

void
mrb_mruby_curl_gem_init(mrb_state* mrb)
{
  struct RClass* _class_curl;
  int ai = mrb_gc_arena_save(mrb); \
  _class_curl = mrb_define_module(mrb, "Curl");
  mrb_define_module_function(mrb, _class_curl, "get", mrb_curl_get, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
  mrb_define_module_function(mrb, _class_curl, "post", mrb_curl_post, MRB_ARGS_REQ(2) | MRB_ARGS_OPT(1));
  mrb_define_module_function(mrb, _class_curl, "send", mrb_curl_send, MRB_ARGS_REQ(2));
  mrb_define_const(mrb, _class_curl, "SSL_VERIFYPEER", mrb_fixnum_value(1));
  mrb_gc_arena_restore(mrb, ai);
}

void
mrb_mruby_curl_gem_final(mrb_state* mrb)
{
}

/* vim:set et ts=2 sts=2 sw=2 tw=0: */
