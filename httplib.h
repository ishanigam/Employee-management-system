//
//  httplib.h
//
//  Copyright (c) 2025 Yuji Hirose. All rights reserved.
//  MIT License
//

#ifndef CPPHTTPLIB_HTTPLIB_H
#define CPPHTTPLIB_HTTPLIB_H

#define CPPHTTPLIB_VERSION "0.15.3"
#define CPPHTTPLIB_VERSION_MAJOR 0
#define CPPHTTPLIB_VERSION_MINOR 15
#define CPPHTTPLIB_VERSION_PATCH 3

/*
 * Configuration
 */

#ifndef CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND
#define CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_KEEPALIVE_MAX_COUNT
#define CPPHTTPLIB_KEEPALIVE_MAX_COUNT 5
#endif

#ifndef CPPHTTPLIB_CONNECTION_TIMEOUT_SECOND
#define CPPHTTPLIB_CONNECTION_TIMEOUT_SECOND 300
#endif

#ifndef CPPHTTPLIB_CONNECTION_TIMEOUT_USECOND
#define CPPHTTPLIB_CONNECTION_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_READ_TIMEOUT_SECOND
#define CPPHTTPLIB_READ_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_READ_TIMEOUT_USECOND
#define CPPHTTPLIB_READ_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_WRITE_TIMEOUT_SECOND
#define CPPHTTPLIB_WRITE_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_WRITE_TIMEOUT_USECOND
#define CPPHTTPLIB_WRITE_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_IDLE_INTERVAL_SECOND
#define CPPHTTPLIB_IDLE_INTERVAL_SECOND 0
#endif

#ifndef CPPHTTPLIB_IDLE_INTERVAL_USECOND
#define CPPHTTPLIB_IDLE_INTERVAL_USECOND 0
#endif

#ifndef CPPHTTPLIB_REQUEST_URI_MAX_LENGTH
#define CPPHTTPLIB_REQUEST_URI_MAX_LENGTH 8192
#endif

#ifndef CPPHTTPLIB_HEADER_MAX_LENGTH
#define CPPHTTPLIB_HEADER_MAX_LENGTH 8192
#endif

#ifndef CPPHTTPLIB_REDIRECT_MAX_COUNT
#define CPPHTTPLIB_REDIRECT_MAX_COUNT 20
#endif

#ifndef CPPHTTPLIB_MULTIPART_FORM_DATA_FILE_MAX_COUNT
#define CPPHTTPLIB_MULTIPART_FORM_DATA_FILE_MAX_COUNT 1024
#endif

#ifndef CPPHTTPLIB_PAYLOAD_MAX_LENGTH
#define CPPHTTPLIB_PAYLOAD_MAX_LENGTH ((std::numeric_limits<size_t>::max)())
#endif

#ifndef CPPHTTPLIB_FORM_URL_ENCODED_PAYLOAD_MAX_LENGTH
#define CPPHTTPLIB_FORM_URL_ENCODED_PAYLOAD_MAX_LENGTH 8192
#endif

#ifndef CPPHTTPLIB_TCP_NODELAY
#define CPPHTTPLIB_TCP_NODELAY false
#endif

#ifndef CPPHTTPLIB_RECV_BUFSIZ
#define CPPHTTPLIB_RECV_BUFSIZ size_t(4096u)
#endif

#ifndef CPPHTTPLIB_COMPRESSION_BUFSIZ
#define CPPHTTPLIB_COMPRESSION_BUFSIZ size_t(16384u)
#endif

#ifndef CPPHTTPLIB_THREAD_POOL_COUNT
#define CPPHTTPLIB_THREAD_POOL_COUNT                                           \
  ((std::max)(8u, std::thread::hardware_concurrency() > 0                     \
                      ? std::thread::hardware_concurrency() - 1               \
                      : 0))
#endif

#ifndef CPPHTTPLIB_RECV_FLAGS
#define CPPHTTPLIB_RECV_FLAGS 0
#endif

#ifndef CPPHTTPLIB_SEND_FLAGS
#define CPPHTTPLIB_SEND_FLAGS 0
#endif

#ifndef CPPHTTPLIB_LISTEN_BACKLOG
#define CPPHTTPLIB_LISTEN_BACKLOG 5
#endif

/*
 * Headers
 */

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <fcntl.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef WSA_FLAG_NO_HANDLE_INHERIT
#define WSA_FLAG_NO_HANDLE_INHERIT 0x80
#endif

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif // strcasecmp

