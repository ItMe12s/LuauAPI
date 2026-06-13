#pragma once

#include <Geode/Result.hpp>
#include <Geode/utils/async.hpp>

#include <cstdint>
#include <vector>

namespace geode {
    class ListenerHandle {
    public:
        ListenerHandle() = default;
    };
}

namespace geode::utils::web {
    using ByteVector = std::vector<std::uint8_t>;

    class WebRequest {
    public:
        WebRequest() = default;
        WebRequest(WebRequest&&) = default;
        WebRequest& operator=(WebRequest&&) = default;
        ~WebRequest() = default;
    };

    class WebResponse {
    public:
        WebResponse() = default;
        WebResponse(WebResponse&&) = default;
        WebResponse& operator=(WebResponse&&) = default;

        ByteVector const& data() const& { return m_data; }
        ByteVector data() && { return std::move(m_data); }

    private:
        ByteVector m_data;
    };

    class MultipartForm {
    public:
        MultipartForm() = default;
        MultipartForm(MultipartForm&&) = default;
        MultipartForm& operator=(MultipartForm&&) = default;
        ~MultipartForm() = default;
    };
}
