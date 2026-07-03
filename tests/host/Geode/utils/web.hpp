#pragma once

#include <Geode/Result.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/async.hpp>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <matjson.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace geode {
    using ByteVector = std::vector<std::uint8_t>;
    using ZStringView = std::string_view;

    template <typename Signature>
    using Function = std::function<Signature>;

    class ListenerHandle {
    public:
        ListenerHandle() = default;

        explicit ListenerHandle(std::function<void()> disconnect) :
            m_disconnect(std::move(disconnect)) {}

        ListenerHandle(ListenerHandle&&) = default;
        ListenerHandle& operator=(ListenerHandle&&) = default;
        ListenerHandle(ListenerHandle const&) = delete;
        ListenerHandle& operator=(ListenerHandle const&) = delete;

        ~ListenerHandle() {
            disconnect();
        }

        void disconnect() {
            if (m_disconnect) {
                m_disconnect();
                m_disconnect = nullptr;
            }
        }

    private:
        std::function<void()> m_disconnect;
    };

    inline Mod* getMod() {
        return Mod::get();
    }
} // namespace geode

namespace geode::utils {
    template <typename T>
    using StringMap = std::unordered_map<std::string, T>;
} // namespace geode::utils

namespace asp {
    class Duration {
    public:
        Duration() = default;

        template <class Rep, class Period>
        Duration(std::chrono::duration<Rep, Period> value) :
            m_value(std::chrono::duration_cast<std::chrono::milliseconds>(value)) {}

        template <class Rep = double>
        Rep millis() const {
            return std::chrono::duration_cast<std::chrono::duration<Rep, std::milli>>(m_value).count();
        }

    private:
        std::chrono::milliseconds m_value{};
    };
} // namespace asp

namespace geode::utils::web {
    using geode::ByteVector;
    using geode::Err;
    using geode::Ok;
    using geode::Result;

    namespace http_auth {
        constexpr static long BASIC = 0x0001;
        constexpr static long DIGEST = 0x0002;
        constexpr static long DIGEST_IE = 0x0004;
        constexpr static long BEARER = 0x0008;
        constexpr static long NEGOTIATE = 0x0010;
        constexpr static long NTLM = 0x0020;
        constexpr static long NTLM_WB = 0x0040;
        constexpr static long ANY = 0x0080;
        constexpr static long ANYSAFE = 0x0100;
        constexpr static long ONLY = 0x0200;
        constexpr static long AWS_SIGV4 = 0x0400;
    } // namespace http_auth

    enum class HttpVersion {
        DEFAULT,
        VERSION_1_0,
        VERSION_1_1,
        VERSION_2_0,
        VERSION_2TLS,
        VERSION_2_PRIOR_KNOWLEDGE,
        VERSION_3 = 30,
        VERSION_3ONLY = 31,
    };

    enum class ProxyType {
        HTTP,
        HTTPS,
        HTTPS2,
        SOCKS4,
        SOCKS4A,
        SOCKS5,
        SOCKS5H
    };

    enum class GeodeWebError {
        CURL_INITIALIZATION_ERROR = -999,
        REQUEST_CANCELLED = -998,
        QUEUE_FULL = -997,
        CHANNEL_CLOSED = -996,
    };

    struct ProxyOpts {
        std::string address;
        std::optional<std::uint16_t> port;
        ProxyType type = ProxyType::HTTP;
        long auth = http_auth::BASIC;
        std::string username;
        std::string password;
        bool tunneling = false;
        bool certVerification = true;
    };

    struct RequestTimings final {
        asp::Duration queueWait;
        asp::Duration nameLookup;
        asp::Duration connect;
        asp::Duration tlsHandshake;
        asp::Duration requestSend;
        asp::Duration firstByte;
        asp::Duration download;
        asp::Duration total;
    };

    class WebProgress final {
    public:
        std::size_t downloaded() const {
            return m_downloadCurrent;
        }

        std::size_t downloadTotal() const {
            return m_downloadTotal;
        }