using socket_t = SOCKET;
#ifdef CPPHTTPLIB_USE_POLL
#define poll(fds, nfds, timeout) WSAPoll(fds, nfds, timeout)
#endif

#else // not _WIN32

#include <arpa/inet.h>
#if !defined(_AIX) && !defined(__MVS__)
#include <ifaddrs.h>
#endif
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef __linux__
#include <resolv.h>
#endif
#include <netinet/tcp.h>
#ifdef CPPHTTPLIB_USE_POLL
#include <poll.h>
#endif
#include <csignal>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

using socket_t = int;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#endif //_WIN32

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cctype>
#include <climits>
#include <condition_variable>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#ifdef _WIN32
#include <wincrypt.h>

// these are defined in wincrypt.h and it breaks compilation if BoringSSL is
// used
#undef X509_NAME
#undef X509_CERT_PAIR
#undef X509_EXTENSIONS
#undef PKCS7_SIGNER_INFO

#ifdef _MSC_VER
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "cryptui.lib")
#endif
#elif defined(CPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN) && defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#endif // TARGET_OS_OSX
#endif // _WIN32

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#if defined(_WIN32) && defined(OPENSSL_USE_APPLINK)
#include <openssl/applink.c>
#endif

#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER < 0x1010100fL
#error Sorry, OpenSSL versions prior to 1.1.1 are not supported
#elif OPENSSL_VERSION_NUMBER < 0x10101000L
#define SSL_CTX_set_ciphersuites(ctx, str) \
  SSL_CTX_set_cipher_list(ctx, str)
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#include <openssl/crypto.h>
inline const unsigned char *ASN1_STRING_get0_data(const ASN1_STRING *asn1) {
  return M_ASN1_STRING_data(asn1);
}
inline int ASN1_STRING_length(const ASN1_STRING *asn1) {
  return M_ASN1_STRING_length(asn1);
}
inline int SSL_CTX_set_min_proto_version(SSL_CTX *ctx, int version) {
  return SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                                      SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
}
#endif
#endif

#ifdef CPPHTTPLIB_ZLIB_SUPPORT
#include <zlib.h>
#endif

#ifdef CPPHTTPLIB_BROTLI_SUPPORT
#include <brotli/decode.h>
#include <brotli/encode.h>
#endif

#ifdef CPPHTTPLIB_ZSTD_SUPPORT
#include <zstd.h>
#endif

/*
 * Declaration
 */
namespace httplib {

namespace detail {

/*
 * Backport std::make_unique from C++14.
 *
 * NOTE: This code came up with the following stackoverflow post:
 * https://stackoverflow.com/questions/10149840/c-arrays-and-make-unique
 *
 */

template <class T, class... Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(Args &&...args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
typename std::enable_if<std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(std::size_t n) {
  typedef typename std::remove_extent<T>::type RT;
  return std::unique_ptr<T>(new RT[n]);
}

struct ci {
  bool operator()(const std::string &s1, const std::string &s2) const {
    return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(),
                                        s2.end(),
                                        [](unsigned char c1, unsigned char c2) {
                                          return ::tolower(c1) < ::tolower(c2);
                                        });
  }
};

} // namespace detail

using Headers = std::multimap<std::string, std::string, detail::ci>;

using Params = std::multimap<std::string, std::string>;
using Match = std::smatch;

using Progress = std::function<bool(uint64_t current, uint64_t total)>;

struct Response;
using ResponseHandler = std::function<bool(const Response &response)>;

struct MultipartFormData {
  std::string name;
  std::string content;
  std::string filename;
  std::string content_type;
};
using MultipartFormDataItems = std::vector<MultipartFormData>;

using Range = std::pair<ssize_t, ssize_t>;
using Ranges = std::vector<Range>;

struct Request {
  std::string method;
  std::string path;
  Headers headers;
  std::string body;

  std::string remote_addr;
  int remote_port = -1;
  std::string local_addr;
  int local_port = -1;

  // for server
  std::string version;
  std::string target;
  Params params;
  MultipartFormDataItems files;
  Ranges ranges;
  Match matches;

  // for client
  ResponseHandler response_handler;
  Progress progress;

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  const SSL *ssl = nullptr;
#endif

  bool has_header(const std::string &key) const;
  std::string get_header_value(const std::string &key, size_t id = 0) const;
  template <typename T>
  T get_header_value(const std::string &key, size_t id = 0) const;
  size_t get_header_value_count(const std::string &key) const;
  void set_header(const std::string &key, const std::string &val);
  void set_header(const std::string &key, const char *val);

