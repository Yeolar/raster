/*
 * tls.h -- Abstracts pthread and Windows thread-local storage mechanisms
 */

#pragma once

#ifndef NO_TLS

#include <pthread.h>

#define TLS_KEY             pthread_key_t
#define TLS_CREATE_KEY      ::srilm::tls_get_key
#define TLS_GET(key)        pthread_getspecific(key)
#define TLS_SET(key, value) pthread_setspecific(key, value)
#define TLS_FREE_KEY(key)   pthread_key_delete(key)

namespace srilm {

inline TLS_KEY tls_get_key() {
  TLS_KEY key;
  pthread_key_create(&key, 0);
  return key;
}

}

#endif /* USE_TLS */