        std::optional<float> downloadProgress() const {
            return downloadTotal() > 0 ?
                std::optional<float>(
                    static_cast<float>(downloaded()) * 100.f / static_cast<float>(downloadTotal())
                ) :
                std::nullopt;
        }

        std::size_t uploaded() const {
            return m_uploadCurrent;
        }

        std::size_t uploadTotal() const {
            return m_uploadTotal;
        }

        std::optional<float> uploadProgress() const {
            return uploadTotal() > 0 ?
                std::optional<float>(
                    static_cast<float>(uploaded()) * 100.f / static_cast<float>(uploadTotal())
                ) :
                std::nullopt;
        }

    private:
        friend class WebRequest;
        std::size_t m_downloadCurrent = 0;
        std::size_t m_downloadTotal = 0;
        std::size_t m_uploadCurrent = 0;
        std::size_t m_uploadTotal = 0;
    };

    class WebResponse;
    class WebRequest;

    namespace test {
        using ResponseFactory = std::function<WebResponse(
            WebRequest const& request, std::string_view method, std::string_view url, Mod* mod
        )>;

        inline ResponseFactory& responseFactory() {
            static ResponseFactory factory;
            return factory;
        }

        inline void setResponseFactory(ResponseFactory factory) {
            responseFactory() = std::move(factory);
        }

        inline void resetResponseFactory() {
            responseFactory() = {};
        }

        inline std::atomic<std::size_t>& sendCount() {
            static std::atomic<std::size_t> count = 0;
            return count;
        }

        inline void resetSendCount() {
            sendCount() = 0;
        }

        WebResponse makeDefaultResponse();
        WebResponse makeResponseWithBody(ByteVector data);
    } // namespace test

    class WebResponse final {
    public:
        bool info() const {
            return m_hasInfo;
        }

        bool ok() const {
            return m_code >= 200 && m_code < 300;
        }

        bool redirected() const {
            return m_code >= 300 && m_code < 400;
        }

        bool badClient() const {
            return m_code >= 400 && m_code < 500;
        }

        bool badServer() const {
            return m_code >= 500 && m_code < 600;
        }

        bool error() const {
            return m_errored;
        }

        bool cancelled() const {
            return m_cancelled;
        }

        int code() const {
            return m_code;
        }

        Result<std::string> string() const {
            if (m_stringError) return Err(*m_stringError);
            return Ok(std::string(reinterpret_cast<char const*>(m_data.data()), m_data.size()));
        }

        Result<matjson::Value> json() const {
            auto text = string();
            if (text.isErr()) return Err(text.unwrapErr());
            return matjson::Value::parse(text.unwrap());
        }

        ByteVector const& data() const& {
            return m_data;
        }

        ByteVector data() && {
            return std::move(m_data);
        }

        Result<void> into(std::filesystem::path const& path) const {
            std::error_code ec;
            auto parent = path.parent_path();
            if (!parent.empty()) {
                std::filesystem::create_directories(parent, ec);
                if (ec) return Err("failed to create parent directory");
            }
            std::ofstream out(path, std::ios::binary);
            if (!out) return Err("failed to open output file");
            if (!m_data.empty()) {
                out.write(
                    reinterpret_cast<char const*>(m_data.data()),
                    static_cast<std::streamsize>(m_data.size())
                );
            }
            return out ? Ok() : Err("failed to write output file");
        }

        std::vector<std::string> headers() const {
            std::vector<std::string> flat;
            flat.reserve(m_headers.size());
            for (auto const& [name, values] : m_headers) {
                for (auto const& value : values)
                    flat.push_back(name + ": " + value);
            }
            return flat;
        }

        std::optional<std::string> header(std::string_view name) const {
            auto it = m_headers.find(std::string(name));
            if (it == m_headers.end() || it->second.empty()) return std::nullopt;
            return it->second.front();
        }

        std::optional<std::vector<std::string>> getAllHeadersNamed(std::string_view name) const {
            auto it = m_headers.find(std::string(name));
            return it == m_headers.end() ? std::nullopt :
                                           std::optional<std::vector<std::string>>{it->second};
        }

        std::string_view errorMessage() const {
            return m_errorMessage;
        }

