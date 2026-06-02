#pragma once

#include "LuaDelegate.hpp"
#include "Stack.hpp"
#include "Types.hpp"
#include "Usertype.hpp"

namespace luax {
    class LuaAnimatedSpriteDelegate : public AnimatedSpriteDelegate, public cocos2d::CCObject {
    public:
        static LuaAnimatedSpriteDelegate* create(lua_State* L, int tableIndex);
        ~LuaAnimatedSpriteDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<AnimatedSpriteDelegate*>(this));
        }
            void animationFinished(char const* key) override {
                struct Ctx { char const* p0; };
        Ctx ctx{ key };
        LuaDelegateBase::invokeTableVoid(m_table, "animationFinished", "AnimatedSpriteDelegate.animationFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0 ? std::string(c->p0) : std::string());
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaBoomScrollLayerDelegate : public BoomScrollLayerDelegate, public cocos2d::CCObject {
    public:
        static LuaBoomScrollLayerDelegate* create(lua_State* L, int tableIndex);
        ~LuaBoomScrollLayerDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<BoomScrollLayerDelegate*>(this));
        }
            void scrollLayerScrollingStarted(BoomScrollLayer* layer) override {
                struct Ctx { BoomScrollLayer* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "scrollLayerScrollingStarted", "BoomScrollLayerDelegate.scrollLayerScrollingStarted", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<BoomScrollLayer>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void scrollLayerScrolledToPage(BoomScrollLayer* layer, int page) override {
                struct Ctx { BoomScrollLayer* p0; int p1; };
        Ctx ctx{ layer, page };
        LuaDelegateBase::invokeTableVoid(m_table, "scrollLayerScrolledToPage", "BoomScrollLayerDelegate.scrollLayerScrolledToPage", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<BoomScrollLayer>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
            void scrollLayerMoved(cocos2d::CCPoint position) override {
                struct Ctx { cocos2d::CCPoint p0; };
        Ctx ctx{ position };
        LuaDelegateBase::invokeTableVoid(m_table, "scrollLayerMoved", "BoomScrollLayerDelegate.scrollLayerMoved", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void scrollLayerWillScrollToPage(BoomScrollLayer* layer, int page) override {
                struct Ctx { BoomScrollLayer* p0; int p1; };
        Ctx ctx{ layer, page };
        LuaDelegateBase::invokeTableVoid(m_table, "scrollLayerWillScrollToPage", "BoomScrollLayerDelegate.scrollLayerWillScrollToPage", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<BoomScrollLayer>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCCCircleWaveDelegate : public CCCircleWaveDelegate, public cocos2d::CCObject {
    public:
        static LuaCCCircleWaveDelegate* create(lua_State* L, int tableIndex);
        ~LuaCCCircleWaveDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CCCircleWaveDelegate*>(this));
        }
            void circleWaveWillBeRemoved(CCCircleWave* circleWave) override {
                struct Ctx { CCCircleWave* p0; };
        Ctx ctx{ circleWave };
        LuaDelegateBase::invokeTableVoid(m_table, "circleWaveWillBeRemoved", "CCCircleWaveDelegate.circleWaveWillBeRemoved", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCCircleWave>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCCScrollLayerExtDelegate : public CCScrollLayerExtDelegate, public cocos2d::CCObject {
    public:
        static LuaCCScrollLayerExtDelegate* create(lua_State* L, int tableIndex);
        ~LuaCCScrollLayerExtDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CCScrollLayerExtDelegate*>(this));
        }
            void scrllViewWillBeginDecelerating(CCScrollLayerExt* layer) override {
                struct Ctx { CCScrollLayerExt* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "scrllViewWillBeginDecelerating", "CCScrollLayerExtDelegate.scrllViewWillBeginDecelerating", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCScrollLayerExt>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void scrollViewDidEndDecelerating(CCScrollLayerExt* layer) override {
                struct Ctx { CCScrollLayerExt* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "scrollViewDidEndDecelerating", "CCScrollLayerExtDelegate.scrollViewDidEndDecelerating", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCScrollLayerExt>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void scrollViewTouchMoving(CCScrollLayerExt* layer) override {
                struct Ctx { CCScrollLayerExt* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "scrollViewTouchMoving", "CCScrollLayerExtDelegate.scrollViewTouchMoving", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCScrollLayerExt>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void scrollViewDidEndMoving(CCScrollLayerExt* layer) override {
                struct Ctx { CCScrollLayerExt* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "scrollViewDidEndMoving", "CCScrollLayerExtDelegate.scrollViewDidEndMoving", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCScrollLayerExt>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void scrollViewTouchBegin(CCScrollLayerExt* layer) override {
                struct Ctx { CCScrollLayerExt* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "scrollViewTouchBegin", "CCScrollLayerExtDelegate.scrollViewTouchBegin", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCScrollLayerExt>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void scrollViewTouchEnd(CCScrollLayerExt* layer) override {
                struct Ctx { CCScrollLayerExt* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "scrollViewTouchEnd", "CCScrollLayerExtDelegate.scrollViewTouchEnd", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCScrollLayerExt>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCharacterColorDelegate : public CharacterColorDelegate, public cocos2d::CCObject {
    public:
        static LuaCharacterColorDelegate* create(lua_State* L, int tableIndex);
        ~LuaCharacterColorDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CharacterColorDelegate*>(this));
        }
        void playerColorChanged() override {
            LuaDelegateBase::invokeTableVoid(m_table, "playerColorChanged", "CharacterColorDelegate.playerColorChanged", 0);
        }
            void showUnlockPopup(int id, UnlockType type) override {
                struct Ctx { int p0; UnlockType p1; };
        Ctx ctx{ id, type };
        LuaDelegateBase::invokeTableVoid(m_table, "showUnlockPopup", "CharacterColorDelegate.showUnlockPopup", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p1)));
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaColorSelectDelegate : public ColorSelectDelegate, public cocos2d::CCObject {
    public:
        static LuaColorSelectDelegate* create(lua_State* L, int tableIndex);
        ~LuaColorSelectDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<ColorSelectDelegate*>(this));
        }
            void colorSelectClosed(cocos2d::CCNode* popup) override {
                struct Ctx { cocos2d::CCNode* p0; };
        Ctx ctx{ popup };
        LuaDelegateBase::invokeTableVoid(m_table, "colorSelectClosed", "ColorSelectDelegate.colorSelectClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCNode>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaColorSetupDelegate : public ColorSetupDelegate, public cocos2d::CCObject {
    public:
        static LuaColorSetupDelegate* create(lua_State* L, int tableIndex);
        ~LuaColorSetupDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<ColorSetupDelegate*>(this));
        }
            void colorSetupClosed(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "colorSetupClosed", "ColorSetupDelegate.colorSetupClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCommentUploadDelegate : public CommentUploadDelegate, public cocos2d::CCObject {
    public:
        static LuaCommentUploadDelegate* create(lua_State* L, int tableIndex);
        ~LuaCommentUploadDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CommentUploadDelegate*>(this));
        }
            void commentUploadFinished(int parentID) override {
                struct Ctx { int p0; };
        Ctx ctx{ parentID };
        LuaDelegateBase::invokeTableVoid(m_table, "commentUploadFinished", "CommentUploadDelegate.commentUploadFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void commentUploadFailed(int parentID, CommentError errorType) override {
                struct Ctx { int p0; CommentError p1; };
        Ctx ctx{ parentID, errorType };
        LuaDelegateBase::invokeTableVoid(m_table, "commentUploadFailed", "CommentUploadDelegate.commentUploadFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p1)));
        }, &ctx);
            }
            void commentDeleteFailed(int id, int parentID) override {
                struct Ctx { int p0; int p1; };
        Ctx ctx{ id, parentID };
        LuaDelegateBase::invokeTableVoid(m_table, "commentDeleteFailed", "CommentUploadDelegate.commentDeleteFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaConfigureValuePopupDelegate : public ConfigureValuePopupDelegate, public cocos2d::CCObject {
    public:
        static LuaConfigureValuePopupDelegate* create(lua_State* L, int tableIndex);
        ~LuaConfigureValuePopupDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<ConfigureValuePopupDelegate*>(this));
        }
            void valuePopupClosed(ConfigureValuePopup* popup, float value) override {
                struct Ctx { ConfigureValuePopup* p0; float p1; };
        Ctx ctx{ popup, value };
        LuaDelegateBase::invokeTableVoid(m_table, "valuePopupClosed", "ConfigureValuePopupDelegate.valuePopupClosed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<ConfigureValuePopup>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCurrencyRewardDelegate : public CurrencyRewardDelegate, public cocos2d::CCObject {
    public:
        static LuaCurrencyRewardDelegate* create(lua_State* L, int tableIndex);
        ~LuaCurrencyRewardDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CurrencyRewardDelegate*>(this));
        }
            void currencyWillExit(CurrencyRewardLayer* layer) override {
                struct Ctx { CurrencyRewardLayer* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "currencyWillExit", "CurrencyRewardDelegate.currencyWillExit", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CurrencyRewardLayer>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCustomSFXDelegate : public CustomSFXDelegate, public cocos2d::CCObject {
    public:
        static LuaCustomSFXDelegate* create(lua_State* L, int tableIndex);
        ~LuaCustomSFXDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CustomSFXDelegate*>(this));
        }
            void sfxObjectSelected(SFXInfoObject* object) override {
                struct Ctx { SFXInfoObject* p0; };
        Ctx ctx{ object };
        LuaDelegateBase::invokeTableVoid(m_table, "sfxObjectSelected", "CustomSFXDelegate.sfxObjectSelected", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SFXInfoObject>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
        int getActiveSFXID() override {
            return LuaDelegateBase::invokeTableInt(m_table, "getActiveSFXID", 0, "CustomSFXDelegate.getActiveSFXID", 0);
        }
            bool overridePlaySFX(SFXInfoObject* object) override {
                struct Ctx { SFXInfoObject* p0; };
        Ctx ctx{ object };
        return LuaDelegateBase::invokeTableBool(m_table, "overridePlaySFX", false, "CustomSFXDelegate.overridePlaySFX", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SFXInfoObject>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCustomSongDelegate : public CustomSongDelegate, public cocos2d::CCObject {
    public:
        static LuaCustomSongDelegate* create(lua_State* L, int tableIndex);
        ~LuaCustomSongDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CustomSongDelegate*>(this));
        }
            void songIDChanged(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "songIDChanged", "CustomSongDelegate.songIDChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
        int getActiveSongID() override {
            return LuaDelegateBase::invokeTableInt(m_table, "getActiveSongID", 0, "CustomSongDelegate.getActiveSongID", 0);
        }
        gd::string getSongFileName() override {
            return gd::string(LuaDelegateBase::invokeTableString(m_table, "getSongFileName", std::string(), "CustomSongDelegate.getSongFileName", 0).c_str());
        }
        LevelSettingsObject* getLevelSettings() override {
            return LuaDelegateBase::invokeTableObject<LevelSettingsObject>(m_table, "getLevelSettings", nullptr, "CustomSongDelegate.getLevelSettings", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCustomSongLayerDelegate : public CustomSongLayerDelegate, public cocos2d::CCObject {
    public:
        static LuaCustomSongLayerDelegate* create(lua_State* L, int tableIndex);
        ~LuaCustomSongLayerDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CustomSongLayerDelegate*>(this));
        }
        void customSongLayerClosed() override {
            LuaDelegateBase::invokeTableVoid(m_table, "customSongLayerClosed", "CustomSongLayerDelegate.customSongLayerClosed", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaDemonFilterDelegate : public DemonFilterDelegate, public cocos2d::CCObject {
    public:
        static LuaDemonFilterDelegate* create(lua_State* L, int tableIndex);
        ~LuaDemonFilterDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<DemonFilterDelegate*>(this));
        }
            void demonFilterSelectClosed(int filter) override {
                struct Ctx { int p0; };
        Ctx ctx{ filter };
        LuaDelegateBase::invokeTableVoid(m_table, "demonFilterSelectClosed", "DemonFilterDelegate.demonFilterSelectClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaDialogDelegate : public DialogDelegate, public cocos2d::CCObject {
    public:
        static LuaDialogDelegate* create(lua_State* L, int tableIndex);
        ~LuaDialogDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<DialogDelegate*>(this));
        }
            void dialogClosed(DialogLayer* layer) override {
                struct Ctx { DialogLayer* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "dialogClosed", "DialogDelegate.dialogClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<DialogLayer>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaDownloadMessageDelegate : public DownloadMessageDelegate, public cocos2d::CCObject {
    public:
        static LuaDownloadMessageDelegate* create(lua_State* L, int tableIndex);
        ~LuaDownloadMessageDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<DownloadMessageDelegate*>(this));
        }
            void downloadMessageFinished(GJUserMessage* message) override {
                struct Ctx { GJUserMessage* p0; };
        Ctx ctx{ message };
        LuaDelegateBase::invokeTableVoid(m_table, "downloadMessageFinished", "DownloadMessageDelegate.downloadMessageFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJUserMessage>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void downloadMessageFailed(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "downloadMessageFailed", "DownloadMessageDelegate.downloadMessageFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaDynamicScrollDelegate : public DynamicScrollDelegate, public cocos2d::CCObject {
    public:
        static LuaDynamicScrollDelegate* create(lua_State* L, int tableIndex);
        ~LuaDynamicScrollDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<DynamicScrollDelegate*>(this));
        }
            void updatePageWithObject(cocos2d::CCObject* layer, cocos2d::CCObject* object) override {
                struct Ctx { cocos2d::CCObject* p0; cocos2d::CCObject* p1; };
        Ctx ctx{ layer, object };
        LuaDelegateBase::invokeTableVoid(m_table, "updatePageWithObject", "DynamicScrollDelegate.updatePageWithObject", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCObject>::pushBorrowed(L, c->p0);
                luax::Usertype<cocos2d::CCObject>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaFLAlertLayerProtocol : public FLAlertLayerProtocol, public cocos2d::CCObject {
    public:
        static LuaFLAlertLayerProtocol* create(lua_State* L, int tableIndex);
        ~LuaFLAlertLayerProtocol() override {
            LuaDelegateBase::unregisterInterface(static_cast<FLAlertLayerProtocol*>(this));
        }
            void FLAlert_Clicked(FLAlertLayer* layer, bool btn2) override {
                struct Ctx { FLAlertLayer* p0; bool p1; };
        Ctx ctx{ layer, btn2 };
        LuaDelegateBase::invokeTableVoid(m_table, "FLAlert_Clicked", "FLAlertLayerProtocol.FLAlert_Clicked", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<FLAlertLayer>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaFriendRequestDelegate : public FriendRequestDelegate, public cocos2d::CCObject {
    public:
        static LuaFriendRequestDelegate* create(lua_State* L, int tableIndex);
        ~LuaFriendRequestDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<FriendRequestDelegate*>(this));
        }
            void loadFRequestsFinished(cocos2d::CCArray* scores, char const* key) override {
                struct Ctx { cocos2d::CCArray* p0; char const* p1; };
        Ctx ctx{ scores, key };
        LuaDelegateBase::invokeTableVoid(m_table, "loadFRequestsFinished", "FriendRequestDelegate.loadFRequestsFinished", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCArray>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
            void setupPageInfo(gd::string info, char const* key) override {
                struct Ctx { gd::string p0; char const* p1; };
        Ctx ctx{ info, key };
        LuaDelegateBase::invokeTableVoid(m_table, "setupPageInfo", "FriendRequestDelegate.setupPageInfo", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, std::string(c->p0.c_str()));
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
            void forceReloadRequests(bool sent) override {
                struct Ctx { bool p0; };
        Ctx ctx{ sent };
        LuaDelegateBase::invokeTableVoid(m_table, "forceReloadRequests", "FriendRequestDelegate.forceReloadRequests", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJAccountBackupDelegate : public GJAccountBackupDelegate, public cocos2d::CCObject {
    public:
        static LuaGJAccountBackupDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJAccountBackupDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJAccountBackupDelegate*>(this));
        }
        void backupAccountFinished() override {
            LuaDelegateBase::invokeTableVoid(m_table, "backupAccountFinished", "GJAccountBackupDelegate.backupAccountFinished", 0);
        }
            void backupAccountFailed(BackupAccountError errorType, int response) override {
                struct Ctx { BackupAccountError p0; int p1; };
        Ctx ctx{ errorType, response };
        LuaDelegateBase::invokeTableVoid(m_table, "backupAccountFailed", "GJAccountBackupDelegate.backupAccountFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJAccountDelegate : public GJAccountDelegate, public cocos2d::CCObject {
    public:
        static LuaGJAccountDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJAccountDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJAccountDelegate*>(this));
        }
        void accountStatusChanged() override {
            LuaDelegateBase::invokeTableVoid(m_table, "accountStatusChanged", "GJAccountDelegate.accountStatusChanged", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJAccountLoginDelegate : public GJAccountLoginDelegate, public cocos2d::CCObject {
    public:
        static LuaGJAccountLoginDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJAccountLoginDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJAccountLoginDelegate*>(this));
        }
            void loginAccountFinished(int accountID, int userID) override {
                struct Ctx { int p0; int p1; };
        Ctx ctx{ accountID, userID };
        LuaDelegateBase::invokeTableVoid(m_table, "loginAccountFinished", "GJAccountLoginDelegate.loginAccountFinished", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
            void loginAccountFailed(AccountError errorType) override {
                struct Ctx { AccountError p0; };
        Ctx ctx{ errorType };
        LuaDelegateBase::invokeTableVoid(m_table, "loginAccountFailed", "GJAccountLoginDelegate.loginAccountFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJAccountRegisterDelegate : public GJAccountRegisterDelegate, public cocos2d::CCObject {
    public:
        static LuaGJAccountRegisterDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJAccountRegisterDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJAccountRegisterDelegate*>(this));
        }
        void registerAccountFinished() override {
            LuaDelegateBase::invokeTableVoid(m_table, "registerAccountFinished", "GJAccountRegisterDelegate.registerAccountFinished", 0);
        }
            void registerAccountFailed(AccountError errorType) override {
                struct Ctx { AccountError p0; };
        Ctx ctx{ errorType };
        LuaDelegateBase::invokeTableVoid(m_table, "registerAccountFailed", "GJAccountRegisterDelegate.registerAccountFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJAccountSettingsDelegate : public GJAccountSettingsDelegate, public cocos2d::CCObject {
    public:
        static LuaGJAccountSettingsDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJAccountSettingsDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJAccountSettingsDelegate*>(this));
        }
        void updateSettingsFinished() override {
            LuaDelegateBase::invokeTableVoid(m_table, "updateSettingsFinished", "GJAccountSettingsDelegate.updateSettingsFinished", 0);
        }
        void updateSettingsFailed() override {
            LuaDelegateBase::invokeTableVoid(m_table, "updateSettingsFailed", "GJAccountSettingsDelegate.updateSettingsFailed", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJAccountSyncDelegate : public GJAccountSyncDelegate, public cocos2d::CCObject {
    public:
        static LuaGJAccountSyncDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJAccountSyncDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJAccountSyncDelegate*>(this));
        }
        void syncAccountFinished() override {
            LuaDelegateBase::invokeTableVoid(m_table, "syncAccountFinished", "GJAccountSyncDelegate.syncAccountFinished", 0);
        }
            void syncAccountFailed(BackupAccountError errorType, int response) override {
                struct Ctx { BackupAccountError p0; int p1; };
        Ctx ctx{ errorType, response };
        LuaDelegateBase::invokeTableVoid(m_table, "syncAccountFailed", "GJAccountSyncDelegate.syncAccountFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJChallengeDelegate : public GJChallengeDelegate, public cocos2d::CCObject {
    public:
        static LuaGJChallengeDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJChallengeDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJChallengeDelegate*>(this));
        }
        void challengeStatusFinished() override {
            LuaDelegateBase::invokeTableVoid(m_table, "challengeStatusFinished", "GJChallengeDelegate.challengeStatusFinished", 0);
        }
        void challengeStatusFailed() override {
            LuaDelegateBase::invokeTableVoid(m_table, "challengeStatusFailed", "GJChallengeDelegate.challengeStatusFailed", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJDailyLevelDelegate : public GJDailyLevelDelegate, public cocos2d::CCObject {
    public:
        static LuaGJDailyLevelDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJDailyLevelDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJDailyLevelDelegate*>(this));
        }
            void dailyStatusFinished(GJTimedLevelType type) override {
                struct Ctx { GJTimedLevelType p0; };
        Ctx ctx{ type };
        LuaDelegateBase::invokeTableVoid(m_table, "dailyStatusFinished", "GJDailyLevelDelegate.dailyStatusFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJDropDownLayerDelegate : public GJDropDownLayerDelegate, public cocos2d::CCObject {
    public:
        static LuaGJDropDownLayerDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJDropDownLayerDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJDropDownLayerDelegate*>(this));
        }
            void dropDownLayerWillClose(GJDropDownLayer* layer) override {
                struct Ctx { GJDropDownLayer* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "dropDownLayerWillClose", "GJDropDownLayerDelegate.dropDownLayerWillClose", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJDropDownLayer>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJMPDelegate : public GJMPDelegate, public cocos2d::CCObject {
    public:
        static LuaGJMPDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJMPDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJMPDelegate*>(this));
        }
            void joinLobbyFinished(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "joinLobbyFinished", "GJMPDelegate.joinLobbyFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void didUploadMPComment(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "didUploadMPComment", "GJMPDelegate.didUploadMPComment", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
        void updateComments() override {
            LuaDelegateBase::invokeTableVoid(m_table, "updateComments", "GJMPDelegate.updateComments", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJOnlineRewardDelegate : public GJOnlineRewardDelegate, public cocos2d::CCObject {
    public:
        static LuaGJOnlineRewardDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJOnlineRewardDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJOnlineRewardDelegate*>(this));
        }
            void onlineRewardStatusFinished(gd::string key) override {
                struct Ctx { gd::string p0; };
        Ctx ctx{ key };
        LuaDelegateBase::invokeTableVoid(m_table, "onlineRewardStatusFinished", "GJOnlineRewardDelegate.onlineRewardStatusFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, std::string(c->p0.c_str()));
        }, &ctx);
            }
        void onlineRewardStatusFailed() override {
            LuaDelegateBase::invokeTableVoid(m_table, "onlineRewardStatusFailed", "GJOnlineRewardDelegate.onlineRewardStatusFailed", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJPurchaseDelegate : public GJPurchaseDelegate, public cocos2d::CCObject {
    public:
        static LuaGJPurchaseDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJPurchaseDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJPurchaseDelegate*>(this));
        }
            void didPurchaseItem(GJStoreItem* item) override {
                struct Ctx { GJStoreItem* p0; };
        Ctx ctx{ item };
        LuaDelegateBase::invokeTableVoid(m_table, "didPurchaseItem", "GJPurchaseDelegate.didPurchaseItem", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJStoreItem>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJRewardDelegate : public GJRewardDelegate, public cocos2d::CCObject {
    public:
        static LuaGJRewardDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJRewardDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJRewardDelegate*>(this));
        }
            void rewardsStatusFinished(int type) override {
                struct Ctx { int p0; };
        Ctx ctx{ type };
        LuaDelegateBase::invokeTableVoid(m_table, "rewardsStatusFinished", "GJRewardDelegate.rewardsStatusFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
        void rewardsStatusFailed() override {
            LuaDelegateBase::invokeTableVoid(m_table, "rewardsStatusFailed", "GJRewardDelegate.rewardsStatusFailed", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJRotationControlDelegate : public GJRotationControlDelegate, public cocos2d::CCObject {
    public:
        static LuaGJRotationControlDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJRotationControlDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJRotationControlDelegate*>(this));
        }
            void angleChanged(float angle) override {
                struct Ctx { float p0; };
        Ctx ctx{ angle };
        LuaDelegateBase::invokeTableVoid(m_table, "angleChanged", "GJRotationControlDelegate.angleChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
        void angleChangeBegin() override {
            LuaDelegateBase::invokeTableVoid(m_table, "angleChangeBegin", "GJRotationControlDelegate.angleChangeBegin", 0);
        }
        void angleChangeEnded() override {
            LuaDelegateBase::invokeTableVoid(m_table, "angleChangeEnded", "GJRotationControlDelegate.angleChangeEnded", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJScaleControlDelegate : public GJScaleControlDelegate, public cocos2d::CCObject {
    public:
        static LuaGJScaleControlDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJScaleControlDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJScaleControlDelegate*>(this));
        }
            void scaleXChanged(float scaleX, bool lock) override {
                struct Ctx { float p0; bool p1; };
        Ctx ctx{ scaleX, lock };
        LuaDelegateBase::invokeTableVoid(m_table, "scaleXChanged", "GJScaleControlDelegate.scaleXChanged", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
            void scaleYChanged(float scaleY, bool lock) override {
                struct Ctx { float p0; bool p1; };
        Ctx ctx{ scaleY, lock };
        LuaDelegateBase::invokeTableVoid(m_table, "scaleYChanged", "GJScaleControlDelegate.scaleYChanged", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
            void scaleXYChanged(float scaleX, float scaleY, bool lock) override {
                struct Ctx { float p0; float p1; bool p2; };
        Ctx ctx{ scaleX, scaleY, lock };
        LuaDelegateBase::invokeTableVoid(m_table, "scaleXYChanged", "GJScaleControlDelegate.scaleXYChanged", 3, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, c->p1);
                luax::push(L, c->p2);
        }, &ctx);
            }
        void scaleChangeBegin() override {
            LuaDelegateBase::invokeTableVoid(m_table, "scaleChangeBegin", "GJScaleControlDelegate.scaleChangeBegin", 0);
        }
        void scaleChangeEnded() override {
            LuaDelegateBase::invokeTableVoid(m_table, "scaleChangeEnded", "GJScaleControlDelegate.scaleChangeEnded", 0);
        }
        void updateScaleControl() override {
            LuaDelegateBase::invokeTableVoid(m_table, "updateScaleControl", "GJScaleControlDelegate.updateScaleControl", 0);
        }
            void anchorPointMoved(cocos2d::CCPoint newAnchor) override {
                struct Ctx { cocos2d::CCPoint p0; };
        Ctx ctx{ newAnchor };
        LuaDelegateBase::invokeTableVoid(m_table, "anchorPointMoved", "GJScaleControlDelegate.anchorPointMoved", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJSpecialColorSelectDelegate : public GJSpecialColorSelectDelegate, public cocos2d::CCObject {
    public:
        static LuaGJSpecialColorSelectDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJSpecialColorSelectDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJSpecialColorSelectDelegate*>(this));
        }
            void colorSelectClosed(GJSpecialColorSelect* select, int id) override {
                struct Ctx { GJSpecialColorSelect* p0; int p1; };
        Ctx ctx{ select, id };
        LuaDelegateBase::invokeTableVoid(m_table, "colorSelectClosed", "GJSpecialColorSelectDelegate.colorSelectClosed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJSpecialColorSelect>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGJTransformControlDelegate : public GJTransformControlDelegate, public cocos2d::CCObject {
    public:
        static LuaGJTransformControlDelegate* create(lua_State* L, int tableIndex);
        ~LuaGJTransformControlDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GJTransformControlDelegate*>(this));
        }
            void transformScaleXChanged(float scaleX) override {
                struct Ctx { float p0; };
        Ctx ctx{ scaleX };
        LuaDelegateBase::invokeTableVoid(m_table, "transformScaleXChanged", "GJTransformControlDelegate.transformScaleXChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void transformScaleYChanged(float scaleY) override {
                struct Ctx { float p0; };
        Ctx ctx{ scaleY };
        LuaDelegateBase::invokeTableVoid(m_table, "transformScaleYChanged", "GJTransformControlDelegate.transformScaleYChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void transformScaleXYChanged(float scaleX, float scaleY) override {
                struct Ctx { float p0; float p1; };
        Ctx ctx{ scaleX, scaleY };
        LuaDelegateBase::invokeTableVoid(m_table, "transformScaleXYChanged", "GJTransformControlDelegate.transformScaleXYChanged", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
            void transformRotationXChanged(float rotationX) override {
                struct Ctx { float p0; };
        Ctx ctx{ rotationX };
        LuaDelegateBase::invokeTableVoid(m_table, "transformRotationXChanged", "GJTransformControlDelegate.transformRotationXChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void transformRotationYChanged(float rotationY) override {
                struct Ctx { float p0; };
        Ctx ctx{ rotationY };
        LuaDelegateBase::invokeTableVoid(m_table, "transformRotationYChanged", "GJTransformControlDelegate.transformRotationYChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void transformRotationChanged(float rotation) override {
                struct Ctx { float p0; };
        Ctx ctx{ rotation };
        LuaDelegateBase::invokeTableVoid(m_table, "transformRotationChanged", "GJTransformControlDelegate.transformRotationChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
        void transformResetRotation() override {
            LuaDelegateBase::invokeTableVoid(m_table, "transformResetRotation", "GJTransformControlDelegate.transformResetRotation", 0);
        }
        void transformRestoreRotation() override {
            LuaDelegateBase::invokeTableVoid(m_table, "transformRestoreRotation", "GJTransformControlDelegate.transformRestoreRotation", 0);
        }
            void transformSkewXChanged(float skewX) override {
                struct Ctx { float p0; };
        Ctx ctx{ skewX };
        LuaDelegateBase::invokeTableVoid(m_table, "transformSkewXChanged", "GJTransformControlDelegate.transformSkewXChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void transformSkewYChanged(float skewY) override {
                struct Ctx { float p0; };
        Ctx ctx{ skewY };
        LuaDelegateBase::invokeTableVoid(m_table, "transformSkewYChanged", "GJTransformControlDelegate.transformSkewYChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
        void transformChangeBegin() override {
            LuaDelegateBase::invokeTableVoid(m_table, "transformChangeBegin", "GJTransformControlDelegate.transformChangeBegin", 0);
        }
        void transformChangeEnded() override {
            LuaDelegateBase::invokeTableVoid(m_table, "transformChangeEnded", "GJTransformControlDelegate.transformChangeEnded", 0);
        }
        void updateTransformControl() override {
            LuaDelegateBase::invokeTableVoid(m_table, "updateTransformControl", "GJTransformControlDelegate.updateTransformControl", 0);
        }
            void anchorPointMoved(cocos2d::CCPoint anchorPoint) override {
                struct Ctx { cocos2d::CCPoint p0; };
        Ctx ctx{ anchorPoint };
        LuaDelegateBase::invokeTableVoid(m_table, "anchorPointMoved", "GJTransformControlDelegate.anchorPointMoved", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
        cocos2d::CCNode* getTransformNode() override {
            return LuaDelegateBase::invokeTableObject<cocos2d::CCNode>(m_table, "getTransformNode", nullptr, "GJTransformControlDelegate.getTransformNode", 0);
        }
        EditorUI* getUI() override {
            return LuaDelegateBase::invokeTableObject<EditorUI>(m_table, "getUI", nullptr, "GJTransformControlDelegate.getUI", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGameRateDelegate : public GameRateDelegate, public cocos2d::CCObject {
    public:
        static LuaGameRateDelegate* create(lua_State* L, int tableIndex);
        ~LuaGameRateDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GameRateDelegate*>(this));
        }
        void updateRate() override {
            LuaDelegateBase::invokeTableVoid(m_table, "updateRate", "GameRateDelegate.updateRate", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaGooglePlayDelegate : public GooglePlayDelegate, public cocos2d::CCObject {
    public:
        static LuaGooglePlayDelegate* create(lua_State* L, int tableIndex);
        ~LuaGooglePlayDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<GooglePlayDelegate*>(this));
        }
        void googlePlaySignedIn() override {
            LuaDelegateBase::invokeTableVoid(m_table, "googlePlaySignedIn", "GooglePlayDelegate.googlePlaySignedIn", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaHSVWidgetDelegate : public HSVWidgetDelegate, public cocos2d::CCObject {
    public:
        static LuaHSVWidgetDelegate* create(lua_State* L, int tableIndex);
        ~LuaHSVWidgetDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<HSVWidgetDelegate*>(this));
        }
            void hsvPopupClosed(HSVWidgetPopup* popup, cocos2d::ccHSVValue value) override {
                struct Ctx { HSVWidgetPopup* p0; cocos2d::ccHSVValue p1; };
        Ctx ctx{ popup, value };
        LuaDelegateBase::invokeTableVoid(m_table, "hsvPopupClosed", "HSVWidgetDelegate.hsvPopupClosed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<HSVWidgetPopup>::pushBorrowed(L, c->p0);
                lua_pushnumber(L, static_cast<double>(c->p1.h));
        }, &ctx);
            }
            void hsvChanged(ConfigureHSVWidget* widget) override {
                struct Ctx { ConfigureHSVWidget* p0; };
        Ctx ctx{ widget };
        LuaDelegateBase::invokeTableVoid(m_table, "hsvChanged", "HSVWidgetDelegate.hsvChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<ConfigureHSVWidget>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLeaderboardManagerDelegate : public LeaderboardManagerDelegate, public cocos2d::CCObject {
    public:
        static LuaLeaderboardManagerDelegate* create(lua_State* L, int tableIndex);
        ~LuaLeaderboardManagerDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LeaderboardManagerDelegate*>(this));
        }
        void updateUserScoreFinished() override {
            LuaDelegateBase::invokeTableVoid(m_table, "updateUserScoreFinished", "LeaderboardManagerDelegate.updateUserScoreFinished", 0);
        }
        void updateUserScoreFailed() override {
            LuaDelegateBase::invokeTableVoid(m_table, "updateUserScoreFailed", "LeaderboardManagerDelegate.updateUserScoreFailed", 0);
        }
            void loadLeaderboardFinished(cocos2d::CCArray* scores, char const* key) override {
                struct Ctx { cocos2d::CCArray* p0; char const* p1; };
        Ctx ctx{ scores, key };
        LuaDelegateBase::invokeTableVoid(m_table, "loadLeaderboardFinished", "LeaderboardManagerDelegate.loadLeaderboardFinished", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCArray>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
            void loadLeaderboardFailed(char const* key) override {
                struct Ctx { char const* p0; };
        Ctx ctx{ key };
        LuaDelegateBase::invokeTableVoid(m_table, "loadLeaderboardFailed", "LeaderboardManagerDelegate.loadLeaderboardFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0 ? std::string(c->p0) : std::string());
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLevelCommentDelegate : public LevelCommentDelegate, public cocos2d::CCObject {
    public:
        static LuaLevelCommentDelegate* create(lua_State* L, int tableIndex);
        ~LuaLevelCommentDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LevelCommentDelegate*>(this));
        }
            void loadCommentsFinished(cocos2d::CCArray* comments, char const* key) override {
                struct Ctx { cocos2d::CCArray* p0; char const* p1; };
        Ctx ctx{ comments, key };
        LuaDelegateBase::invokeTableVoid(m_table, "loadCommentsFinished", "LevelCommentDelegate.loadCommentsFinished", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCArray>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
            void loadCommentsFailed(char const* key) override {
                struct Ctx { char const* p0; };
        Ctx ctx{ key };
        LuaDelegateBase::invokeTableVoid(m_table, "loadCommentsFailed", "LevelCommentDelegate.loadCommentsFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0 ? std::string(c->p0) : std::string());
        }, &ctx);
            }
        void updateUserScoreFinished() override {
            LuaDelegateBase::invokeTableVoid(m_table, "updateUserScoreFinished", "LevelCommentDelegate.updateUserScoreFinished", 0);
        }
            void setupPageInfo(gd::string info, char const* key) override {
                struct Ctx { gd::string p0; char const* p1; };
        Ctx ctx{ info, key };
        LuaDelegateBase::invokeTableVoid(m_table, "setupPageInfo", "LevelCommentDelegate.setupPageInfo", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, std::string(c->p0.c_str()));
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLevelDeleteDelegate : public LevelDeleteDelegate, public cocos2d::CCObject {
    public:
        static LuaLevelDeleteDelegate* create(lua_State* L, int tableIndex);
        ~LuaLevelDeleteDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LevelDeleteDelegate*>(this));
        }
            void levelDeleteFinished(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "levelDeleteFinished", "LevelDeleteDelegate.levelDeleteFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void levelDeleteFailed(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "levelDeleteFailed", "LevelDeleteDelegate.levelDeleteFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLevelDownloadDelegate : public LevelDownloadDelegate, public cocos2d::CCObject {
    public:
        static LuaLevelDownloadDelegate* create(lua_State* L, int tableIndex);
        ~LuaLevelDownloadDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LevelDownloadDelegate*>(this));
        }
            void levelDownloadFinished(GJGameLevel* level) override {
                struct Ctx { GJGameLevel* p0; };
        Ctx ctx{ level };
        LuaDelegateBase::invokeTableVoid(m_table, "levelDownloadFinished", "LevelDownloadDelegate.levelDownloadFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJGameLevel>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void levelDownloadFailed(int response) override {
                struct Ctx { int p0; };
        Ctx ctx{ response };
        LuaDelegateBase::invokeTableVoid(m_table, "levelDownloadFailed", "LevelDownloadDelegate.levelDownloadFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLevelListDeleteDelegate : public LevelListDeleteDelegate, public cocos2d::CCObject {
    public:
        static LuaLevelListDeleteDelegate* create(lua_State* L, int tableIndex);
        ~LuaLevelListDeleteDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LevelListDeleteDelegate*>(this));
        }
            void levelListDeleteFinished(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "levelListDeleteFinished", "LevelListDeleteDelegate.levelListDeleteFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void levelListDeleteFailed(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "levelListDeleteFailed", "LevelListDeleteDelegate.levelListDeleteFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLevelManagerDelegate : public LevelManagerDelegate, public cocos2d::CCObject {
    public:
        static LuaLevelManagerDelegate* create(lua_State* L, int tableIndex);
        ~LuaLevelManagerDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LevelManagerDelegate*>(this));
        }
            void loadLevelsFinished(cocos2d::CCArray* levels, char const* key) override {
                struct Ctx { cocos2d::CCArray* p0; char const* p1; };
        Ctx ctx{ levels, key };
        LuaDelegateBase::invokeTableVoid(m_table, "loadLevelsFinished", "LevelManagerDelegate.loadLevelsFinished", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCArray>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
            void loadLevelsFailed(char const* key) override {
                struct Ctx { char const* p0; };
        Ctx ctx{ key };
        LuaDelegateBase::invokeTableVoid(m_table, "loadLevelsFailed", "LevelManagerDelegate.loadLevelsFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0 ? std::string(c->p0) : std::string());
        }, &ctx);
            }
            void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int type) override {
                struct Ctx { cocos2d::CCArray* p0; char const* p1; int p2; };
        Ctx ctx{ levels, key, type };
        LuaDelegateBase::invokeTableVoid(m_table, "loadLevelsFinished", "LevelManagerDelegate.loadLevelsFinished", 3, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCArray>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
                luax::push(L, c->p2);
        }, &ctx);
            }
            void loadLevelsFailed(char const* key, int type) override {
                struct Ctx { char const* p0; int p1; };
        Ctx ctx{ key, type };
        LuaDelegateBase::invokeTableVoid(m_table, "loadLevelsFailed", "LevelManagerDelegate.loadLevelsFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0 ? std::string(c->p0) : std::string());
                luax::push(L, c->p1);
        }, &ctx);
            }
            void setupPageInfo(gd::string info, char const* key) override {
                struct Ctx { gd::string p0; char const* p1; };
        Ctx ctx{ info, key };
        LuaDelegateBase::invokeTableVoid(m_table, "setupPageInfo", "LevelManagerDelegate.setupPageInfo", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, std::string(c->p0.c_str()));
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLevelRateInfoDelegate : public LevelRateInfoDelegate, public cocos2d::CCObject {
    public:
        static LuaLevelRateInfoDelegate* create(lua_State* L, int tableIndex);
        ~LuaLevelRateInfoDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LevelRateInfoDelegate*>(this));
        }
            void rateInfoFinished(int id, gd::string info) override {
                struct Ctx { int p0; gd::string p1; };
        Ctx ctx{ id, info };
        LuaDelegateBase::invokeTableVoid(m_table, "rateInfoFinished", "LevelRateInfoDelegate.rateInfoFinished", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, std::string(c->p1.c_str()));
        }, &ctx);
            }
            void rateInfoFailed(int id, int response) override {
                struct Ctx { int p0; int p1; };
        Ctx ctx{ id, response };
        LuaDelegateBase::invokeTableVoid(m_table, "rateInfoFailed", "LevelRateInfoDelegate.rateInfoFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLevelSettingsDelegate : public LevelSettingsDelegate, public cocos2d::CCObject {
    public:
        static LuaLevelSettingsDelegate* create(lua_State* L, int tableIndex);
        ~LuaLevelSettingsDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LevelSettingsDelegate*>(this));
        }
        void levelSettingsUpdated() override {
            LuaDelegateBase::invokeTableVoid(m_table, "levelSettingsUpdated", "LevelSettingsDelegate.levelSettingsUpdated", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLevelUpdateDelegate : public LevelUpdateDelegate, public cocos2d::CCObject {
    public:
        static LuaLevelUpdateDelegate* create(lua_State* L, int tableIndex);
        ~LuaLevelUpdateDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LevelUpdateDelegate*>(this));
        }
            void levelUpdateFailed(int response) override {
                struct Ctx { int p0; };
        Ctx ctx{ response };
        LuaDelegateBase::invokeTableVoid(m_table, "levelUpdateFailed", "LevelUpdateDelegate.levelUpdateFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLevelUploadDelegate : public LevelUploadDelegate, public cocos2d::CCObject {
    public:
        static LuaLevelUploadDelegate* create(lua_State* L, int tableIndex);
        ~LuaLevelUploadDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LevelUploadDelegate*>(this));
        }
            void levelUploadFinished(GJGameLevel* level) override {
                struct Ctx { GJGameLevel* p0; };
        Ctx ctx{ level };
        LuaDelegateBase::invokeTableVoid(m_table, "levelUploadFinished", "LevelUploadDelegate.levelUploadFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJGameLevel>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void levelUploadFailed(GJGameLevel* level) override {
                struct Ctx { GJGameLevel* p0; };
        Ctx ctx{ level };
        LuaDelegateBase::invokeTableVoid(m_table, "levelUploadFailed", "LevelUploadDelegate.levelUploadFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJGameLevel>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaLikeItemDelegate : public LikeItemDelegate, public cocos2d::CCObject {
    public:
        static LuaLikeItemDelegate* create(lua_State* L, int tableIndex);
        ~LuaLikeItemDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<LikeItemDelegate*>(this));
        }
            void likedItem(LikeItemType type, int id, bool liked) override {
                struct Ctx { LikeItemType p0; int p1; bool p2; };
        Ctx ctx{ type, id, liked };
        LuaDelegateBase::invokeTableVoid(m_table, "likedItem", "LikeItemDelegate.likedItem", 3, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
                luax::push(L, c->p1);
                luax::push(L, c->p2);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaListButtonBarDelegate : public ListButtonBarDelegate, public cocos2d::CCObject {
    public:
        static LuaListButtonBarDelegate* create(lua_State* L, int tableIndex);
        ~LuaListButtonBarDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<ListButtonBarDelegate*>(this));
        }
            void listButtonBarSwitchedPage(ListButtonBar* bar, int page) override {
                struct Ctx { ListButtonBar* p0; int p1; };
        Ctx ctx{ bar, page };
        LuaDelegateBase::invokeTableVoid(m_table, "listButtonBarSwitchedPage", "ListButtonBarDelegate.listButtonBarSwitchedPage", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<ListButtonBar>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaListUploadDelegate : public ListUploadDelegate, public cocos2d::CCObject {
    public:
        static LuaListUploadDelegate* create(lua_State* L, int tableIndex);
        ~LuaListUploadDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<ListUploadDelegate*>(this));
        }
            void listUploadFinished(GJLevelList* list) override {
                struct Ctx { GJLevelList* p0; };
        Ctx ctx{ list };
        LuaDelegateBase::invokeTableVoid(m_table, "listUploadFinished", "ListUploadDelegate.listUploadFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJLevelList>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void listUploadFailed(GJLevelList* list, int response) override {
                struct Ctx { GJLevelList* p0; int p1; };
        Ctx ctx{ list, response };
        LuaDelegateBase::invokeTableVoid(m_table, "listUploadFailed", "ListUploadDelegate.listUploadFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJLevelList>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaMessageListDelegate : public MessageListDelegate, public cocos2d::CCObject {
    public:
        static LuaMessageListDelegate* create(lua_State* L, int tableIndex);
        ~LuaMessageListDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<MessageListDelegate*>(this));
        }
            void loadMessagesFinished(cocos2d::CCArray* messages, char const* key) override {
                struct Ctx { cocos2d::CCArray* p0; char const* p1; };
        Ctx ctx{ messages, key };
        LuaDelegateBase::invokeTableVoid(m_table, "loadMessagesFinished", "MessageListDelegate.loadMessagesFinished", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCArray>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
            void forceReloadMessages(bool sent) override {
                struct Ctx { bool p0; };
        Ctx ctx{ sent };
        LuaDelegateBase::invokeTableVoid(m_table, "forceReloadMessages", "MessageListDelegate.forceReloadMessages", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void setupPageInfo(gd::string info, char const* key) override {
                struct Ctx { gd::string p0; char const* p1; };
        Ctx ctx{ info, key };
        LuaDelegateBase::invokeTableVoid(m_table, "setupPageInfo", "MessageListDelegate.setupPageInfo", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, std::string(c->p0.c_str()));
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaMusicBrowserDelegate : public MusicBrowserDelegate, public cocos2d::CCObject {
    public:
        static LuaMusicBrowserDelegate* create(lua_State* L, int tableIndex);
        ~LuaMusicBrowserDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<MusicBrowserDelegate*>(this));
        }
            void musicBrowserClosed(MusicBrowser* browser) override {
                struct Ctx { MusicBrowser* p0; };
        Ctx ctx{ browser };
        LuaDelegateBase::invokeTableVoid(m_table, "musicBrowserClosed", "MusicBrowserDelegate.musicBrowserClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<MusicBrowser>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaMusicDownloadDelegate : public MusicDownloadDelegate, public cocos2d::CCObject {
    public:
        static LuaMusicDownloadDelegate* create(lua_State* L, int tableIndex);
        ~LuaMusicDownloadDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<MusicDownloadDelegate*>(this));
        }
            void loadSongInfoFinished(SongInfoObject* object) override {
                struct Ctx { SongInfoObject* p0; };
        Ctx ctx{ object };
        LuaDelegateBase::invokeTableVoid(m_table, "loadSongInfoFinished", "MusicDownloadDelegate.loadSongInfoFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SongInfoObject>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void loadSongInfoFailed(int id, GJSongError errorType) override {
                struct Ctx { int p0; GJSongError p1; };
        Ctx ctx{ id, errorType };
        LuaDelegateBase::invokeTableVoid(m_table, "loadSongInfoFailed", "MusicDownloadDelegate.loadSongInfoFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p1)));
        }, &ctx);
            }
            void downloadSongStarted(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "downloadSongStarted", "MusicDownloadDelegate.downloadSongStarted", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void downloadSongFinished(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "downloadSongFinished", "MusicDownloadDelegate.downloadSongFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void downloadSongFailed(int id, GJSongError errorType) override {
                struct Ctx { int p0; GJSongError p1; };
        Ctx ctx{ id, errorType };
        LuaDelegateBase::invokeTableVoid(m_table, "downloadSongFailed", "MusicDownloadDelegate.downloadSongFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p1)));
        }, &ctx);
            }
        void songStateChanged() override {
            LuaDelegateBase::invokeTableVoid(m_table, "songStateChanged", "MusicDownloadDelegate.songStateChanged", 0);
        }
            void downloadSFXFinished(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "downloadSFXFinished", "MusicDownloadDelegate.downloadSFXFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void downloadSFXFailed(int id, GJSongError errorType) override {
                struct Ctx { int p0; GJSongError p1; };
        Ctx ctx{ id, errorType };
        LuaDelegateBase::invokeTableVoid(m_table, "downloadSFXFailed", "MusicDownloadDelegate.downloadSFXFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p1)));
        }, &ctx);
            }
            void musicActionFinished(GJMusicAction action) override {
                struct Ctx { GJMusicAction p0; };
        Ctx ctx{ action };
        LuaDelegateBase::invokeTableVoid(m_table, "musicActionFinished", "MusicDownloadDelegate.musicActionFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
        }, &ctx);
            }
            void musicActionFailed(GJMusicAction action) override {
                struct Ctx { GJMusicAction p0; };
        Ctx ctx{ action };
        LuaDelegateBase::invokeTableVoid(m_table, "musicActionFailed", "MusicDownloadDelegate.musicActionFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaNumberInputDelegate : public NumberInputDelegate, public cocos2d::CCObject {
    public:
        static LuaNumberInputDelegate* create(lua_State* L, int tableIndex);
        ~LuaNumberInputDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<NumberInputDelegate*>(this));
        }
            void numberInputClosed(NumberInputLayer* layer) override {
                struct Ctx { NumberInputLayer* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "numberInputClosed", "NumberInputDelegate.numberInputClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<NumberInputLayer>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaOnlineListDelegate : public OnlineListDelegate, public cocos2d::CCObject {
    public:
        static LuaOnlineListDelegate* create(lua_State* L, int tableIndex);
        ~LuaOnlineListDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<OnlineListDelegate*>(this));
        }
            void loadListFinished(cocos2d::CCArray* objects, char const* key) override {
                struct Ctx { cocos2d::CCArray* p0; char const* p1; };
        Ctx ctx{ objects, key };
        LuaDelegateBase::invokeTableVoid(m_table, "loadListFinished", "OnlineListDelegate.loadListFinished", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCArray>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
            void loadListFailed(char const* key) override {
                struct Ctx { char const* p0; };
        Ctx ctx{ key };
        LuaDelegateBase::invokeTableVoid(m_table, "loadListFailed", "OnlineListDelegate.loadListFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0 ? std::string(c->p0) : std::string());
        }, &ctx);
            }
            void setupPageInfo(gd::string info, char const* key) override {
                struct Ctx { gd::string p0; char const* p1; };
        Ctx ctx{ info, key };
        LuaDelegateBase::invokeTableVoid(m_table, "setupPageInfo", "OnlineListDelegate.setupPageInfo", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, std::string(c->p0.c_str()));
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaOptionsObjectDelegate : public OptionsObjectDelegate, public cocos2d::CCObject {
    public:
        static LuaOptionsObjectDelegate* create(lua_State* L, int tableIndex);
        ~LuaOptionsObjectDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<OptionsObjectDelegate*>(this));
        }
            void stateChanged(OptionsObject* object) override {
                struct Ctx { OptionsObject* p0; };
        Ctx ctx{ object };
        LuaDelegateBase::invokeTableVoid(m_table, "stateChanged", "OptionsObjectDelegate.stateChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<OptionsObject>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaRateLevelDelegate : public RateLevelDelegate, public cocos2d::CCObject {
    public:
        static LuaRateLevelDelegate* create(lua_State* L, int tableIndex);
        ~LuaRateLevelDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<RateLevelDelegate*>(this));
        }
        void rateLevelClosed() override {
            LuaDelegateBase::invokeTableVoid(m_table, "rateLevelClosed", "RateLevelDelegate.rateLevelClosed", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaRewardedVideoDelegate : public RewardedVideoDelegate, public cocos2d::CCObject {
    public:
        static LuaRewardedVideoDelegate* create(lua_State* L, int tableIndex);
        ~LuaRewardedVideoDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<RewardedVideoDelegate*>(this));
        }
        void rewardedVideoFinished() override {
            LuaDelegateBase::invokeTableVoid(m_table, "rewardedVideoFinished", "RewardedVideoDelegate.rewardedVideoFinished", 0);
        }
        bool shouldOffsetRewardCurrency() override {
            return LuaDelegateBase::invokeTableBool(m_table, "shouldOffsetRewardCurrency", false, "RewardedVideoDelegate.shouldOffsetRewardCurrency", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSFXBrowserDelegate : public SFXBrowserDelegate, public cocos2d::CCObject {
    public:
        static LuaSFXBrowserDelegate* create(lua_State* L, int tableIndex);
        ~LuaSFXBrowserDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SFXBrowserDelegate*>(this));
        }
            void sfxBrowserClosed(SFXBrowser* browser) override {
                struct Ctx { SFXBrowser* p0; };
        Ctx ctx{ browser };
        LuaDelegateBase::invokeTableVoid(m_table, "sfxBrowserClosed", "SFXBrowserDelegate.sfxBrowserClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SFXBrowser>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSelectArtDelegate : public SelectArtDelegate, public cocos2d::CCObject {
    public:
        static LuaSelectArtDelegate* create(lua_State* L, int tableIndex);
        ~LuaSelectArtDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SelectArtDelegate*>(this));
        }
            void selectArtClosed(SelectArtLayer* layer) override {
                struct Ctx { SelectArtLayer* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "selectArtClosed", "SelectArtDelegate.selectArtClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SelectArtLayer>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSelectListIconDelegate : public SelectListIconDelegate, public cocos2d::CCObject {
    public:
        static LuaSelectListIconDelegate* create(lua_State* L, int tableIndex);
        ~LuaSelectListIconDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SelectListIconDelegate*>(this));
        }
            void iconSelectClosed(SelectListIconLayer* layer) override {
                struct Ctx { SelectListIconLayer* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "iconSelectClosed", "SelectListIconDelegate.iconSelectClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SelectListIconLayer>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSelectPremadeDelegate : public SelectPremadeDelegate, public cocos2d::CCObject {
    public:
        static LuaSelectPremadeDelegate* create(lua_State* L, int tableIndex);
        ~LuaSelectPremadeDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SelectPremadeDelegate*>(this));
        }
            void selectPremadeClosed(SelectPremadeLayer* layer, int type) override {
                struct Ctx { SelectPremadeLayer* p0; int p1; };
        Ctx ctx{ layer, type };
        LuaDelegateBase::invokeTableVoid(m_table, "selectPremadeClosed", "SelectPremadeDelegate.selectPremadeClosed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SelectPremadeLayer>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSelectSFXSortDelegate : public SelectSFXSortDelegate, public cocos2d::CCObject {
    public:
        static LuaSelectSFXSortDelegate* create(lua_State* L, int tableIndex);
        ~LuaSelectSFXSortDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SelectSFXSortDelegate*>(this));
        }
            void sortSelectClosed(SelectSFXSortLayer* layer) override {
                struct Ctx { SelectSFXSortLayer* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "sortSelectClosed", "SelectSFXSortDelegate.sortSelectClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SelectSFXSortLayer>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSelectSettingDelegate : public SelectSettingDelegate, public cocos2d::CCObject {
    public:
        static LuaSelectSettingDelegate* create(lua_State* L, int tableIndex);
        ~LuaSelectSettingDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SelectSettingDelegate*>(this));
        }
            void selectSettingClosed(SelectSettingLayer* layer) override {
                struct Ctx { SelectSettingLayer* p0; };
        Ctx ctx{ layer };
        LuaDelegateBase::invokeTableVoid(m_table, "selectSettingClosed", "SelectSettingDelegate.selectSettingClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SelectSettingLayer>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSetIDPopupDelegate : public SetIDPopupDelegate, public cocos2d::CCObject {
    public:
        static LuaSetIDPopupDelegate* create(lua_State* L, int tableIndex);
        ~LuaSetIDPopupDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SetIDPopupDelegate*>(this));
        }
            void setIDPopupClosed(SetIDPopup* popup, int value) override {
                struct Ctx { SetIDPopup* p0; int p1; };
        Ctx ctx{ popup, value };
        LuaDelegateBase::invokeTableVoid(m_table, "setIDPopupClosed", "SetIDPopupDelegate.setIDPopupClosed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SetIDPopup>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSetTextPopupDelegate : public SetTextPopupDelegate, public cocos2d::CCObject {
    public:
        static LuaSetTextPopupDelegate* create(lua_State* L, int tableIndex);
        ~LuaSetTextPopupDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SetTextPopupDelegate*>(this));
        }
            void setTextPopupClosed(SetTextPopup* popup, gd::string text) override {
                struct Ctx { SetTextPopup* p0; gd::string p1; };
        Ctx ctx{ popup, text };
        LuaDelegateBase::invokeTableVoid(m_table, "setTextPopupClosed", "SetTextPopupDelegate.setTextPopupClosed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SetTextPopup>::pushBorrowed(L, c->p0);
                luax::push(L, std::string(c->p1.c_str()));
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaShareCommentDelegate : public ShareCommentDelegate, public cocos2d::CCObject {
    public:
        static LuaShareCommentDelegate* create(lua_State* L, int tableIndex);
        ~LuaShareCommentDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<ShareCommentDelegate*>(this));
        }
            void shareCommentClosed(gd::string text, ShareCommentLayer* layer) override {
                struct Ctx { gd::string p0; ShareCommentLayer* p1; };
        Ctx ctx{ text, layer };
        LuaDelegateBase::invokeTableVoid(m_table, "shareCommentClosed", "ShareCommentDelegate.shareCommentClosed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, std::string(c->p0.c_str()));
                luax::Usertype<ShareCommentLayer>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSliderDelegate : public SliderDelegate, public cocos2d::CCObject {
    public:
        static LuaSliderDelegate* create(lua_State* L, int tableIndex);
        ~LuaSliderDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SliderDelegate*>(this));
        }
            void sliderBegan(Slider* slider) override {
                struct Ctx { Slider* p0; };
        Ctx ctx{ slider };
        LuaDelegateBase::invokeTableVoid(m_table, "sliderBegan", "SliderDelegate.sliderBegan", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<Slider>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void sliderEnded(Slider* slider) override {
                struct Ctx { Slider* p0; };
        Ctx ctx{ slider };
        LuaDelegateBase::invokeTableVoid(m_table, "sliderEnded", "SliderDelegate.sliderEnded", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<Slider>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSongPlaybackDelegate : public SongPlaybackDelegate, public cocos2d::CCObject {
    public:
        static LuaSongPlaybackDelegate* create(lua_State* L, int tableIndex);
        ~LuaSongPlaybackDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SongPlaybackDelegate*>(this));
        }
            void onPlayback(SongInfoObject* object) override {
                struct Ctx { SongInfoObject* p0; };
        Ctx ctx{ object };
        LuaDelegateBase::invokeTableVoid(m_table, "onPlayback", "SongPlaybackDelegate.onPlayback", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<SongInfoObject>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaSpritePartDelegate : public SpritePartDelegate, public cocos2d::CCObject {
    public:
        static LuaSpritePartDelegate* create(lua_State* L, int tableIndex);
        ~LuaSpritePartDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<SpritePartDelegate*>(this));
        }
            void displayFrameChanged(cocos2d::CCObject* sprite, gd::string frameName) override {
                struct Ctx { cocos2d::CCObject* p0; gd::string p1; };
        Ctx ctx{ sprite, frameName };
        LuaDelegateBase::invokeTableVoid(m_table, "displayFrameChanged", "SpritePartDelegate.displayFrameChanged", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCObject>::pushBorrowed(L, c->p0);
                luax::push(L, std::string(c->p1.c_str()));
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaTableViewCellDelegate : public TableViewCellDelegate, public cocos2d::CCObject {
    public:
        static LuaTableViewCellDelegate* create(lua_State* L, int tableIndex);
        ~LuaTableViewCellDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<TableViewCellDelegate*>(this));
        }
            bool cellPerformedAction(TableViewCell* cell, int listType, CellAction action, cocos2d::CCNode* parent) override {
                struct Ctx { TableViewCell* p0; int p1; CellAction p2; cocos2d::CCNode* p3; };
        Ctx ctx{ cell, listType, action, parent };
        return LuaDelegateBase::invokeTableBool(m_table, "cellPerformedAction", false, "TableViewCellDelegate.cellPerformedAction", 4, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<TableViewCell>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p2)));
                luax::Usertype<cocos2d::CCNode>::pushBorrowed(L, c->p3);
        }, &ctx);
            }
        int getSelectedCellIdx() override {
            return LuaDelegateBase::invokeTableInt(m_table, "getSelectedCellIdx", 0, "TableViewCellDelegate.getSelectedCellIdx", 0);
        }
        bool shouldSnapToSelected() override {
            return LuaDelegateBase::invokeTableBool(m_table, "shouldSnapToSelected", false, "TableViewCellDelegate.shouldSnapToSelected", 0);
        }
        int getCellDelegateType() override {
            return LuaDelegateBase::invokeTableInt(m_table, "getCellDelegateType", 0, "TableViewCellDelegate.getCellDelegateType", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaTableViewDelegate : public TableViewDelegate, public cocos2d::CCObject {
    public:
        static LuaTableViewDelegate* create(lua_State* L, int tableIndex);
        ~LuaTableViewDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<TableViewDelegate*>(this));
        }
            void willTweenToIndexPath(CCIndexPath& indexPath, TableViewCell* cell, TableView* tableView) override {
                struct Ctx { CCIndexPath p0; TableViewCell* p1; TableView* p2; };
        Ctx ctx{ indexPath, cell, tableView };
        LuaDelegateBase::invokeTableVoid(m_table, "willTweenToIndexPath", "TableViewDelegate.willTweenToIndexPath", 3, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_createtable(L, 0, 2);
                            lua_pushnumber(L, c->p0.m_section); lua_setfield(L, -2, "section");
                            lua_pushnumber(L, c->p0.m_row); lua_setfield(L, -2, "row");
                luax::Usertype<TableViewCell>::pushBorrowed(L, c->p1);
                luax::Usertype<TableView>::pushBorrowed(L, c->p2);
        }, &ctx);
            }
            void didEndTweenToIndexPath(CCIndexPath& indexPath, TableView* tableView) override {
                struct Ctx { CCIndexPath p0; TableView* p1; };
        Ctx ctx{ indexPath, tableView };
        LuaDelegateBase::invokeTableVoid(m_table, "didEndTweenToIndexPath", "TableViewDelegate.didEndTweenToIndexPath", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_createtable(L, 0, 2);
                            lua_pushnumber(L, c->p0.m_section); lua_setfield(L, -2, "section");
                            lua_pushnumber(L, c->p0.m_row); lua_setfield(L, -2, "row");
                luax::Usertype<TableView>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
            void TableViewWillDisplayCellForRowAtIndexPath(CCIndexPath& indexPath, TableViewCell* cell, TableView* tableView) override {
                struct Ctx { CCIndexPath p0; TableViewCell* p1; TableView* p2; };
        Ctx ctx{ indexPath, cell, tableView };
        LuaDelegateBase::invokeTableVoid(m_table, "TableViewWillDisplayCellForRowAtIndexPath", "TableViewDelegate.TableViewWillDisplayCellForRowAtIndexPath", 3, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_createtable(L, 0, 2);
                            lua_pushnumber(L, c->p0.m_section); lua_setfield(L, -2, "section");
                            lua_pushnumber(L, c->p0.m_row); lua_setfield(L, -2, "row");
                luax::Usertype<TableViewCell>::pushBorrowed(L, c->p1);
                luax::Usertype<TableView>::pushBorrowed(L, c->p2);
        }, &ctx);
            }
            void TableViewDidDisplayCellForRowAtIndexPath(CCIndexPath& indexPath, TableViewCell* cell, TableView* tableView) override {
                struct Ctx { CCIndexPath p0; TableViewCell* p1; TableView* p2; };
        Ctx ctx{ indexPath, cell, tableView };
        LuaDelegateBase::invokeTableVoid(m_table, "TableViewDidDisplayCellForRowAtIndexPath", "TableViewDelegate.TableViewDidDisplayCellForRowAtIndexPath", 3, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_createtable(L, 0, 2);
                            lua_pushnumber(L, c->p0.m_section); lua_setfield(L, -2, "section");
                            lua_pushnumber(L, c->p0.m_row); lua_setfield(L, -2, "row");
                luax::Usertype<TableViewCell>::pushBorrowed(L, c->p1);
                luax::Usertype<TableView>::pushBorrowed(L, c->p2);
        }, &ctx);
            }
            void TableViewWillReloadCellForRowAtIndexPath(CCIndexPath& indexPath, TableViewCell* cell, TableView* tableView) override {
                struct Ctx { CCIndexPath p0; TableViewCell* p1; TableView* p2; };
        Ctx ctx{ indexPath, cell, tableView };
        LuaDelegateBase::invokeTableVoid(m_table, "TableViewWillReloadCellForRowAtIndexPath", "TableViewDelegate.TableViewWillReloadCellForRowAtIndexPath", 3, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_createtable(L, 0, 2);
                            lua_pushnumber(L, c->p0.m_section); lua_setfield(L, -2, "section");
                            lua_pushnumber(L, c->p0.m_row); lua_setfield(L, -2, "row");
                luax::Usertype<TableViewCell>::pushBorrowed(L, c->p1);
                luax::Usertype<TableView>::pushBorrowed(L, c->p2);
        }, &ctx);
            }
            void didSelectRowAtIndexPath(CCIndexPath& indexPath, TableView* tableView) override {
                struct Ctx { CCIndexPath p0; TableView* p1; };
        Ctx ctx{ indexPath, tableView };
        LuaDelegateBase::invokeTableVoid(m_table, "didSelectRowAtIndexPath", "TableViewDelegate.didSelectRowAtIndexPath", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_createtable(L, 0, 2);
                            lua_pushnumber(L, c->p0.m_section); lua_setfield(L, -2, "section");
                            lua_pushnumber(L, c->p0.m_row); lua_setfield(L, -2, "row");
                luax::Usertype<TableView>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaTextAreaDelegate : public TextAreaDelegate, public cocos2d::CCObject {
    public:
        static LuaTextAreaDelegate* create(lua_State* L, int tableIndex);
        ~LuaTextAreaDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<TextAreaDelegate*>(this));
        }
            void fadeInTextFinished(TextArea* textArea) override {
                struct Ctx { TextArea* p0; };
        Ctx ctx{ textArea };
        LuaDelegateBase::invokeTableVoid(m_table, "fadeInTextFinished", "TextAreaDelegate.fadeInTextFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<TextArea>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaTextInputDelegate : public TextInputDelegate, public cocos2d::CCObject {
    public:
        static LuaTextInputDelegate* create(lua_State* L, int tableIndex);
        ~LuaTextInputDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<TextInputDelegate*>(this));
        }
            void textChanged(CCTextInputNode* node) override {
                struct Ctx { CCTextInputNode* p0; };
        Ctx ctx{ node };
        LuaDelegateBase::invokeTableVoid(m_table, "textChanged", "TextInputDelegate.textChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCTextInputNode>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void textInputOpened(CCTextInputNode* node) override {
                struct Ctx { CCTextInputNode* p0; };
        Ctx ctx{ node };
        LuaDelegateBase::invokeTableVoid(m_table, "textInputOpened", "TextInputDelegate.textInputOpened", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCTextInputNode>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void textInputClosed(CCTextInputNode* node) override {
                struct Ctx { CCTextInputNode* p0; };
        Ctx ctx{ node };
        LuaDelegateBase::invokeTableVoid(m_table, "textInputClosed", "TextInputDelegate.textInputClosed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCTextInputNode>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void textInputShouldOffset(CCTextInputNode* node, float yOffset) override {
                struct Ctx { CCTextInputNode* p0; float p1; };
        Ctx ctx{ node, yOffset };
        LuaDelegateBase::invokeTableVoid(m_table, "textInputShouldOffset", "TextInputDelegate.textInputShouldOffset", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCTextInputNode>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
            void textInputReturn(CCTextInputNode* node) override {
                struct Ctx { CCTextInputNode* p0; };
        Ctx ctx{ node };
        LuaDelegateBase::invokeTableVoid(m_table, "textInputReturn", "TextInputDelegate.textInputReturn", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCTextInputNode>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            bool allowTextInput(CCTextInputNode* node) override {
                struct Ctx { CCTextInputNode* p0; };
        Ctx ctx{ node };
        return LuaDelegateBase::invokeTableBool(m_table, "allowTextInput", false, "TextInputDelegate.allowTextInput", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCTextInputNode>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void enterPressed(CCTextInputNode* node) override {
                struct Ctx { CCTextInputNode* p0; };
        Ctx ctx{ node };
        LuaDelegateBase::invokeTableVoid(m_table, "enterPressed", "TextInputDelegate.enterPressed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<CCTextInputNode>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaUploadActionDelegate : public UploadActionDelegate, public cocos2d::CCObject {
    public:
        static LuaUploadActionDelegate* create(lua_State* L, int tableIndex);
        ~LuaUploadActionDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<UploadActionDelegate*>(this));
        }
            void uploadActionFinished(int id, int response) override {
                struct Ctx { int p0; int p1; };
        Ctx ctx{ id, response };
        LuaDelegateBase::invokeTableVoid(m_table, "uploadActionFinished", "UploadActionDelegate.uploadActionFinished", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
            void uploadActionFailed(int id, int response) override {
                struct Ctx { int p0; int p1; };
        Ctx ctx{ id, response };
        LuaDelegateBase::invokeTableVoid(m_table, "uploadActionFailed", "UploadActionDelegate.uploadActionFailed", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaUploadMessageDelegate : public UploadMessageDelegate, public cocos2d::CCObject {
    public:
        static LuaUploadMessageDelegate* create(lua_State* L, int tableIndex);
        ~LuaUploadMessageDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<UploadMessageDelegate*>(this));
        }
            void uploadMessageFinished(int accountID) override {
                struct Ctx { int p0; };
        Ctx ctx{ accountID };
        LuaDelegateBase::invokeTableVoid(m_table, "uploadMessageFinished", "UploadMessageDelegate.uploadMessageFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void uploadMessageFailed(int accountID) override {
                struct Ctx { int p0; };
        Ctx ctx{ accountID };
        LuaDelegateBase::invokeTableVoid(m_table, "uploadMessageFailed", "UploadMessageDelegate.uploadMessageFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaUploadPopupDelegate : public UploadPopupDelegate, public cocos2d::CCObject {
    public:
        static LuaUploadPopupDelegate* create(lua_State* L, int tableIndex);
        ~LuaUploadPopupDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<UploadPopupDelegate*>(this));
        }
            void onClosePopup(UploadActionPopup* popup) override {
                struct Ctx { UploadActionPopup* p0; };
        Ctx ctx{ popup };
        LuaDelegateBase::invokeTableVoid(m_table, "onClosePopup", "UploadPopupDelegate.onClosePopup", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<UploadActionPopup>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaUserInfoDelegate : public UserInfoDelegate, public cocos2d::CCObject {
    public:
        static LuaUserInfoDelegate* create(lua_State* L, int tableIndex);
        ~LuaUserInfoDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<UserInfoDelegate*>(this));
        }
            void getUserInfoFinished(GJUserScore* score) override {
                struct Ctx { GJUserScore* p0; };
        Ctx ctx{ score };
        LuaDelegateBase::invokeTableVoid(m_table, "getUserInfoFinished", "UserInfoDelegate.getUserInfoFinished", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJUserScore>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void getUserInfoFailed(int id) override {
                struct Ctx { int p0; };
        Ctx ctx{ id };
        LuaDelegateBase::invokeTableVoid(m_table, "getUserInfoFailed", "UserInfoDelegate.getUserInfoFailed", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
            void userInfoChanged(GJUserScore* score) override {
                struct Ctx { GJUserScore* p0; };
        Ctx ctx{ score };
        LuaDelegateBase::invokeTableVoid(m_table, "userInfoChanged", "UserInfoDelegate.userInfoChanged", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<GJUserScore>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaUserListDelegate : public UserListDelegate, public cocos2d::CCObject {
    public:
        static LuaUserListDelegate* create(lua_State* L, int tableIndex);
        ~LuaUserListDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<UserListDelegate*>(this));
        }
            void getUserListFinished(cocos2d::CCArray* scores, UserListType type) override {
                struct Ctx { cocos2d::CCArray* p0; UserListType p1; };
        Ctx ctx{ scores, type };
        LuaDelegateBase::invokeTableVoid(m_table, "getUserListFinished", "UserListDelegate.getUserListFinished", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCArray>::pushBorrowed(L, c->p0);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p1)));
        }, &ctx);
            }
            void userListChanged(cocos2d::CCArray* scores, UserListType type) override {
                struct Ctx { cocos2d::CCArray* p0; UserListType p1; };
        Ctx ctx{ scores, type };
        LuaDelegateBase::invokeTableVoid(m_table, "userListChanged", "UserListDelegate.userListChanged", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCArray>::pushBorrowed(L, c->p0);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p1)));
        }, &ctx);
            }
            void forceReloadList(UserListType type) override {
                struct Ctx { UserListType p0; };
        Ctx ctx{ type };
        LuaDelegateBase::invokeTableVoid(m_table, "forceReloadList", "UserListDelegate.forceReloadList", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCCDirectorDelegate : public cocos2d::CCDirectorDelegate, public cocos2d::CCObject {
    public:
        static LuaCCDirectorDelegate* create(lua_State* L, int tableIndex);
        ~LuaCCDirectorDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CCDirectorDelegate*>(this));
        }
        void updateProjection() override {
            LuaDelegateBase::invokeTableVoid(m_table, "updateProjection", "CCDirectorDelegate.updateProjection", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCCIMEDelegate : public cocos2d::CCIMEDelegate, public cocos2d::CCObject {
    public:
        static LuaCCIMEDelegate* create(lua_State* L, int tableIndex);
        ~LuaCCIMEDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CCIMEDelegate*>(this));
        }
        bool attachWithIME() override {
            return LuaDelegateBase::invokeTableBool(m_table, "attachWithIME", false, "CCIMEDelegate.attachWithIME", 0);
        }
        bool detachWithIME() override {
            return LuaDelegateBase::invokeTableBool(m_table, "detachWithIME", false, "CCIMEDelegate.detachWithIME", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCCKeyboardDelegate : public cocos2d::CCKeyboardDelegate, public cocos2d::CCObject {
    public:
        static LuaCCKeyboardDelegate* create(lua_State* L, int tableIndex);
        ~LuaCCKeyboardDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CCKeyboardDelegate*>(this));
        }
            void keyDown(cocos2d::enumKeyCodes key, double dt) override {
                struct Ctx { cocos2d::enumKeyCodes p0; double p1; };
        Ctx ctx{ key, dt };
        LuaDelegateBase::invokeTableVoid(m_table, "keyDown", "CCKeyboardDelegate.keyDown", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
                luax::push(L, c->p1);
        }, &ctx);
            }
            void keyUp(cocos2d::enumKeyCodes key, double dt) override {
                struct Ctx { cocos2d::enumKeyCodes p0; double p1; };
        Ctx ctx{ key, dt };
        LuaDelegateBase::invokeTableVoid(m_table, "keyUp", "CCKeyboardDelegate.keyUp", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p0)));
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCCKeypadDelegate : public cocos2d::CCKeypadDelegate, public cocos2d::CCObject {
    public:
        static LuaCCKeypadDelegate* create(lua_State* L, int tableIndex);
        ~LuaCCKeypadDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CCKeypadDelegate*>(this));
        }
        void keyBackClicked() override {
            LuaDelegateBase::invokeTableVoid(m_table, "keyBackClicked", "CCKeypadDelegate.keyBackClicked", 0);
        }
        void keyMenuClicked() override {
            LuaDelegateBase::invokeTableVoid(m_table, "keyMenuClicked", "CCKeypadDelegate.keyMenuClicked", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCCMouseDelegate : public cocos2d::CCMouseDelegate, public cocos2d::CCObject {
    public:
        static LuaCCMouseDelegate* create(lua_State* L, int tableIndex);
        ~LuaCCMouseDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CCMouseDelegate*>(this));
        }
        void rightKeyDown() override {
            LuaDelegateBase::invokeTableVoid(m_table, "rightKeyDown", "CCMouseDelegate.rightKeyDown", 0);
        }
        void rightKeyUp() override {
            LuaDelegateBase::invokeTableVoid(m_table, "rightKeyUp", "CCMouseDelegate.rightKeyUp", 0);
        }
            void scrollWheel(float y, float x) override {
                struct Ctx { float p0; float p1; };
        Ctx ctx{ y, x };
        LuaDelegateBase::invokeTableVoid(m_table, "scrollWheel", "CCMouseDelegate.scrollWheel", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
                luax::push(L, c->p1);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCCTextFieldDelegate : public cocos2d::CCTextFieldDelegate, public cocos2d::CCObject {
    public:
        static LuaCCTextFieldDelegate* create(lua_State* L, int tableIndex);
        ~LuaCCTextFieldDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CCTextFieldDelegate*>(this));
        }
            bool onTextFieldAttachWithIME(cocos2d::CCTextFieldTTF* sender) override {
                struct Ctx { cocos2d::CCTextFieldTTF* p0; };
        Ctx ctx{ sender };
        return LuaDelegateBase::invokeTableBool(m_table, "onTextFieldAttachWithIME", false, "CCTextFieldDelegate.onTextFieldAttachWithIME", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCTextFieldTTF>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            bool onTextFieldDetachWithIME(cocos2d::CCTextFieldTTF* sender) override {
                struct Ctx { cocos2d::CCTextFieldTTF* p0; };
        Ctx ctx{ sender };
        return LuaDelegateBase::invokeTableBool(m_table, "onTextFieldDetachWithIME", false, "CCTextFieldDelegate.onTextFieldDetachWithIME", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCTextFieldTTF>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            bool onTextFieldInsertText(cocos2d::CCTextFieldTTF* sender, char const* text, int len, cocos2d::enumKeyCodes keyCode) override {
                struct Ctx { cocos2d::CCTextFieldTTF* p0; char const* p1; int p2; cocos2d::enumKeyCodes p3; };
        Ctx ctx{ sender, text, len, keyCode };
        return LuaDelegateBase::invokeTableBool(m_table, "onTextFieldInsertText", false, "CCTextFieldDelegate.onTextFieldInsertText", 4, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCTextFieldTTF>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
                luax::push(L, c->p2);
                lua_pushnumber(L, static_cast<double>(static_cast<int>(c->p3)));
        }, &ctx);
            }
            bool onTextFieldDeleteBackward(cocos2d::CCTextFieldTTF* sender, char const* text, int len) override {
                struct Ctx { cocos2d::CCTextFieldTTF* p0; char const* p1; int p2; };
        Ctx ctx{ sender, text, len };
        return LuaDelegateBase::invokeTableBool(m_table, "onTextFieldDeleteBackward", false, "CCTextFieldDelegate.onTextFieldDeleteBackward", 3, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCTextFieldTTF>::pushBorrowed(L, c->p0);
                luax::push(L, c->p1 ? std::string(c->p1) : std::string());
                luax::push(L, c->p2);
        }, &ctx);
            }
            bool onDraw(cocos2d::CCTextFieldTTF* sender) override {
                struct Ctx { cocos2d::CCTextFieldTTF* p0; };
        Ctx ctx{ sender };
        return LuaDelegateBase::invokeTableBool(m_table, "onDraw", false, "CCTextFieldDelegate.onDraw", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCTextFieldTTF>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
        void textChanged() override {
            LuaDelegateBase::invokeTableVoid(m_table, "textChanged", "CCTextFieldDelegate.textChanged", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCCTouchDelegate : public cocos2d::CCTouchDelegate, public cocos2d::CCObject {
    public:
        static LuaCCTouchDelegate* create(lua_State* L, int tableIndex);
        ~LuaCCTouchDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CCTouchDelegate*>(this));
        }
            bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override {
                struct Ctx { cocos2d::CCTouch* p0; cocos2d::CCEvent* p1; };
        Ctx ctx{ touch, event };
        return LuaDelegateBase::invokeTableBool(m_table, "ccTouchBegan", false, "CCTouchDelegate.ccTouchBegan", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCTouch>::pushBorrowed(L, c->p0);
                luax::Usertype<cocos2d::CCEvent>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
            void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override {
                struct Ctx { cocos2d::CCTouch* p0; cocos2d::CCEvent* p1; };
        Ctx ctx{ touch, event };
        LuaDelegateBase::invokeTableVoid(m_table, "ccTouchMoved", "CCTouchDelegate.ccTouchMoved", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCTouch>::pushBorrowed(L, c->p0);
                luax::Usertype<cocos2d::CCEvent>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
            void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override {
                struct Ctx { cocos2d::CCTouch* p0; cocos2d::CCEvent* p1; };
        Ctx ctx{ touch, event };
        LuaDelegateBase::invokeTableVoid(m_table, "ccTouchEnded", "CCTouchDelegate.ccTouchEnded", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCTouch>::pushBorrowed(L, c->p0);
                luax::Usertype<cocos2d::CCEvent>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
            void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override {
                struct Ctx { cocos2d::CCTouch* p0; cocos2d::CCEvent* p1; };
        Ctx ctx{ touch, event };
        LuaDelegateBase::invokeTableVoid(m_table, "ccTouchCancelled", "CCTouchDelegate.ccTouchCancelled", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCTouch>::pushBorrowed(L, c->p0);
                luax::Usertype<cocos2d::CCEvent>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
            void ccTouchesBegan(cocos2d::CCSet* touches, cocos2d::CCEvent* event) override {
                struct Ctx { cocos2d::CCSet* p0; cocos2d::CCEvent* p1; };
        Ctx ctx{ touches, event };
        LuaDelegateBase::invokeTableVoid(m_table, "ccTouchesBegan", "CCTouchDelegate.ccTouchesBegan", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCSet>::pushBorrowed(L, c->p0);
                luax::Usertype<cocos2d::CCEvent>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
            void ccTouchesMoved(cocos2d::CCSet* touches, cocos2d::CCEvent* event) override {
                struct Ctx { cocos2d::CCSet* p0; cocos2d::CCEvent* p1; };
        Ctx ctx{ touches, event };
        LuaDelegateBase::invokeTableVoid(m_table, "ccTouchesMoved", "CCTouchDelegate.ccTouchesMoved", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCSet>::pushBorrowed(L, c->p0);
                luax::Usertype<cocos2d::CCEvent>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
            void ccTouchesEnded(cocos2d::CCSet* touches, cocos2d::CCEvent* event) override {
                struct Ctx { cocos2d::CCSet* p0; cocos2d::CCEvent* p1; };
        Ctx ctx{ touches, event };
        LuaDelegateBase::invokeTableVoid(m_table, "ccTouchesEnded", "CCTouchDelegate.ccTouchesEnded", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCSet>::pushBorrowed(L, c->p0);
                luax::Usertype<cocos2d::CCEvent>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
            void ccTouchesCancelled(cocos2d::CCSet* touches, cocos2d::CCEvent* event) override {
                struct Ctx { cocos2d::CCSet* p0; cocos2d::CCEvent* p1; };
        Ctx ctx{ touches, event };
        LuaDelegateBase::invokeTableVoid(m_table, "ccTouchesCancelled", "CCTouchDelegate.ccTouchesCancelled", 2, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::CCSet>::pushBorrowed(L, c->p0);
                luax::Usertype<cocos2d::CCEvent>::pushBorrowed(L, c->p1);
        }, &ctx);
            }
            void setPreviousPriority(int priority) override {
                struct Ctx { int p0; };
        Ctx ctx{ priority };
        LuaDelegateBase::invokeTableVoid(m_table, "setPreviousPriority", "CCTouchDelegate.setPreviousPriority", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::push(L, c->p0);
        }, &ctx);
            }
        int getPreviousPriority() override {
            return LuaDelegateBase::invokeTableInt(m_table, "getPreviousPriority", 0, "CCTouchDelegate.getPreviousPriority", 0);
        }
    private:
        std::shared_ptr<LuaRef> m_table;
    };

    class LuaCCEditBoxDelegate : public cocos2d::extension::CCEditBoxDelegate, public cocos2d::CCObject {
    public:
        static LuaCCEditBoxDelegate* create(lua_State* L, int tableIndex);
        ~LuaCCEditBoxDelegate() override {
            LuaDelegateBase::unregisterInterface(static_cast<CCEditBoxDelegate*>(this));
        }
            void editBoxEditingDidBegin(cocos2d::extension::CCEditBox* editBox) override {
                struct Ctx { cocos2d::extension::CCEditBox* p0; };
        Ctx ctx{ editBox };
        LuaDelegateBase::invokeTableVoid(m_table, "editBoxEditingDidBegin", "CCEditBoxDelegate.editBoxEditingDidBegin", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::extension::CCEditBox>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void editBoxEditingDidEnd(cocos2d::extension::CCEditBox* editBox) override {
                struct Ctx { cocos2d::extension::CCEditBox* p0; };
        Ctx ctx{ editBox };
        LuaDelegateBase::invokeTableVoid(m_table, "editBoxEditingDidEnd", "CCEditBoxDelegate.editBoxEditingDidEnd", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::extension::CCEditBox>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
            void editBoxReturn(cocos2d::extension::CCEditBox* editBox) override {
                struct Ctx { cocos2d::extension::CCEditBox* p0; };
        Ctx ctx{ editBox };
        LuaDelegateBase::invokeTableVoid(m_table, "editBoxReturn", "CCEditBoxDelegate.editBoxReturn", 1, +[](lua_State* L, void* raw) {
            auto* c = static_cast<Ctx*>(raw);
                luax::Usertype<cocos2d::extension::CCEditBox>::pushBorrowed(L, c->p0);
        }, &ctx);
            }
    private:
        std::shared_ptr<LuaRef> m_table;
    };
}
