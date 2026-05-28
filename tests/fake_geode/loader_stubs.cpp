#include <Geode/loader/SettingV3.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/SpacerNode.hpp>
#include <Geode/ui/TextRenderer.hpp>
#include <Geode/utils/JsonValidation.hpp>
#include <Geode/utils/string.hpp>
#include <Geode/utils/web.hpp>

#include <cocos2d.h>
#include <matjson.hpp>

using namespace geode::prelude;

namespace geode {
    void SettingV3::parseBaseProperties(std::string, std::string, JsonExpectedValue&) {}

    void SettingV3::markChanged() {}

    bool TitleSettingV3::load(matjson::Value const&) { return false; }
    bool TitleSettingV3::save(matjson::Value&) const { return false; }
    SettingNodeV3* TitleSettingV3::createNode(float) { return nullptr; }
    bool TitleSettingV3::isDefaultValue() const { return true; }
    void TitleSettingV3::reset() {}

    bool InfoSettingV3::load(matjson::Value const&) { return false; }
    bool InfoSettingV3::save(matjson::Value&) const { return false; }
    SettingNodeV3* InfoSettingV3::createNode(float) { return nullptr; }
    bool InfoSettingV3::isDefaultValue() const { return true; }
    void InfoSettingV3::reset() {}

    bool ButtonSettingV3::load(matjson::Value const&) { return false; }
    bool ButtonSettingV3::save(matjson::Value&) const { return false; }
    SettingNodeV3* ButtonSettingV3::createNode(float) { return nullptr; }
    bool ButtonSettingV3::isDefaultValue() const { return true; }
    void ButtonSettingV3::reset() {}

    Result<> BoolSettingV3::isValid(bool) const { return Ok(); }
    SettingNodeV3* BoolSettingV3::createNode(float) { return nullptr; }

    Result<> IntSettingV3::isValid(int64_t) const { return Ok(); }
    SettingNodeV3* IntSettingV3::createNode(float) { return nullptr; }

    Result<> FloatSettingV3::isValid(double) const { return Ok(); }
    SettingNodeV3* FloatSettingV3::createNode(float) { return nullptr; }

    Result<> StringSettingV3::isValid(std::string_view) const { return Ok(); }
    SettingNodeV3* StringSettingV3::createNode(float) { return nullptr; }

    Result<> FileSettingV3::isValid(std::filesystem::path const&) const { return Ok(); }
    SettingNodeV3* FileSettingV3::createNode(float) { return nullptr; }

    Result<> Color3BSettingV3::isValid(cocos2d::ccColor3B) const { return Ok(); }
    SettingNodeV3* Color3BSettingV3::createNode(float) { return nullptr; }

    Result<> Color4BSettingV3::isValid(cocos2d::ccColor4B) const { return Ok(); }
    SettingNodeV3* Color4BSettingV3::createNode(float) { return nullptr; }

    bool KeybindSettingV3::load(matjson::Value const&) { return false; }
    bool KeybindSettingV3::save(matjson::Value&) const { return false; }
    SettingNodeV3* KeybindSettingV3::createNode(float) { return nullptr; }
    bool KeybindSettingV3::isDefaultValue() const { return true; }
    void KeybindSettingV3::reset() {}

    void SpacerNodeChild::setContentSize(cocos2d::CCSize const& size) {
        cocos2d::CCNode::setContentSize(size);
    }

    void SettingNodeV3::setContentSize(cocos2d::CCSize const& size) {
        cocos2d::CCNode::setContentSize(size);
    }

    void SettingNodeV3::updateState(cocos2d::CCNode*) {}

    void Popup::registerWithTouchDispatcher() {}

    void Popup::onClose(cocos2d::CCObject*) {}

    void Popup::keyBackClicked() {}

    void Popup::keyDown(cocos2d::enumKeyCodes, double) {}

    Popup::~Popup() = default;

    TextRenderer::~TextRenderer() = default;

    namespace utils::string {
        std::string pathToString(std::filesystem::path const& path) {
            return path.string();
        }

        std::wstring utf8ToWide(std::string_view utf8) {
            return std::wstring(utf8.begin(), utf8.end());
        }
    }

    namespace utils::web {
        std::optional<WebResponse> WebFuture::poll(arc::Context&) {
            return std::nullopt;
        }
    }
}

namespace matjson {
    Result<cocos2d::ccColor3B, std::string> Serialize<cocos2d::ccColor3B>::fromJson(Value const&) {
        return Err("fake_geode: ccColor3B json unavailable in host tests");
    }

    Value Serialize<cocos2d::ccColor3B>::toJson(cocos2d::ccColor3B const& value) {
        return makeObject({
            { "r", value.r },
            { "g", value.g },
            { "b", value.b },
        });
    }

    Result<cocos2d::ccColor4B, std::string> Serialize<cocos2d::ccColor4B>::fromJson(Value const&) {
        return Err("fake_geode: ccColor4B json unavailable in host tests");
    }

    Value Serialize<cocos2d::ccColor4B>::toJson(cocos2d::ccColor4B const& value) {
        return makeObject({
            { "r", value.r },
            { "g", value.g },
            { "b", value.b },
            { "a", value.a },
        });
    }
}
