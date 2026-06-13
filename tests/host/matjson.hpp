#pragma once

#include <Geode/Result.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace matjson {
    inline constexpr int NO_INDENTATION = -1;

    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object,
    };

    class Value;

    class Element {
    public:
        Element() = default;
        Element(Value const& value) : m_value(&value) {}
        Element(std::string key, Value const& value) : m_key(std::move(key)), m_value(&value) {}

        std::optional<std::string> getKey() const { return m_key; }
        operator Value const&() const { return *m_value; }

    private:
        std::optional<std::string> m_key;
        Value const* m_value = nullptr;
    };

    class Value {
    public:
        Value() = default;
        Value(std::nullptr_t) : m_data(nullptr) {}
        Value(bool value) : m_data(value) {}
        Value(double value) : m_data(value) {}
        Value(std::string value) : m_data(std::move(value)) {}
        Value(std::string_view value) : m_data(std::string(value)) {}
        Value(std::vector<Value> value) : m_data(std::move(value)) {}

        static Value object() { return Value(std::vector<std::pair<std::string, Value>>{}); }

        static geode::Result<Value> parse(std::string const& text) {
            return parse(std::string_view(text));
        }

        static geode::Result<Value> parse(std::string_view text) {
            if (text.empty()) {
                return geode::Err("empty json input");
            }
            if (text == "null") {
                return geode::Ok(Value(nullptr));
            }
            if (text == "true" || text == "false") {
                return geode::Ok(Value(text == "true"));
            }
            if (text == "{}" || text == "[]") {
                return geode::Ok(text == "{}" ? object() : Value(std::vector<Value>{}));
            }
            return geode::Err("unsupported json in host stub");
        }

        Type type() const {
            if (std::holds_alternative<std::nullptr_t>(m_data)) return Type::Null;
            if (std::holds_alternative<bool>(m_data)) return Type::Bool;
            if (std::holds_alternative<double>(m_data)) return Type::Number;
            if (std::holds_alternative<std::string>(m_data)) return Type::String;
            if (std::holds_alternative<std::vector<Value>>(m_data)) return Type::Array;
            return Type::Object;
        }

        geode::Result<bool> asBool() const {
            if (auto const* value = std::get_if<bool>(&m_data)) {
                return geode::Ok(*value);
            }
            return geode::Err("not a bool");
        }

        geode::Result<double> asDouble() const {
            if (auto const* value = std::get_if<double>(&m_data)) {
                return geode::Ok(*value);
            }
            return geode::Err("not a number");
        }

        geode::Result<std::string> asString() const {
            if (auto const* value = std::get_if<std::string>(&m_data)) {
                return geode::Ok(*value);
            }
            return geode::Err("not a string");
        }

        void set(std::string_view key, Value value) {
            auto& items = std::get<std::vector<std::pair<std::string, Value>>>(m_data);
            items.emplace_back(std::string(key), std::move(value));
        }

        std::string dump(int /*indent*/) const {
            switch (type()) {
                case Type::Null: return "null";
                case Type::Bool: return asBool().unwrapOr(false) ? "true" : "false";
                case Type::Number: return std::to_string(asDouble().unwrapOr(0.0));
                case Type::String: return "\"" + asString().unwrapOr(std::string()) + "\"";
                case Type::Array: return "[]";
                case Type::Object: {
                    auto const& items = std::get<std::vector<std::pair<std::string, Value>>>(m_data);
                    if (items.empty()) {
                        return "{}";
                    }
                    std::string out = "{";
                    for (std::size_t i = 0; i < items.size(); ++i) {
                        if (i != 0) {
                            out += ",";
                        }
                        out += "\"" + items[i].first + "\":" + items[i].second.dump(NO_INDENTATION);
                    }
                    out += "}";
                    return out;
                }
            }
            return "null";
        }

        class Iterator {
        public:
            Iterator(Value const* owner, std::size_t index) : m_owner(owner), m_index(index) {}

            Element operator*() const {
                if (auto const* array = std::get_if<std::vector<Value>>(&m_owner->m_data)) {
                    return Element((*array)[m_index]);
                }
                auto const& object = std::get<std::vector<std::pair<std::string, Value>>>(m_owner->m_data);
                return Element(object[m_index].first, object[m_index].second);
            }

            Iterator& operator++() {
                ++m_index;
                return *this;
            }

            bool operator!=(Iterator const& other) const {
                return m_index != other.m_index;
            }

        private:
            Value const* m_owner = nullptr;
            std::size_t m_index = 0;
        };

        Iterator begin() const {
            return Iterator(this, 0);
        }

        Iterator end() const {
            if (auto const* array = std::get_if<std::vector<Value>>(&m_data)) {
                return Iterator(this, array->size());
            }
            if (auto const* object = std::get_if<std::vector<std::pair<std::string, Value>>>(&m_data)) {
                return Iterator(this, object->size());
            }
            return Iterator(this, 0);
        }

    private:
        explicit Value(std::vector<std::pair<std::string, Value>> items) : m_data(std::move(items)) {}

        std::variant<
            std::nullptr_t,
            bool,
            double,
            std::string,
            std::vector<Value>,
            std::vector<std::pair<std::string, Value>>
        > m_data = nullptr;
    };
}