        std::string_view verboseLogs() const {
            return m_verboseLogs;
        }

        RequestTimings const& timings() const {
            return m_timings;
        }

    private:
        friend class WebRequest;
        friend WebResponse test::makeDefaultResponse();
        friend WebResponse test::makeResponseWithBody(ByteVector data);

        bool m_hasInfo = true;
        bool m_errored = false;
        bool m_cancelled = false;
        int m_code = 200;
        ByteVector m_data;
        std::unordered_map<std::string, std::vector<std::string>> m_headers;
        std::string m_errorMessage;
        std::string m_verboseLogs;
        std::optional<std::string> m_stringError;
        RequestTimings m_timings;
    };

    inline WebResponse test::makeDefaultResponse() {
        WebResponse response;
        response.m_code = 200;
        response.m_data = {'O', 'K'};
        response.m_headers["content-type"] = {"text/plain"};
        return response;
    }

    inline WebResponse test::makeResponseWithBody(ByteVector data) {
        WebResponse response;
        response.m_data = std::move(data);
        response.m_headers["content-type"] = {"application/octet-stream"};
        return response;
    }

    struct WebFuture {
        explicit WebFuture(std::function<WebResponse()> producer = {}) :
            m_producer(std::move(producer)) {}

        WebFuture(WebFuture&&) noexcept = default;
        WebFuture& operator=(WebFuture&&) noexcept = delete;
        WebFuture(WebFuture const&) = delete;
        WebFuture& operator=(WebFuture const&) = delete;

        WebResponse resolve() && {
            return m_producer ? m_producer() : test::makeDefaultResponse();
        }

    private:
        std::function<WebResponse()> m_producer;
    };

    class MultipartForm final {
    public:
        MultipartForm& param(std::string name, std::string value) {
            m_dirty = true;
            m_fields.push_back({std::move(name), std::move(value), {}, {}, {}});
            return *this;
        }

        MultipartForm& file(
            std::string name, std::span<std::uint8_t const> data, std::string filename,
            std::string mime = "application/octet-stream"
        ) {
            m_dirty = true;
            m_fields.push_back(
                {std::move(name),
                 {},
                 std::move(filename),
                 std::move(mime),
                 ByteVector(data.begin(), data.end())}
            );
            return *this;
        }

        Result<void> file(
            std::string name, std::filesystem::path const& path,
            std::string mime = "application/octet-stream"
        ) {
            std::ifstream in(path, std::ios::binary);
            if (!in) return Err("failed to read multipart file");
            ByteVector bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            file(
                std::move(name),
                std::span<std::uint8_t const>(bytes.data(), bytes.size()),
                path.filename().string(),
                std::move(mime)
            );
            return Ok();
        }

        std::string_view getBoundary() const {
            rebuild();
            return m_boundary;
        }

        std::string getHeader() const {
            rebuild();
            return "multipart/form-data; boundary=" + m_boundary;
        }

        ByteVector getBody() const {
            rebuild();
            return m_body;
        }

    private:
        struct Field {
            std::string name;
            std::string value;
            std::string filename;
            std::string mime;
            ByteVector bytes;
        };

        void rebuild() const {
            if (!m_dirty) return;
            if (m_boundary.empty()) m_boundary = "----LuauAPIHostStubBoundary";
            std::string body;
            for (auto const& field : m_fields) {
                body += "--" + m_boundary + "\r\n";
                if (field.filename.empty()) {
                    body += "Content-Disposition: form-data; name=\"" + field.name + "\"\r\n\r\n" +
                        field.value;
                }
                else {
                    body += "Content-Disposition: form-data; name=\"" + field.name +
                        "\"; filename=\"" + field.filename + "\"\r\nContent-Type: " + field.mime +
                        "\r\n\r\n";
                    body.append(reinterpret_cast<char const*>(field.bytes.data()), field.bytes.size());
                }
                body += "\r\n";
            }
            body += "--" + m_boundary + "--\r\n";
            m_body.assign(body.begin(), body.end());
            m_dirty = false;
        }

        mutable std::string m_boundary;
        mutable ByteVector m_body;
        mutable bool m_dirty = false;
        std::vector<Field> m_fields;
    };

