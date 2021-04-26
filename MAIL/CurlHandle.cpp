#include "CurlHandle.h"

#include <curl/curl.h>
#include <stdexcept>

CurlHandle::CurlHandle() {
   const auto eCode = curl_global_init(CURL_GLOBAL_ALL);
   if (eCode != CURLE_OK) {
      throw std::runtime_error{"Error initializing libCURL"};
   }
}

CurlHandle::~CurlHandle() { curl_global_cleanup(); }

CurlHandle &CurlHandle::instance() {
   static CurlHandle inst{};
   return inst;
}