  bool has_param(const std::string &key) const;
  std::string get_param_value(const std::string &key, size_t id = 0) const;
  size_t get_param_value_count(const std::string &key) const;

  bool is_multipart_form_data() const;

  bool has_file(const std::string &key) const;
  MultipartFormData get_file_value(const std::string &key) const;
  std::vector<MultipartFormData> get_file_values(const std::string &key) const;

  // private members...
  size_t redirect_count_ = CPPHTTPLIB_REDIRECT_MAX_COUNT;
  size_t content_length_ = 0;
  ContentProvider content_provider_;
  bool is_chunked_content_provider_ = false;
  size_t authorization_count_ = 0;
};

struct Response {
  std::string version;
  int status = -1;
  std::string reason;
  Headers headers;
  std::string body;
  std::string location; // Redirect location

  bool has_header(const std::string &key) const;
  std::string get_header_value(const std::string &key, size_t id = 0) const;
  template <typename T>
  T get_header_value(const std::string &key, size_t id = 0) const;
  size_t get_header_value_count(const std::string &key) const;
  void set_header(const std::string &key, const std::string &val);
  void set_header(const std::string &key, const char *val);

  void set_redirect(const std::string &url, int status = 302);
  void set_content(const std::string &s, const std::string &content_type);
  void set_content(const char *s, size_t n, const std::string &content_type);
  void set_content(const char *s, const std::string &content_type);

  void set_chunked_content_provider(
      const std::string &content_type,
      ContentProviderWithoutLength provider,
      ContentProviderResourceReleaser resource_releaser = nullptr);

  void set_content_provider(
      size_t length, const std::string &content_type, ContentProvider provider,
      ContentProviderResourceReleaser resource_releaser = nullptr);

  void set_content_provider(
      const std::string &content_type, ContentProviderWithoutLength provider,
      ContentProviderResourceReleaser resource_releaser = nullptr);

  // private members...
  size_t content_length_ = 0;
  ContentProvider content_provider_;
  bool is_chunked_content_provider_ = false;
  ContentProviderResourceReleaser content_provider_resource_releaser_;
  bool is_content_provider_resource_released_ = false;
};

class Stream {
public:
  virtual ~Stream() = default;

  virtual bool is_readable() const = 0;
  virtual bool is_writable() const = 0;

  virtual ssize_t read(char *ptr, size_t size) = 0;
  virtual ssize_t write(const char *ptr, size_t size) = 0;
  virtual void get_remote_ip_and_port(std::string &ip, int &port) const = 0;
  virtual void get_local_ip_and_port(std::string &ip, int &port) const = 0;
  virtual socket_t socket() const = 0;

  template <typename... Args>
  ssize_t write_format(const char *fmt, const Args &...args);
  ssize_t write(const char *ptr);
  ssize_t write(const std::string &s);
};

class TaskQueue {
public:
  TaskQueue() = default;
  virtual ~TaskQueue() = default;

  virtual bool enqueue(std::function<void()> fn) = 0;
  virtual void shutdown() = 0;

  virtual void on_idle(){};
};

class ThreadPool : public TaskQueue {
public:
  explicit ThreadPool(size_t n) : shutdown_(false) {
    while (n) {
      threads_.emplace_back(worker(*this));
      n--;
    }
  }

  ThreadPool(const ThreadPool &) = delete;
  ~ThreadPool() override = default;

  bool enqueue(std::function<void()> fn) override {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (shutdown_) { return false; }
      jobs_.push_back(std::move(fn));
    }
    cond_.notify_one();
    return true;
  }

  void shutdown() override {
    // Stop all worker threads...
    {
      std::unique_lock<std::mutex> lock(mutex_);
      shutdown_ = true;
    }
    cond_.notify_all();

    // Join...
    for (auto &t : threads_) {
      t.join();
    }
  }

private:
  struct worker {
    explicit worker(ThreadPool &pool) : pool_(pool) {}

    void operator()() {
      for (;;) {
        std::function<void()> job;
        {
          std::unique_lock<std::mutex> lock(pool_.mutex_);
          pool_.cond_.wait(
              lock, [&] { return !pool_.jobs_.empty() || pool_.shutdown_; });
          if (pool_.shutdown_ && pool_.jobs_.empty()) { break; }
          job = std::move(pool_.jobs_.front());
          pool_.jobs_.pop_front();
        }
        job();
      }
    }

    ThreadPool &pool_;
  };
  friend struct worker;

  std::vector<std::thread> threads_;
  std::list<std::function<void()>> jobs_;