    class WebRequest final {
    public:
        WebRequest() : m_id(nextID()) {}

        WebFuture send(std::string method, std::string url, Mod* mod = geode::getMod()) {
            test::sendCount().fetch_add(1, std::memory_order_relaxed);
            m_method = std::move(method);
            m_url = std::move(url);
            m_mod = mod;
            return makeFuture();
        }

        WebFuture post(std::string url, Mod* mod = geode::getMod()) {
            return send("POST", std::move(url), mod);
        }

        WebFuture get(std::string url, Mod* mod = geode::getMod()) {
            return send("GET", std::move(url), mod);
        }

        WebFuture put(std::string url, Mod* mod = geode::getMod()) {
            return send("PUT", std::move(url), mod);
        }

        WebFuture patch(std::string url, Mod* mod = geode::getMod()) {
            return send("PATCH", std::move(url), mod);
        }

        WebRequest& header(std::string name, std::string value) {
            m_headers[std::move(name)].push_back(std::move(value));
            return *this;
        }

        WebRequest& removeHeader(std::string_view name) {
            m_headers.erase(std::string(name));
            return *this;
        }

        WebRequest& param(std::string name, std::string value) {
            m_params[std::move(name)] = std::move(value);
            return *this;
        }

        WebRequest& removeParam(std::string_view name) {
            m_params.erase(std::string(name));
            return *this;
        }

        WebRequest& method(std::string value) {
            m_method = std::move(value);
            return *this;
        }

        WebRequest& url(std::string value) {
            m_url = std::move(value);
            return *this;
        }

        WebRequest& userAgent(std::string value) {
            m_userAgent = std::move(value);
            return *this;
        }

        WebRequest& acceptEncoding(std::string value) {
            m_acceptEncoding = std::move(value);
            return *this;
        }

        WebRequest& timeout(std::chrono::seconds time) {
            m_timeout = time;
            return *this;
        }

        WebRequest& downloadRange(std::pair<std::uint64_t, std::uint64_t> byteRange) {
            m_downloadRange = byteRange;
            return *this;
        }

        WebRequest& certVerification(bool enabled) {
            m_certVerification = enabled;
            return *this;
        }

        WebRequest& transferBody(bool enabled) {
            m_transferBody = enabled;
            return *this;
        }

        WebRequest& followRedirects(bool enabled) {
            m_followRedirects = enabled;
            return *this;
        }

        WebRequest& ignoreContentLength(bool enabled) {
            m_ignoreContentLength = enabled;
            return *this;
        }

        WebRequest& CABundleContent(std::string content) {
            m_caBundle = std::move(content);
            return *this;
        }

        WebRequest& proxyOpts(ProxyOpts proxyOpts) {
            m_proxy = std::move(proxyOpts);
            return *this;
        }

        WebRequest& version(HttpVersion httpVersion) {
            m_httpVersion = httpVersion;
            return *this;
        }

        WebRequest& body(ByteVector raw) {
            m_body = std::move(raw);
            m_bodyString.reset();
            m_bodyJson.reset();
            m_bodyMultipart.reset();
            return *this;
        }

        WebRequest& bodyString(std::string_view str) {
            m_bodyString = std::string(str);
            m_body.reset();
            m_bodyJson.reset();
            m_bodyMultipart.reset();
            return *this;
        }

        WebRequest& bodyJSON(matjson::Value const& json) {
            m_bodyJson = json;
            m_body.reset();
            m_bodyString.reset();
            m_bodyMultipart.reset();
            return *this;
        }

        WebRequest& bodyMultipart(MultipartForm const& form) {
            m_bodyMultipart = form.getBody();
            m_body.reset();
            m_bodyString.reset();
            m_bodyJson.reset();
            return *this;
        }

        WebRequest& onProgress(Function<void(WebProgress const&)> callback) {
            m_progress = std::move(callback);
            return *this;
        }

        std::size_t getID() const {
            return m_id;
        }

        Mod* getMod() const {
            return m_mod;
        }

        ZStringView getMethod() const {
            return m_method;
        }

