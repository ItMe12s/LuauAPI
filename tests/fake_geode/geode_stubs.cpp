#include <Geode/loader/Hook.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Layout.hpp>

namespace geode {
    class Hook::Impl {};
    class Patch::Impl {};
    class AxisLayout::Impl {};
    class SettingV3::GeodeImpl {};
    class BasedButtonSprite::Impl {};

    Hook::~Hook() = default;
    Patch::~Patch() = default;

    std::shared_ptr<Hook> Hook::create(
        void*,
        void*,
        std::string,
        tulip::hook::HandlerMetadata,
        tulip::hook::HookMetadata
    ) {
        return nullptr;
    }

    std::shared_ptr<Patch> Patch::create(void*, ByteSpan) {
        return nullptr;
    }

    Result<Hook*> Mod::claimHook(std::shared_ptr<Hook>) {
        return Err("fake_geode: claimHook unavailable in host tests");
    }

    Result<Patch*> Mod::claimPatch(std::shared_ptr<Patch>) {
        return Err("fake_geode: claimPatch unavailable in host tests");
    }

    SettingV3::~SettingV3() = default;

    SettingV3::SettingV3() : m_impl(std::make_shared<GeodeImpl>()) {}

    AxisLayout::~AxisLayout() = default;
    void AxisLayout::apply(cocos2d::CCNode*) {}
    cocos2d::CCSize AxisLayout::getSizeHint(cocos2d::CCNode*) const { return {}; }

    void AnchorLayout::apply(cocos2d::CCNode*) {}
    cocos2d::CCSize AnchorLayout::getSizeHint(cocos2d::CCNode*) const { return {}; }

    CopySizeLayout::~CopySizeLayout() = default;
    void CopySizeLayout::apply(cocos2d::CCNode*) {}
    cocos2d::CCSize CopySizeLayout::getSizeHint(cocos2d::CCNode*) const { return {}; }

    BasedButtonSprite::BasedButtonSprite() = default;
    BasedButtonSprite::~BasedButtonSprite() = default;

    cocos2d::CCSize BasedButtonSprite::getMaxTopSize() const {
        return m_obContentSize - cocos2d::CCSize { 18.f, 18.f };
    }

    cocos2d::CCSize CircleButtonSprite::getMaxTopSize() const {
        return m_obContentSize * 0.65f;
    }

    cocos2d::CCSize EditorButtonSprite::getMaxTopSize() const {
        return m_obContentSize - cocos2d::CCSize { 8.f, 8.f };
    }
}
