#pragma once

#define LUAX_WEB_INT_ENUM_ENTRY(Enum, name) {#name, static_cast<int>(Enum::name)},

#define LUAX_WEB_HTTP_VERSION(X)                   \
    X(web::HttpVersion, DEFAULT)                   \
    X(web::HttpVersion, VERSION_1_0)               \
    X(web::HttpVersion, VERSION_1_1)               \
    X(web::HttpVersion, VERSION_2_0)               \
    X(web::HttpVersion, VERSION_2TLS)              \
    X(web::HttpVersion, VERSION_2_PRIOR_KNOWLEDGE) \
    X(web::HttpVersion, VERSION_3)                 \
    X(web::HttpVersion, VERSION_3ONLY)

#define LUAX_WEB_PROXY_TYPE(X) \
    X(web::ProxyType, HTTP)    \
    X(web::ProxyType, HTTPS)   \
    X(web::ProxyType, HTTPS2)  \
    X(web::ProxyType, SOCKS4)  \
    X(web::ProxyType, SOCKS4A) \
    X(web::ProxyType, SOCKS5)  \
    X(web::ProxyType, SOCKS5H)

#define LUAX_WEB_GEODE_WEB_ERROR(X)                  \
    X(web::GeodeWebError, CURL_INITIALIZATION_ERROR) \
    X(web::GeodeWebError, REQUEST_CANCELLED)         \
    X(web::GeodeWebError, QUEUE_FULL)                \
    X(web::GeodeWebError, CHANNEL_CLOSED)

#define LUAX_WEB_LONG_ENUM_ENTRY(Ns, name) {#name, Ns::name},

#define LUAX_WEB_HTTP_AUTH(X)    \
    X(web::http_auth, BASIC)     \
    X(web::http_auth, DIGEST)    \
    X(web::http_auth, DIGEST_IE) \
    X(web::http_auth, BEARER)    \
    X(web::http_auth, NEGOTIATE) \
    X(web::http_auth, NTLM)      \
    X(web::http_auth, NTLM_WB)   \
    X(web::http_auth, ANY)       \
    X(web::http_auth, ANYSAFE)   \
    X(web::http_auth, ONLY)      \
    X(web::http_auth, AWS_SIGV4)