  bool shutdown_;

  std::condition_variable cond_;
  std::mutex mutex_;
};

using Logger = std::function<void(const Request &, const Response &)>;

using SocketOptions = std::function<void(socket_t sock)>;

void default_socket_options(socket_t sock);

const char *status_message(int status);

std::string get_bearer_token_auth(const std::string &token);

template <class Rep, class Period>
constexpr std::chrono::duration<Rep, Period> operator""_t(
    unsigned long long val) {
  return std::chrono::duration<Rep, Period>{static_cast<Rep>(val)};
}

namespace detail {

std::string encode_query_param(const std::string &value);
std::string decode_url(const std::string &s, bool decode_plus_as_space);
std::string encode_url(const std::string &s);
std::wstring encode_url(const std::wstring &s);

void read_file(const std::string &path, std::string &out);
std::string file_extension(const std::string &path);

bool is_space_or_tab(char c);
bool is_hex(char c, int &v);
bool from_hex_to_i(const std::string &s, size_t i, size_t cnt,
                   int &val);
std::string from_i_to_hex(size_t n);
size_t to_utf8(int code, char *buff);
int from_utf8(const char *buff, size_t len, size_t &bytes);
std::string base64_encode(const std::string &in);
std::string base64_decode(const std::string &in);

void get_random_string(char *buff, size_t length);
std::string MD5(const std::string &);
std::string SHA_256(const std::string &);
std::string SHA_512(const std::string &);

#ifdef _WIN32
class WSInit {
public:
  WSInit();
  ~WSInit();

private:
  WSADATA wsa_data_;
};
#endif

} // namespace detail

// HTTP server implementation
class Server {
public:
  using Handler = std::function<void(const Request &, Response &)>;

  using ExceptionHandler = std::function<void(const Request &, Response &,
                                              std::exception_ptr ep)>;

  using PreRoutingHandler = std::function<Server::HandlerResponse(
      const Request &, Response &)>;
  using PostRoutingHandler = std::function<Server::HandlerResponse(
      const Request &, Response &)>;

  enum class HandlerResponse { Handled, Unhandled };

  Server();

  virtual ~Server();

  virtual bool is_valid() const;

  Server &Get(const std::string &pattern, Handler handler);
  Server &Post(const std::string &pattern, Handler handler);
  Server &Put(const std::string &pattern, Handler handler);
  Server &Patch(const std::string &pattern, Handler handler);
  Server &Delete(const std::string &pattern, Handler handler);
  Server &Options(const std::string &pattern, Handler handler);

  bool remove_mount_point(const std::string &mount_point);
  Server &set_mount_point(const std::string &mount_point,
                          const std::string &dir);
  bool mount_builtin_editor(const std::string &pattern);

  Server &set_file_extension_and_mimetype_mapping(const std::string &ext,
                                                  const std::string &mime);
  Server &set_default_file_mimetype(const std::string &mime);
  Server &set_file_request_handler(Handler handler);

  Server &set_error_handler(HandlerWithResponse handler);
  Server &set_error_handler(Handler handler);
  Server &set_exception_handler(ExceptionHandler handler);
  Server &set_pre_routing_handler(PreRoutingHandler handler);
  Server &set_post_routing_handler(PostRoutingHandler handler);

  Server &set_expect_100_continue_handler(Expect100ContinueHandler handler);
  Server &set_logger(Logger logger);

  Server &set_address_family(int family);
  Server &set_tcp_nodelay(bool on);
  Server &set_socket_options(SocketOptions socket_options);

  Server &set_default_headers(Headers headers);

  Server &set_header_writer(
      std::function<ssize_t(Stream &, Headers &)> const &writer);

  Server &set_keep_alive_max_count(size_t count);
  Server &set_keep_alive_timeout(time_t sec);

  Server &set_read_timeout(time_t sec, time_t usec = 0);
  Server &set_write_timeout(time_t sec, time_t usec = 0);
  Server &set_idle_interval(time_t sec, time_t usec = 0);

  Server &set_payload_max_length(size_t length);

  bool bind_to_port(const std::string &host, int port, int socket_flags = 0);
  int bind_to_any_port(const std::string &host, int socket_flags = 0);
  bool listen_after_bind();

  bool listen(const std::string &host, int port, int socket_flags = 0);

  bool is_running() const;
  void wait_until_ready() const;
  void stop();

  std::function<TaskQueue *(void)> new_task_queue;

protected:
  bool process_request(Stream &strm, bool close_connection,
                       bool &connection_closed,
                       const std::function<void(Request &)> &setup_request);

