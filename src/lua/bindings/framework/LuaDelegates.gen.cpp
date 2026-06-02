#include "LuaDelegates.gen.hpp"

namespace luax {
LuaAnimatedSpriteDelegate* LuaAnimatedSpriteDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaAnimatedSpriteDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<AnimatedSpriteDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaBoomScrollLayerDelegate* LuaBoomScrollLayerDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaBoomScrollLayerDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<BoomScrollLayerDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCCCircleWaveDelegate* LuaCCCircleWaveDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCCCircleWaveDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<CCCircleWaveDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCCScrollLayerExtDelegate* LuaCCScrollLayerExtDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCCScrollLayerExtDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<CCScrollLayerExtDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCharacterColorDelegate* LuaCharacterColorDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCharacterColorDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<CharacterColorDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaColorSelectDelegate* LuaColorSelectDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaColorSelectDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<ColorSelectDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaColorSetupDelegate* LuaColorSetupDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaColorSetupDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<ColorSetupDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCommentUploadDelegate* LuaCommentUploadDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCommentUploadDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<CommentUploadDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaConfigureValuePopupDelegate* LuaConfigureValuePopupDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaConfigureValuePopupDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<ConfigureValuePopupDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCurrencyRewardDelegate* LuaCurrencyRewardDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCurrencyRewardDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<CurrencyRewardDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCustomSFXDelegate* LuaCustomSFXDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCustomSFXDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<CustomSFXDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCustomSongDelegate* LuaCustomSongDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCustomSongDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<CustomSongDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCustomSongLayerDelegate* LuaCustomSongLayerDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCustomSongLayerDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<CustomSongLayerDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaDemonFilterDelegate* LuaDemonFilterDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaDemonFilterDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<DemonFilterDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaDialogDelegate* LuaDialogDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaDialogDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<DialogDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaDownloadMessageDelegate* LuaDownloadMessageDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaDownloadMessageDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<DownloadMessageDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaDynamicScrollDelegate* LuaDynamicScrollDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaDynamicScrollDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<DynamicScrollDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaFLAlertLayerProtocol* LuaFLAlertLayerProtocol::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaFLAlertLayerProtocol();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<FLAlertLayerProtocol*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaFriendRequestDelegate* LuaFriendRequestDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaFriendRequestDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<FriendRequestDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJAccountBackupDelegate* LuaGJAccountBackupDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJAccountBackupDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJAccountBackupDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJAccountDelegate* LuaGJAccountDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJAccountDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJAccountDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJAccountLoginDelegate* LuaGJAccountLoginDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJAccountLoginDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJAccountLoginDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJAccountRegisterDelegate* LuaGJAccountRegisterDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJAccountRegisterDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJAccountRegisterDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJAccountSettingsDelegate* LuaGJAccountSettingsDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJAccountSettingsDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJAccountSettingsDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJAccountSyncDelegate* LuaGJAccountSyncDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJAccountSyncDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJAccountSyncDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJChallengeDelegate* LuaGJChallengeDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJChallengeDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJChallengeDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJDailyLevelDelegate* LuaGJDailyLevelDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJDailyLevelDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJDailyLevelDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJDropDownLayerDelegate* LuaGJDropDownLayerDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJDropDownLayerDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJDropDownLayerDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJMPDelegate* LuaGJMPDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJMPDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJMPDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJOnlineRewardDelegate* LuaGJOnlineRewardDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJOnlineRewardDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJOnlineRewardDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJPurchaseDelegate* LuaGJPurchaseDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJPurchaseDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJPurchaseDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJRewardDelegate* LuaGJRewardDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJRewardDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJRewardDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJRotationControlDelegate* LuaGJRotationControlDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJRotationControlDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJRotationControlDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJScaleControlDelegate* LuaGJScaleControlDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJScaleControlDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJScaleControlDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJSpecialColorSelectDelegate* LuaGJSpecialColorSelectDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJSpecialColorSelectDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJSpecialColorSelectDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGJTransformControlDelegate* LuaGJTransformControlDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGJTransformControlDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GJTransformControlDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGameRateDelegate* LuaGameRateDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGameRateDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GameRateDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaGooglePlayDelegate* LuaGooglePlayDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaGooglePlayDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<GooglePlayDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaHSVWidgetDelegate* LuaHSVWidgetDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaHSVWidgetDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<HSVWidgetDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLeaderboardManagerDelegate* LuaLeaderboardManagerDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLeaderboardManagerDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LeaderboardManagerDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLevelCommentDelegate* LuaLevelCommentDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLevelCommentDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LevelCommentDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLevelDeleteDelegate* LuaLevelDeleteDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLevelDeleteDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LevelDeleteDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLevelDownloadDelegate* LuaLevelDownloadDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLevelDownloadDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LevelDownloadDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLevelListDeleteDelegate* LuaLevelListDeleteDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLevelListDeleteDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LevelListDeleteDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLevelManagerDelegate* LuaLevelManagerDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLevelManagerDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LevelManagerDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLevelRateInfoDelegate* LuaLevelRateInfoDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLevelRateInfoDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LevelRateInfoDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLevelSettingsDelegate* LuaLevelSettingsDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLevelSettingsDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LevelSettingsDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLevelUpdateDelegate* LuaLevelUpdateDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLevelUpdateDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LevelUpdateDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLevelUploadDelegate* LuaLevelUploadDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLevelUploadDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LevelUploadDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaLikeItemDelegate* LuaLikeItemDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaLikeItemDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<LikeItemDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaListButtonBarDelegate* LuaListButtonBarDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaListButtonBarDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<ListButtonBarDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaListUploadDelegate* LuaListUploadDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaListUploadDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<ListUploadDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaMessageListDelegate* LuaMessageListDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaMessageListDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<MessageListDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaMusicBrowserDelegate* LuaMusicBrowserDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaMusicBrowserDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<MusicBrowserDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaMusicDownloadDelegate* LuaMusicDownloadDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaMusicDownloadDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<MusicDownloadDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaNumberInputDelegate* LuaNumberInputDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaNumberInputDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<NumberInputDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaOnlineListDelegate* LuaOnlineListDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaOnlineListDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<OnlineListDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaOptionsObjectDelegate* LuaOptionsObjectDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaOptionsObjectDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<OptionsObjectDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaRateLevelDelegate* LuaRateLevelDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaRateLevelDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<RateLevelDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaRewardedVideoDelegate* LuaRewardedVideoDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaRewardedVideoDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<RewardedVideoDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSFXBrowserDelegate* LuaSFXBrowserDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSFXBrowserDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SFXBrowserDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSelectArtDelegate* LuaSelectArtDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSelectArtDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SelectArtDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSelectListIconDelegate* LuaSelectListIconDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSelectListIconDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SelectListIconDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSelectPremadeDelegate* LuaSelectPremadeDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSelectPremadeDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SelectPremadeDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSelectSFXSortDelegate* LuaSelectSFXSortDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSelectSFXSortDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SelectSFXSortDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSelectSettingDelegate* LuaSelectSettingDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSelectSettingDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SelectSettingDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSetIDPopupDelegate* LuaSetIDPopupDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSetIDPopupDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SetIDPopupDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSetTextPopupDelegate* LuaSetTextPopupDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSetTextPopupDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SetTextPopupDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaShareCommentDelegate* LuaShareCommentDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaShareCommentDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<ShareCommentDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSliderDelegate* LuaSliderDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSliderDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SliderDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSongPlaybackDelegate* LuaSongPlaybackDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSongPlaybackDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SongPlaybackDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaSpritePartDelegate* LuaSpritePartDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaSpritePartDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<SpritePartDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaTableViewCellDelegate* LuaTableViewCellDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaTableViewCellDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<TableViewCellDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaTableViewDelegate* LuaTableViewDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaTableViewDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<TableViewDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaTextAreaDelegate* LuaTextAreaDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaTextAreaDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<TextAreaDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaTextInputDelegate* LuaTextInputDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaTextInputDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<TextInputDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaUploadActionDelegate* LuaUploadActionDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaUploadActionDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<UploadActionDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaUploadMessageDelegate* LuaUploadMessageDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaUploadMessageDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<UploadMessageDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaUploadPopupDelegate* LuaUploadPopupDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaUploadPopupDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<UploadPopupDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaUserInfoDelegate* LuaUserInfoDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaUserInfoDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<UserInfoDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaUserListDelegate* LuaUserListDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaUserListDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<UserListDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCCDirectorDelegate* LuaCCDirectorDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCCDirectorDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<cocos2d::CCDirectorDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCCIMEDelegate* LuaCCIMEDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCCIMEDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<cocos2d::CCIMEDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCCKeyboardDelegate* LuaCCKeyboardDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCCKeyboardDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<cocos2d::CCKeyboardDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCCKeypadDelegate* LuaCCKeypadDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCCKeypadDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<cocos2d::CCKeypadDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCCMouseDelegate* LuaCCMouseDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCCMouseDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<cocos2d::CCMouseDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCCTextFieldDelegate* LuaCCTextFieldDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCCTextFieldDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<cocos2d::CCTextFieldDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCCTouchDelegate* LuaCCTouchDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCCTouchDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<cocos2d::CCTouchDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

LuaCCEditBoxDelegate* LuaCCEditBoxDelegate::create(lua_State* L, int tableIndex) {
    LuaDelegateBase::checkDelegateTable(L, tableIndex);
    auto* self = new LuaCCEditBoxDelegate();
    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);
    LuaDelegateBase::registerInterface(static_cast<cocos2d::extension::CCEditBoxDelegate*>(self), self->m_table);
    self->autorelease();
    return self;
}

}
