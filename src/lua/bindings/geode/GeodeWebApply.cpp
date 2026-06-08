#include "lua/bindings/geode/GeodeWebInternal.hpp"
#include "lua/bindings/geode/WebCaps.hpp"

#include <Geode/utils/web.hpp>
#include <chrono>
#include <cstdint>
#include <matjson.hpp>
#include <string>
#include <utility>

namespace luax::webdetail {
    using namespace luax;

    void applyHeader(web::WebRequest& req, std::string name, std::string value) {
        req.header(std::move(name), std::move(value));
    }

    void applyParam(web::WebRequest& req, std::string name, std::string value) {
        req.param(std::move(name), std::move(value));
    }

    void applyMethod(web::WebRequest& req, std::string value) {
        req.method(std::move(value));
    }

    void applyUrl(web::WebRequest& req, std::string value) {
        req.url(std::move(value));
    }

    void applyUserAgent(web::WebRequest& req, std::string value) {
        req.userAgent(std::move(value));
    }

    void applyAcceptEncoding(web::WebRequest& req, std::string value) {
        req.acceptEncoding(std::move(value));
    }

    void applyTimeout(web::WebRequest& req, double seconds) {
        req.timeout(std::chrono::seconds(static_cast<std::int64_t>(seconds)));
    }

    void applyDownloadRange(web::WebRequest& req, std::uint64_t start, std::uint64_t stop) {
        req.downloadRange({start, stop});
    }

    void applyCertVerification(web::WebRequest& req, bool value) {
        req.certVerification(value);
    }

    void applyTransferBody(web::WebRequest& req, bool value) {
        req.transferBody(value);
    }

    void applyFollowRedirects(web::WebRequest& req, bool value) {
        req.followRedirects(value);
    }

    void applyIgnoreContentLength(web::WebRequest& req, bool value) {
        req.ignoreContentLength(value);
    }

    void applyCaBundle(web::WebRequest& req, std::string value) {
        req.CABundleContent(std::move(value));
    }

    void applyProxy(web::WebRequest& req, web::ProxyOpts opts) {
        req.proxyOpts(std::move(opts));
    }

    void applyVersion(web::WebRequest& req, web::HttpVersion version) {
        req.version(version);
    }

    bool applyBody(web::WebRequest& req, std::string const& data) {
        if (!requestBodyWithinLimit(data.size())) return false;
        geode::ByteVector bytes(data.begin(), data.end());
        req.body(std::move(bytes));
        return true;
    }

    bool applyBodyString(web::WebRequest& req, std::string const& data) {
        if (!requestBodyWithinLimit(data.size())) return false;
        req.bodyString(data);
        return true;
    }

    bool applyBodyJson(web::WebRequest& req, matjson::Value value) {
        if (!requestJsonBodyWithinLimit(value)) return false;
        req.bodyJSON(std::move(value));
        return true;
    }

    bool applyBodyMultipart(web::WebRequest& req, web::MultipartForm const& form) {
        if (!requestBodyWithinLimit(form.getBody().size())) return false;
        req.bodyMultipart(form);
        return true;
    }
} // namespace luax::webdetail