  std::atomic<socket_t> svr_sock_{INVALID_SOCKET};
  size_t keep_alive_max_count_ = CPPHTTPLIB_KEEPALIVE_MAX_COUNT;
  time_t keep_alive_timeout_sec_ = CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND;
  time_t read_timeout_sec_ = CPPHTTPLIB_READ_TIMEOUT_SECOND;
  time_t read_timeout_usec_ = CPPHTTPLIB_READ_TIMEOUT_USECOND;
  time_t write_timeout_sec_ = CPPHTTPLIB_WRITE_TIMEOUT_SECOND;
  time_t write_timeout_usec_ = CPPHTTPLIB_WRITE_TIMEOUT_USECOND;
  time_t idle_interval_sec_ = CPPHTTPLIB_IDLE_INTERVAL_SECOND;
  time_t idle_interval_usec_ = CPPHTTPLIB_IDLE_INTERVAL_USECOND;
  size_t payload_max_length_ = CPPHTTPLIB_PAYLOAD_MAX_LENGTH;

private:
  using Handlers =
      std::vector<std::pair<std::regex, Handler>>;
  using HandlersForContentReader =
      std::vector<std::pair<std::regex, HandlerWithContentReader>>;

  socket_t create_server_socket(const std::string &host, int port,
                                int socket_flags,
                                SocketOptions socket_options) const;
  int bind_internal(const std::string &host, int port, int socket_flags);
  bool listen_internal();

  bool routing(Request &req, Response &res, Stream &strm);
  bool handle_file_request(const Request &req, Response &res,
                           bool head = false);
  bool dispatch_request(Request &req, Response &res, const Handlers &handlers);

  bool dispatch_request_for_content_reader(
      Request &req, Response &res, ContentReader content_reader,
      const HandlersForContentReader &handlers);

  bool parse_request_line(const char *s, Request &req);
  void apply_ranges(const Request &req, Response &res,
                    std::string &content_type, std::string &boundary);
  bool write_response(Stream &strm, bool close_connection, const Request &req,
                      Response &res);
  bool write_response_with_content(Stream &strm, bool close_connection,
                                   const Request &req, Response &res);
  bool write_response_core(Stream &strm, bool close_connection,
                           const Request &req, Response &res,
                           bool need_apply_ranges);
  bool write_content_with_provider(Stream &strm, const Request &req,
                                   Response &res, const std::string &boundary,
                                   const std::string &content_type);
  bool read_content(Stream &strm, Request &req, Response &res);
  bool read_content_with_content_receiver(
      Stream &strm, Request &req, Response &res, ContentReceiver receiver,
      MultipartContentHeader multipart_header,
      ContentReceiver multipart_receiver);
  bool read_content_core(Stream &strm, Request &req, Response &res,
                         ContentReceiver receiver,
                         MultipartContentHeader multipart_header,
                         ContentReceiver multipart_receiver);

  virtual bool process_and_close_socket(socket_t sock);

  struct MountPointEntry {
    std::string mount_point;
    std::string base_dir;
    Headers headers;
  };
  std::vector<MountPointEntry> base_dirs_;

  std::atomic<bool> is_running_{false};
  std::atomic<bool> done_{false};

  std::map<std::string, std::string> file_extension_and_mimetype_map_;
  std::string default_file_mimetype_ = "application/octet-stream";
  Handler file_request_handler_;

  Handlers get_handlers_;
  Handlers post_handlers_;
  Handlers put_handlers_;
  Handlers patch_handlers_;
  Handlers delete_handlers_;
  Handlers options_handlers_;

  HandlersForContentReader post_handlers_for_content_reader_;
  HandlersForContentReader put_handlers_for_content_reader_;
  HandlersForContentReader patch_handlers_for_content_reader_;

  HandlerWithResponse error_handler_;
  ExceptionHandler exception_handler_;
  PreRoutingHandler pre_routing_handler_;
  PostRoutingHandler post_routing_handler_;

  Expect100ContinueHandler expect_100_continue_handler_;

  Logger logger_;

  int address_family_ = AF_UNSPEC;
  bool tcp_nodelay_ = CPPHTTPLIB_TCP_NODELAY;
  SocketOptions socket_options_ = default_socket_options;

  Headers default_headers_;
  std::function<ssize_t(Stream &, Headers &)> header_writer_ =
      detail::write_headers;
};

} // namespace httplib

#endif // CPPHTTPLIB_HTTPLIB_H