        ZStringView getUrl() const {
            return m_url;
        }

        utils::StringMap<std::vector<std::string>> const& getHeaders() const {
            return m_headers;
        }

        utils::StringMap<std::string> const& getUrlParams() const {
            return m_params;
        }

        std::optional<ByteVector> getBody() const {
            if (m_body) return m_body;
            if (m_bodyString) return ByteVector(m_bodyString->begin(), m_bodyString->end());
            if (m_bodyJson) {
                auto text = m_bodyJson->dump(matjson::NO_INDENTATION);
                return ByteVector(text.begin(), text.end());
            }
            if (m_bodyMultipart) return m_bodyMultipart;
            return std::nullopt;
        }

        std::optional<std::chrono::seconds> getTimeout() const {
            return m_timeout;
        }

        HttpVersion getHttpVersion() const {
            return m_httpVersion;
        }

        WebProgress getProgress() const {
            WebProgress progress;
            if (auto body = getBody()) {
                progress.m_uploadTotal = body->size();
                progress.m_uploadCurrent = body->size();
            }
            return progress;
        }

    private:
        static std::size_t nextID() {
            static std::size_t next = 1;
            return next++;
        }

        WebFuture makeFuture() {
            auto snapshot = std::make_shared<WebRequest>(*this);
            return WebFuture([snapshot]() {
                if (snapshot->m_progress) snapshot->m_progress(snapshot->getProgress());
                if (auto const& factory = test::responseFactory()) {
                    return factory(*snapshot, snapshot->m_method, snapshot->m_url, snapshot->m_mod);
                }
                return test::makeDefaultResponse();
            });
        }

        std::size_t m_id = 0;
        std::string m_method;
        std::string m_url;
        std::string m_userAgent;
        std::string m_acceptEncoding;
        std::string m_caBundle;
        utils::StringMap<std::vector<std::string>> m_headers;
        utils::StringMap<std::string> m_params;
        std::optional<std::chrono::seconds> m_timeout;
        std::optional<std::pair<std::uint64_t, std::uint64_t>> m_downloadRange;
        bool m_certVerification = true;
        bool m_transferBody = true;
        bool m_followRedirects = true;
        bool m_ignoreContentLength = false;
        ProxyOpts m_proxy;
        HttpVersion m_httpVersion = HttpVersion::DEFAULT;
        std::optional<ByteVector> m_body;
        std::optional<std::string> m_bodyString;
        std::optional<matjson::Value> m_bodyJson;
        std::optional<ByteVector> m_bodyMultipart;
        Function<void(WebProgress const&)> m_progress;
        Mod* m_mod = nullptr;
    };

    inline void openLinkInBrowser(ZStringView /*url*/) {}

    namespace detail {
        struct WebRequestInterceptEventTag {};

        struct IDBasedWebRequestInterceptEventTag {};

        struct WebResponseEventTag {};

        struct IDBasedWebResponseEventTag {};

        template <class EventTag, class Cb>
        geode::ListenerHandle listenEvent(Cb&& cb, int /*priority*/) {
            auto handler = std::make_shared<std::decay_t<Cb>>(std::forward<Cb>(cb));
            return geode::ListenerHandle([handler]() mutable {
                handler.reset();
            });
        }

        template <class Tag>
        struct WebEvent {
            WebEvent() = default;

            template <class T>
            explicit WebEvent(T&&) {}

            template <class Cb>
            geode::ListenerHandle listen(Cb&& cb, int priority = 0) {
                return listenEvent<Tag>(std::forward<Cb>(cb), priority);
            }
        };
    } // namespace detail

    using WebRequestInterceptEvent = detail::WebEvent<detail::WebRequestInterceptEventTag>;
    using IDBasedWebRequestInterceptEvent =
        detail::WebEvent<detail::IDBasedWebRequestInterceptEventTag>;
    using WebResponseEvent = detail::WebEvent<detail::WebResponseEventTag>;
    using IDBasedWebResponseEvent = detail::WebEvent<detail::IDBasedWebResponseEventTag>;
} // namespace geode::utils::web
