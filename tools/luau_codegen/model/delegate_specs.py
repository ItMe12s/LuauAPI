"""Delegate specs for LuauAPI codegen (generated)."""
from __future__ import annotations
from dataclasses import dataclass
from typing import Dict, FrozenSet, Optional, Tuple

@dataclass(frozen=True)
class DelegateMethodSpec:
    name: str
    ret_lua: str
    args_lua: Tuple[str, ...]

@dataclass(frozen=True)
class DelegateSpec:
    cxx_type: str
    lua_name: str
    cpp_class: str
    create_fn: str
    methods: Tuple[DelegateMethodSpec, ...]

DELEGATE_SPECS: Dict[str, DelegateSpec] = {
    "AnimatedSpriteDelegate": DelegateSpec(
        cxx_type="AnimatedSpriteDelegate", lua_name="AnimatedSpriteDelegate",
        cpp_class="LuaAnimatedSpriteDelegate", create_fn="LuaAnimatedSpriteDelegate::create",
        methods=(
            DelegateMethodSpec("animationFinished", "()", ('string',)),
        ),
    ),
    "BoomScrollLayerDelegate": DelegateSpec(
        cxx_type="BoomScrollLayerDelegate", lua_name="BoomScrollLayerDelegate",
        cpp_class="LuaBoomScrollLayerDelegate", create_fn="LuaBoomScrollLayerDelegate::create",
        methods=(
            DelegateMethodSpec("scrollLayerScrollingStarted", "()", ('BoomScrollLayer',)),
            DelegateMethodSpec("scrollLayerScrolledToPage", "()", ('BoomScrollLayer', 'number')),
            DelegateMethodSpec("scrollLayerMoved", "()", ('CCPoint',)),
            DelegateMethodSpec("scrollLayerWillScrollToPage", "()", ('BoomScrollLayer', 'number')),
        ),
    ),
    "CCCircleWaveDelegate": DelegateSpec(
        cxx_type="CCCircleWaveDelegate", lua_name="CCCircleWaveDelegate",
        cpp_class="LuaCCCircleWaveDelegate", create_fn="LuaCCCircleWaveDelegate::create",
        methods=(
            DelegateMethodSpec("circleWaveWillBeRemoved", "()", ('CCCircleWave',)),
        ),
    ),
    "CCScrollLayerExtDelegate": DelegateSpec(
        cxx_type="CCScrollLayerExtDelegate", lua_name="CCScrollLayerExtDelegate",
        cpp_class="LuaCCScrollLayerExtDelegate", create_fn="LuaCCScrollLayerExtDelegate::create",
        methods=(
            DelegateMethodSpec("scrllViewWillBeginDecelerating", "()", ('CCScrollLayerExt',)),
            DelegateMethodSpec("scrollViewDidEndDecelerating", "()", ('CCScrollLayerExt',)),
            DelegateMethodSpec("scrollViewTouchMoving", "()", ('CCScrollLayerExt',)),
            DelegateMethodSpec("scrollViewDidEndMoving", "()", ('CCScrollLayerExt',)),
            DelegateMethodSpec("scrollViewTouchBegin", "()", ('CCScrollLayerExt',)),
            DelegateMethodSpec("scrollViewTouchEnd", "()", ('CCScrollLayerExt',)),
        ),
    ),
    "CharacterColorDelegate": DelegateSpec(
        cxx_type="CharacterColorDelegate", lua_name="CharacterColorDelegate",
        cpp_class="LuaCharacterColorDelegate", create_fn="LuaCharacterColorDelegate::create",
        methods=(
            DelegateMethodSpec("playerColorChanged", "()", ()),
            DelegateMethodSpec("showUnlockPopup", "()", ('number', 'number')),
        ),
    ),
    "ColorSelectDelegate": DelegateSpec(
        cxx_type="ColorSelectDelegate", lua_name="ColorSelectDelegate",
        cpp_class="LuaColorSelectDelegate", create_fn="LuaColorSelectDelegate::create",
        methods=(
            DelegateMethodSpec("colorSelectClosed", "()", ('CCNode',)),
        ),
    ),
    "ColorSetupDelegate": DelegateSpec(
        cxx_type="ColorSetupDelegate", lua_name="ColorSetupDelegate",
        cpp_class="LuaColorSetupDelegate", create_fn="LuaColorSetupDelegate::create",
        methods=(
            DelegateMethodSpec("colorSetupClosed", "()", ('number',)),
        ),
    ),
    "CommentUploadDelegate": DelegateSpec(
        cxx_type="CommentUploadDelegate", lua_name="CommentUploadDelegate",
        cpp_class="LuaCommentUploadDelegate", create_fn="LuaCommentUploadDelegate::create",
        methods=(
            DelegateMethodSpec("commentUploadFinished", "()", ('number',)),
            DelegateMethodSpec("commentUploadFailed", "()", ('number', 'number')),
            DelegateMethodSpec("commentDeleteFailed", "()", ('number', 'number')),
        ),
    ),
    "ConfigureValuePopupDelegate": DelegateSpec(
        cxx_type="ConfigureValuePopupDelegate", lua_name="ConfigureValuePopupDelegate",
        cpp_class="LuaConfigureValuePopupDelegate", create_fn="LuaConfigureValuePopupDelegate::create",
        methods=(
            DelegateMethodSpec("valuePopupClosed", "()", ('ConfigureValuePopup', 'number')),
        ),
    ),
    "CurrencyRewardDelegate": DelegateSpec(
        cxx_type="CurrencyRewardDelegate", lua_name="CurrencyRewardDelegate",
        cpp_class="LuaCurrencyRewardDelegate", create_fn="LuaCurrencyRewardDelegate::create",
        methods=(
            DelegateMethodSpec("currencyWillExit", "()", ('CurrencyRewardLayer',)),
        ),
    ),
    "CustomSFXDelegate": DelegateSpec(
        cxx_type="CustomSFXDelegate", lua_name="CustomSFXDelegate",
        cpp_class="LuaCustomSFXDelegate", create_fn="LuaCustomSFXDelegate::create",
        methods=(
            DelegateMethodSpec("sfxObjectSelected", "()", ('SFXInfoObject',)),
            DelegateMethodSpec("getActiveSFXID", "number", ()),
            DelegateMethodSpec("overridePlaySFX", "boolean", ('SFXInfoObject',)),
        ),
    ),
    "CustomSongDelegate": DelegateSpec(
        cxx_type="CustomSongDelegate", lua_name="CustomSongDelegate",
        cpp_class="LuaCustomSongDelegate", create_fn="LuaCustomSongDelegate::create",
        methods=(
            DelegateMethodSpec("songIDChanged", "()", ('number',)),
            DelegateMethodSpec("getActiveSongID", "number", ()),
            DelegateMethodSpec("getSongFileName", "string", ()),
            DelegateMethodSpec("getLevelSettings", "LevelSettingsObject", ()),
        ),
    ),
    "CustomSongLayerDelegate": DelegateSpec(
        cxx_type="CustomSongLayerDelegate", lua_name="CustomSongLayerDelegate",
        cpp_class="LuaCustomSongLayerDelegate", create_fn="LuaCustomSongLayerDelegate::create",
        methods=(
            DelegateMethodSpec("customSongLayerClosed", "()", ()),
        ),
    ),
    "DemonFilterDelegate": DelegateSpec(
        cxx_type="DemonFilterDelegate", lua_name="DemonFilterDelegate",
        cpp_class="LuaDemonFilterDelegate", create_fn="LuaDemonFilterDelegate::create",
        methods=(
            DelegateMethodSpec("demonFilterSelectClosed", "()", ('number',)),
        ),
    ),
    "DialogDelegate": DelegateSpec(
        cxx_type="DialogDelegate", lua_name="DialogDelegate",
        cpp_class="LuaDialogDelegate", create_fn="LuaDialogDelegate::create",
        methods=(
            DelegateMethodSpec("dialogClosed", "()", ('DialogLayer',)),
        ),
    ),
    "DownloadMessageDelegate": DelegateSpec(
        cxx_type="DownloadMessageDelegate", lua_name="DownloadMessageDelegate",
        cpp_class="LuaDownloadMessageDelegate", create_fn="LuaDownloadMessageDelegate::create",
        methods=(
            DelegateMethodSpec("downloadMessageFinished", "()", ('GJUserMessage',)),
            DelegateMethodSpec("downloadMessageFailed", "()", ('number',)),
        ),
    ),
    "DynamicScrollDelegate": DelegateSpec(
        cxx_type="DynamicScrollDelegate", lua_name="DynamicScrollDelegate",
        cpp_class="LuaDynamicScrollDelegate", create_fn="LuaDynamicScrollDelegate::create",
        methods=(
            DelegateMethodSpec("updatePageWithObject", "()", ('CCObject', 'CCObject')),
        ),
    ),
    "FLAlertLayerProtocol": DelegateSpec(
        cxx_type="FLAlertLayerProtocol", lua_name="FLAlertLayerProtocol",
        cpp_class="LuaFLAlertLayerProtocol", create_fn="LuaFLAlertLayerProtocol::create",
        methods=(
            DelegateMethodSpec("FLAlert_Clicked", "()", ('FLAlertLayer', 'boolean')),
        ),
    ),
    "FriendRequestDelegate": DelegateSpec(
        cxx_type="FriendRequestDelegate", lua_name="FriendRequestDelegate",
        cpp_class="LuaFriendRequestDelegate", create_fn="LuaFriendRequestDelegate::create",
        methods=(
            DelegateMethodSpec("loadFRequestsFinished", "()", ('CCArray', 'string')),
            DelegateMethodSpec("setupPageInfo", "()", ('string', 'string')),
            DelegateMethodSpec("forceReloadRequests", "()", ('boolean',)),
        ),
    ),
    "GJAccountBackupDelegate": DelegateSpec(
        cxx_type="GJAccountBackupDelegate", lua_name="GJAccountBackupDelegate",
        cpp_class="LuaGJAccountBackupDelegate", create_fn="LuaGJAccountBackupDelegate::create",
        methods=(
            DelegateMethodSpec("backupAccountFinished", "()", ()),
            DelegateMethodSpec("backupAccountFailed", "()", ('number', 'number')),
        ),
    ),
    "GJAccountDelegate": DelegateSpec(
        cxx_type="GJAccountDelegate", lua_name="GJAccountDelegate",
        cpp_class="LuaGJAccountDelegate", create_fn="LuaGJAccountDelegate::create",
        methods=(
            DelegateMethodSpec("accountStatusChanged", "()", ()),
        ),
    ),
    "GJAccountLoginDelegate": DelegateSpec(
        cxx_type="GJAccountLoginDelegate", lua_name="GJAccountLoginDelegate",
        cpp_class="LuaGJAccountLoginDelegate", create_fn="LuaGJAccountLoginDelegate::create",
        methods=(
            DelegateMethodSpec("loginAccountFinished", "()", ('number', 'number')),
            DelegateMethodSpec("loginAccountFailed", "()", ('number',)),
        ),
    ),
    "GJAccountRegisterDelegate": DelegateSpec(
        cxx_type="GJAccountRegisterDelegate", lua_name="GJAccountRegisterDelegate",
        cpp_class="LuaGJAccountRegisterDelegate", create_fn="LuaGJAccountRegisterDelegate::create",
        methods=(
            DelegateMethodSpec("registerAccountFinished", "()", ()),
            DelegateMethodSpec("registerAccountFailed", "()", ('number',)),
        ),
    ),
    "GJAccountSettingsDelegate": DelegateSpec(
        cxx_type="GJAccountSettingsDelegate", lua_name="GJAccountSettingsDelegate",
        cpp_class="LuaGJAccountSettingsDelegate", create_fn="LuaGJAccountSettingsDelegate::create",
        methods=(
            DelegateMethodSpec("updateSettingsFinished", "()", ()),
            DelegateMethodSpec("updateSettingsFailed", "()", ()),
        ),
    ),
    "GJAccountSyncDelegate": DelegateSpec(
        cxx_type="GJAccountSyncDelegate", lua_name="GJAccountSyncDelegate",
        cpp_class="LuaGJAccountSyncDelegate", create_fn="LuaGJAccountSyncDelegate::create",
        methods=(
            DelegateMethodSpec("syncAccountFinished", "()", ()),
            DelegateMethodSpec("syncAccountFailed", "()", ('number', 'number')),
        ),
    ),
    "GJChallengeDelegate": DelegateSpec(
        cxx_type="GJChallengeDelegate", lua_name="GJChallengeDelegate",
        cpp_class="LuaGJChallengeDelegate", create_fn="LuaGJChallengeDelegate::create",
        methods=(
            DelegateMethodSpec("challengeStatusFinished", "()", ()),
            DelegateMethodSpec("challengeStatusFailed", "()", ()),
        ),
    ),
    "GJDailyLevelDelegate": DelegateSpec(
        cxx_type="GJDailyLevelDelegate", lua_name="GJDailyLevelDelegate",
        cpp_class="LuaGJDailyLevelDelegate", create_fn="LuaGJDailyLevelDelegate::create",
        methods=(
            DelegateMethodSpec("dailyStatusFinished", "()", ('number',)),
        ),
    ),
    "GJDropDownLayerDelegate": DelegateSpec(
        cxx_type="GJDropDownLayerDelegate", lua_name="GJDropDownLayerDelegate",
        cpp_class="LuaGJDropDownLayerDelegate", create_fn="LuaGJDropDownLayerDelegate::create",
        methods=(
            DelegateMethodSpec("dropDownLayerWillClose", "()", ('GJDropDownLayer',)),
        ),
    ),
    "GJMPDelegate": DelegateSpec(
        cxx_type="GJMPDelegate", lua_name="GJMPDelegate",
        cpp_class="LuaGJMPDelegate", create_fn="LuaGJMPDelegate::create",
        methods=(
            DelegateMethodSpec("joinLobbyFinished", "()", ('number',)),
            DelegateMethodSpec("didUploadMPComment", "()", ('number',)),
            DelegateMethodSpec("updateComments", "()", ()),
        ),
    ),
    "GJOnlineRewardDelegate": DelegateSpec(
        cxx_type="GJOnlineRewardDelegate", lua_name="GJOnlineRewardDelegate",
        cpp_class="LuaGJOnlineRewardDelegate", create_fn="LuaGJOnlineRewardDelegate::create",
        methods=(
            DelegateMethodSpec("onlineRewardStatusFinished", "()", ('string',)),
            DelegateMethodSpec("onlineRewardStatusFailed", "()", ()),
        ),
    ),
    "GJPurchaseDelegate": DelegateSpec(
        cxx_type="GJPurchaseDelegate", lua_name="GJPurchaseDelegate",
        cpp_class="LuaGJPurchaseDelegate", create_fn="LuaGJPurchaseDelegate::create",
        methods=(
            DelegateMethodSpec("didPurchaseItem", "()", ('GJStoreItem',)),
        ),
    ),
    "GJRewardDelegate": DelegateSpec(
        cxx_type="GJRewardDelegate", lua_name="GJRewardDelegate",
        cpp_class="LuaGJRewardDelegate", create_fn="LuaGJRewardDelegate::create",
        methods=(
            DelegateMethodSpec("rewardsStatusFinished", "()", ('number',)),
            DelegateMethodSpec("rewardsStatusFailed", "()", ()),
        ),
    ),
    "GJRotationControlDelegate": DelegateSpec(
        cxx_type="GJRotationControlDelegate", lua_name="GJRotationControlDelegate",
        cpp_class="LuaGJRotationControlDelegate", create_fn="LuaGJRotationControlDelegate::create",
        methods=(
            DelegateMethodSpec("angleChanged", "()", ('number',)),
            DelegateMethodSpec("angleChangeBegin", "()", ()),
            DelegateMethodSpec("angleChangeEnded", "()", ()),
        ),
    ),
    "GJScaleControlDelegate": DelegateSpec(
        cxx_type="GJScaleControlDelegate", lua_name="GJScaleControlDelegate",
        cpp_class="LuaGJScaleControlDelegate", create_fn="LuaGJScaleControlDelegate::create",
        methods=(
            DelegateMethodSpec("scaleXChanged", "()", ('number', 'boolean')),
            DelegateMethodSpec("scaleYChanged", "()", ('number', 'boolean')),
            DelegateMethodSpec("scaleXYChanged", "()", ('number', 'number', 'boolean')),
            DelegateMethodSpec("scaleChangeBegin", "()", ()),
            DelegateMethodSpec("scaleChangeEnded", "()", ()),
            DelegateMethodSpec("updateScaleControl", "()", ()),
            DelegateMethodSpec("anchorPointMoved", "()", ('CCPoint',)),
        ),
    ),
    "GJSpecialColorSelectDelegate": DelegateSpec(
        cxx_type="GJSpecialColorSelectDelegate", lua_name="GJSpecialColorSelectDelegate",
        cpp_class="LuaGJSpecialColorSelectDelegate", create_fn="LuaGJSpecialColorSelectDelegate::create",
        methods=(
            DelegateMethodSpec("colorSelectClosed", "()", ('GJSpecialColorSelect', 'number')),
        ),
    ),
    "GJTransformControlDelegate": DelegateSpec(
        cxx_type="GJTransformControlDelegate", lua_name="GJTransformControlDelegate",
        cpp_class="LuaGJTransformControlDelegate", create_fn="LuaGJTransformControlDelegate::create",
        methods=(
            DelegateMethodSpec("transformScaleXChanged", "()", ('number',)),
            DelegateMethodSpec("transformScaleYChanged", "()", ('number',)),
            DelegateMethodSpec("transformScaleXYChanged", "()", ('number', 'number')),
            DelegateMethodSpec("transformRotationXChanged", "()", ('number',)),
            DelegateMethodSpec("transformRotationYChanged", "()", ('number',)),
            DelegateMethodSpec("transformRotationChanged", "()", ('number',)),
            DelegateMethodSpec("transformResetRotation", "()", ()),
            DelegateMethodSpec("transformRestoreRotation", "()", ()),
            DelegateMethodSpec("transformSkewXChanged", "()", ('number',)),
            DelegateMethodSpec("transformSkewYChanged", "()", ('number',)),
            DelegateMethodSpec("transformChangeBegin", "()", ()),
            DelegateMethodSpec("transformChangeEnded", "()", ()),
            DelegateMethodSpec("updateTransformControl", "()", ()),
            DelegateMethodSpec("anchorPointMoved", "()", ('CCPoint',)),
            DelegateMethodSpec("getTransformNode", "CCNode", ()),
            DelegateMethodSpec("getUI", "EditorUI", ()),
        ),
    ),
    "GameRateDelegate": DelegateSpec(
        cxx_type="GameRateDelegate", lua_name="GameRateDelegate",
        cpp_class="LuaGameRateDelegate", create_fn="LuaGameRateDelegate::create",
        methods=(
            DelegateMethodSpec("updateRate", "()", ()),
        ),
    ),
    "GooglePlayDelegate": DelegateSpec(
        cxx_type="GooglePlayDelegate", lua_name="GooglePlayDelegate",
        cpp_class="LuaGooglePlayDelegate", create_fn="LuaGooglePlayDelegate::create",
        methods=(
            DelegateMethodSpec("googlePlaySignedIn", "()", ()),
        ),
    ),
    "HSVWidgetDelegate": DelegateSpec(
        cxx_type="HSVWidgetDelegate", lua_name="HSVWidgetDelegate",
        cpp_class="LuaHSVWidgetDelegate", create_fn="LuaHSVWidgetDelegate::create",
        methods=(
            DelegateMethodSpec("hsvPopupClosed", "()", ('HSVWidgetPopup', 'HSVValue')),
            DelegateMethodSpec("hsvChanged", "()", ('ConfigureHSVWidget',)),
        ),
    ),
    "LeaderboardManagerDelegate": DelegateSpec(
        cxx_type="LeaderboardManagerDelegate", lua_name="LeaderboardManagerDelegate",
        cpp_class="LuaLeaderboardManagerDelegate", create_fn="LuaLeaderboardManagerDelegate::create",
        methods=(
            DelegateMethodSpec("updateUserScoreFinished", "()", ()),
            DelegateMethodSpec("updateUserScoreFailed", "()", ()),
            DelegateMethodSpec("loadLeaderboardFinished", "()", ('CCArray', 'string')),
            DelegateMethodSpec("loadLeaderboardFailed", "()", ('string',)),
        ),
    ),
    "LevelCommentDelegate": DelegateSpec(
        cxx_type="LevelCommentDelegate", lua_name="LevelCommentDelegate",
        cpp_class="LuaLevelCommentDelegate", create_fn="LuaLevelCommentDelegate::create",
        methods=(
            DelegateMethodSpec("loadCommentsFinished", "()", ('CCArray', 'string')),
            DelegateMethodSpec("loadCommentsFailed", "()", ('string',)),
            DelegateMethodSpec("updateUserScoreFinished", "()", ()),
            DelegateMethodSpec("setupPageInfo", "()", ('string', 'string')),
        ),
    ),
    "LevelDeleteDelegate": DelegateSpec(
        cxx_type="LevelDeleteDelegate", lua_name="LevelDeleteDelegate",
        cpp_class="LuaLevelDeleteDelegate", create_fn="LuaLevelDeleteDelegate::create",
        methods=(
            DelegateMethodSpec("levelDeleteFinished", "()", ('number',)),
            DelegateMethodSpec("levelDeleteFailed", "()", ('number',)),
        ),
    ),
    "LevelDownloadDelegate": DelegateSpec(
        cxx_type="LevelDownloadDelegate", lua_name="LevelDownloadDelegate",
        cpp_class="LuaLevelDownloadDelegate", create_fn="LuaLevelDownloadDelegate::create",
        methods=(
            DelegateMethodSpec("levelDownloadFinished", "()", ('GJGameLevel',)),
            DelegateMethodSpec("levelDownloadFailed", "()", ('number',)),
        ),
    ),
    "LevelListDeleteDelegate": DelegateSpec(
        cxx_type="LevelListDeleteDelegate", lua_name="LevelListDeleteDelegate",
        cpp_class="LuaLevelListDeleteDelegate", create_fn="LuaLevelListDeleteDelegate::create",
        methods=(
            DelegateMethodSpec("levelListDeleteFinished", "()", ('number',)),
            DelegateMethodSpec("levelListDeleteFailed", "()", ('number',)),
        ),
    ),
    "LevelManagerDelegate": DelegateSpec(
        cxx_type="LevelManagerDelegate", lua_name="LevelManagerDelegate",
        cpp_class="LuaLevelManagerDelegate", create_fn="LuaLevelManagerDelegate::create",
        methods=(
            DelegateMethodSpec("loadLevelsFinished", "()", ('CCArray', 'string')),
            DelegateMethodSpec("loadLevelsFailed", "()", ('string',)),
            DelegateMethodSpec("loadLevelsFinished", "()", ('CCArray', 'string', 'number')),
            DelegateMethodSpec("loadLevelsFailed", "()", ('string', 'number')),
            DelegateMethodSpec("setupPageInfo", "()", ('string', 'string')),
        ),
    ),
    "LevelRateInfoDelegate": DelegateSpec(
        cxx_type="LevelRateInfoDelegate", lua_name="LevelRateInfoDelegate",
        cpp_class="LuaLevelRateInfoDelegate", create_fn="LuaLevelRateInfoDelegate::create",
        methods=(
            DelegateMethodSpec("rateInfoFinished", "()", ('number', 'string')),
            DelegateMethodSpec("rateInfoFailed", "()", ('number', 'number')),
        ),
    ),
    "LevelSettingsDelegate": DelegateSpec(
        cxx_type="LevelSettingsDelegate", lua_name="LevelSettingsDelegate",
        cpp_class="LuaLevelSettingsDelegate", create_fn="LuaLevelSettingsDelegate::create",
        methods=(
            DelegateMethodSpec("levelSettingsUpdated", "()", ()),
        ),
    ),
    "LevelUpdateDelegate": DelegateSpec(
        cxx_type="LevelUpdateDelegate", lua_name="LevelUpdateDelegate",
        cpp_class="LuaLevelUpdateDelegate", create_fn="LuaLevelUpdateDelegate::create",
        methods=(
            DelegateMethodSpec("levelUpdateFailed", "()", ('number',)),
        ),
    ),
    "LevelUploadDelegate": DelegateSpec(
        cxx_type="LevelUploadDelegate", lua_name="LevelUploadDelegate",
        cpp_class="LuaLevelUploadDelegate", create_fn="LuaLevelUploadDelegate::create",
        methods=(
            DelegateMethodSpec("levelUploadFinished", "()", ('GJGameLevel',)),
            DelegateMethodSpec("levelUploadFailed", "()", ('GJGameLevel',)),
        ),
    ),
    "LikeItemDelegate": DelegateSpec(
        cxx_type="LikeItemDelegate", lua_name="LikeItemDelegate",
        cpp_class="LuaLikeItemDelegate", create_fn="LuaLikeItemDelegate::create",
        methods=(
            DelegateMethodSpec("likedItem", "()", ('number', 'number', 'boolean')),
        ),
    ),
    "ListButtonBarDelegate": DelegateSpec(
        cxx_type="ListButtonBarDelegate", lua_name="ListButtonBarDelegate",
        cpp_class="LuaListButtonBarDelegate", create_fn="LuaListButtonBarDelegate::create",
        methods=(
            DelegateMethodSpec("listButtonBarSwitchedPage", "()", ('ListButtonBar', 'number')),
        ),
    ),
    "ListUploadDelegate": DelegateSpec(
        cxx_type="ListUploadDelegate", lua_name="ListUploadDelegate",
        cpp_class="LuaListUploadDelegate", create_fn="LuaListUploadDelegate::create",
        methods=(
            DelegateMethodSpec("listUploadFinished", "()", ('GJLevelList',)),
            DelegateMethodSpec("listUploadFailed", "()", ('GJLevelList', 'number')),
        ),
    ),
    "MessageListDelegate": DelegateSpec(
        cxx_type="MessageListDelegate", lua_name="MessageListDelegate",
        cpp_class="LuaMessageListDelegate", create_fn="LuaMessageListDelegate::create",
        methods=(
            DelegateMethodSpec("loadMessagesFinished", "()", ('CCArray', 'string')),
            DelegateMethodSpec("forceReloadMessages", "()", ('boolean',)),
            DelegateMethodSpec("setupPageInfo", "()", ('string', 'string')),
        ),
    ),
    "MusicBrowserDelegate": DelegateSpec(
        cxx_type="MusicBrowserDelegate", lua_name="MusicBrowserDelegate",
        cpp_class="LuaMusicBrowserDelegate", create_fn="LuaMusicBrowserDelegate::create",
        methods=(
            DelegateMethodSpec("musicBrowserClosed", "()", ('MusicBrowser',)),
        ),
    ),
    "MusicDownloadDelegate": DelegateSpec(
        cxx_type="MusicDownloadDelegate", lua_name="MusicDownloadDelegate",
        cpp_class="LuaMusicDownloadDelegate", create_fn="LuaMusicDownloadDelegate::create",
        methods=(
            DelegateMethodSpec("loadSongInfoFinished", "()", ('SongInfoObject',)),
            DelegateMethodSpec("loadSongInfoFailed", "()", ('number', 'number')),
            DelegateMethodSpec("downloadSongStarted", "()", ('number',)),
            DelegateMethodSpec("downloadSongFinished", "()", ('number',)),
            DelegateMethodSpec("downloadSongFailed", "()", ('number', 'number')),
            DelegateMethodSpec("songStateChanged", "()", ()),
            DelegateMethodSpec("downloadSFXFinished", "()", ('number',)),
            DelegateMethodSpec("downloadSFXFailed", "()", ('number', 'number')),
            DelegateMethodSpec("musicActionFinished", "()", ('number',)),
            DelegateMethodSpec("musicActionFailed", "()", ('number',)),
        ),
    ),
    "NumberInputDelegate": DelegateSpec(
        cxx_type="NumberInputDelegate", lua_name="NumberInputDelegate",
        cpp_class="LuaNumberInputDelegate", create_fn="LuaNumberInputDelegate::create",
        methods=(
            DelegateMethodSpec("numberInputClosed", "()", ('NumberInputLayer',)),
        ),
    ),
    "OnlineListDelegate": DelegateSpec(
        cxx_type="OnlineListDelegate", lua_name="OnlineListDelegate",
        cpp_class="LuaOnlineListDelegate", create_fn="LuaOnlineListDelegate::create",
        methods=(
            DelegateMethodSpec("loadListFinished", "()", ('CCArray', 'string')),
            DelegateMethodSpec("loadListFailed", "()", ('string',)),
            DelegateMethodSpec("setupPageInfo", "()", ('string', 'string')),
        ),
    ),
    "OptionsObjectDelegate": DelegateSpec(
        cxx_type="OptionsObjectDelegate", lua_name="OptionsObjectDelegate",
        cpp_class="LuaOptionsObjectDelegate", create_fn="LuaOptionsObjectDelegate::create",
        methods=(
            DelegateMethodSpec("stateChanged", "()", ('OptionsObject',)),
        ),
    ),
    "RateLevelDelegate": DelegateSpec(
        cxx_type="RateLevelDelegate", lua_name="RateLevelDelegate",
        cpp_class="LuaRateLevelDelegate", create_fn="LuaRateLevelDelegate::create",
        methods=(
            DelegateMethodSpec("rateLevelClosed", "()", ()),
        ),
    ),
    "RewardedVideoDelegate": DelegateSpec(
        cxx_type="RewardedVideoDelegate", lua_name="RewardedVideoDelegate",
        cpp_class="LuaRewardedVideoDelegate", create_fn="LuaRewardedVideoDelegate::create",
        methods=(
            DelegateMethodSpec("rewardedVideoFinished", "()", ()),
            DelegateMethodSpec("shouldOffsetRewardCurrency", "boolean", ()),
        ),
    ),
    "SFXBrowserDelegate": DelegateSpec(
        cxx_type="SFXBrowserDelegate", lua_name="SFXBrowserDelegate",
        cpp_class="LuaSFXBrowserDelegate", create_fn="LuaSFXBrowserDelegate::create",
        methods=(
            DelegateMethodSpec("sfxBrowserClosed", "()", ('SFXBrowser',)),
        ),
    ),
    "SelectArtDelegate": DelegateSpec(
        cxx_type="SelectArtDelegate", lua_name="SelectArtDelegate",
        cpp_class="LuaSelectArtDelegate", create_fn="LuaSelectArtDelegate::create",
        methods=(
            DelegateMethodSpec("selectArtClosed", "()", ('SelectArtLayer',)),
        ),
    ),
    "SelectListIconDelegate": DelegateSpec(
        cxx_type="SelectListIconDelegate", lua_name="SelectListIconDelegate",
        cpp_class="LuaSelectListIconDelegate", create_fn="LuaSelectListIconDelegate::create",
        methods=(
            DelegateMethodSpec("iconSelectClosed", "()", ('SelectListIconLayer',)),
        ),
    ),
    "SelectPremadeDelegate": DelegateSpec(
        cxx_type="SelectPremadeDelegate", lua_name="SelectPremadeDelegate",
        cpp_class="LuaSelectPremadeDelegate", create_fn="LuaSelectPremadeDelegate::create",
        methods=(
            DelegateMethodSpec("selectPremadeClosed", "()", ('SelectPremadeLayer', 'number')),
        ),
    ),
    "SelectSFXSortDelegate": DelegateSpec(
        cxx_type="SelectSFXSortDelegate", lua_name="SelectSFXSortDelegate",
        cpp_class="LuaSelectSFXSortDelegate", create_fn="LuaSelectSFXSortDelegate::create",
        methods=(
            DelegateMethodSpec("sortSelectClosed", "()", ('SelectSFXSortLayer',)),
        ),
    ),
    "SelectSettingDelegate": DelegateSpec(
        cxx_type="SelectSettingDelegate", lua_name="SelectSettingDelegate",
        cpp_class="LuaSelectSettingDelegate", create_fn="LuaSelectSettingDelegate::create",
        methods=(
            DelegateMethodSpec("selectSettingClosed", "()", ('SelectSettingLayer',)),
        ),
    ),
    "SetIDPopupDelegate": DelegateSpec(
        cxx_type="SetIDPopupDelegate", lua_name="SetIDPopupDelegate",
        cpp_class="LuaSetIDPopupDelegate", create_fn="LuaSetIDPopupDelegate::create",
        methods=(
            DelegateMethodSpec("setIDPopupClosed", "()", ('SetIDPopup', 'number')),
        ),
    ),
    "SetTextPopupDelegate": DelegateSpec(
        cxx_type="SetTextPopupDelegate", lua_name="SetTextPopupDelegate",
        cpp_class="LuaSetTextPopupDelegate", create_fn="LuaSetTextPopupDelegate::create",
        methods=(
            DelegateMethodSpec("setTextPopupClosed", "()", ('SetTextPopup', 'string')),
        ),
    ),
    "ShareCommentDelegate": DelegateSpec(
        cxx_type="ShareCommentDelegate", lua_name="ShareCommentDelegate",
        cpp_class="LuaShareCommentDelegate", create_fn="LuaShareCommentDelegate::create",
        methods=(
            DelegateMethodSpec("shareCommentClosed", "()", ('string', 'ShareCommentLayer')),
        ),
    ),
    "SliderDelegate": DelegateSpec(
        cxx_type="SliderDelegate", lua_name="SliderDelegate",
        cpp_class="LuaSliderDelegate", create_fn="LuaSliderDelegate::create",
        methods=(
            DelegateMethodSpec("sliderBegan", "()", ('Slider',)),
            DelegateMethodSpec("sliderEnded", "()", ('Slider',)),
        ),
    ),
    "SongPlaybackDelegate": DelegateSpec(
        cxx_type="SongPlaybackDelegate", lua_name="SongPlaybackDelegate",
        cpp_class="LuaSongPlaybackDelegate", create_fn="LuaSongPlaybackDelegate::create",
        methods=(
            DelegateMethodSpec("onPlayback", "()", ('SongInfoObject',)),
        ),
    ),
    "SpritePartDelegate": DelegateSpec(
        cxx_type="SpritePartDelegate", lua_name="SpritePartDelegate",
        cpp_class="LuaSpritePartDelegate", create_fn="LuaSpritePartDelegate::create",
        methods=(
            DelegateMethodSpec("displayFrameChanged", "()", ('CCObject', 'string')),
        ),
    ),
    "TableViewCellDelegate": DelegateSpec(
        cxx_type="TableViewCellDelegate", lua_name="TableViewCellDelegate",
        cpp_class="LuaTableViewCellDelegate", create_fn="LuaTableViewCellDelegate::create",
        methods=(
            DelegateMethodSpec("cellPerformedAction", "boolean", ('TableViewCell', 'number', 'number', 'CCNode')),
            DelegateMethodSpec("getSelectedCellIdx", "number", ()),
            DelegateMethodSpec("shouldSnapToSelected", "boolean", ()),
            DelegateMethodSpec("getCellDelegateType", "number", ()),
        ),
    ),
    "TableViewDelegate": DelegateSpec(
        cxx_type="TableViewDelegate", lua_name="TableViewDelegate",
        cpp_class="LuaTableViewDelegate", create_fn="LuaTableViewDelegate::create",
        methods=(
            DelegateMethodSpec("willTweenToIndexPath", "()", ('CCIndexPath', 'TableViewCell', 'TableView')),
            DelegateMethodSpec("didEndTweenToIndexPath", "()", ('CCIndexPath', 'TableView')),
            DelegateMethodSpec("TableViewWillDisplayCellForRowAtIndexPath", "()", ('CCIndexPath', 'TableViewCell', 'TableView')),
            DelegateMethodSpec("TableViewDidDisplayCellForRowAtIndexPath", "()", ('CCIndexPath', 'TableViewCell', 'TableView')),
            DelegateMethodSpec("TableViewWillReloadCellForRowAtIndexPath", "()", ('CCIndexPath', 'TableViewCell', 'TableView')),
            DelegateMethodSpec("didSelectRowAtIndexPath", "()", ('CCIndexPath', 'TableView')),
        ),
    ),
    "TextAreaDelegate": DelegateSpec(
        cxx_type="TextAreaDelegate", lua_name="TextAreaDelegate",
        cpp_class="LuaTextAreaDelegate", create_fn="LuaTextAreaDelegate::create",
        methods=(
            DelegateMethodSpec("fadeInTextFinished", "()", ('TextArea',)),
        ),
    ),
    "TextInputDelegate": DelegateSpec(
        cxx_type="TextInputDelegate", lua_name="TextInputDelegate",
        cpp_class="LuaTextInputDelegate", create_fn="LuaTextInputDelegate::create",
        methods=(
            DelegateMethodSpec("textChanged", "()", ('CCTextInputNode',)),
            DelegateMethodSpec("textInputOpened", "()", ('CCTextInputNode',)),
            DelegateMethodSpec("textInputClosed", "()", ('CCTextInputNode',)),
            DelegateMethodSpec("textInputShouldOffset", "()", ('CCTextInputNode', 'number')),
            DelegateMethodSpec("textInputReturn", "()", ('CCTextInputNode',)),
            DelegateMethodSpec("allowTextInput", "boolean", ('CCTextInputNode',)),
            DelegateMethodSpec("enterPressed", "()", ('CCTextInputNode',)),
        ),
    ),
    "UploadActionDelegate": DelegateSpec(
        cxx_type="UploadActionDelegate", lua_name="UploadActionDelegate",
        cpp_class="LuaUploadActionDelegate", create_fn="LuaUploadActionDelegate::create",
        methods=(
            DelegateMethodSpec("uploadActionFinished", "()", ('number', 'number')),
            DelegateMethodSpec("uploadActionFailed", "()", ('number', 'number')),
        ),
    ),
    "UploadMessageDelegate": DelegateSpec(
        cxx_type="UploadMessageDelegate", lua_name="UploadMessageDelegate",
        cpp_class="LuaUploadMessageDelegate", create_fn="LuaUploadMessageDelegate::create",
        methods=(
            DelegateMethodSpec("uploadMessageFinished", "()", ('number',)),
            DelegateMethodSpec("uploadMessageFailed", "()", ('number',)),
        ),
    ),
    "UploadPopupDelegate": DelegateSpec(
        cxx_type="UploadPopupDelegate", lua_name="UploadPopupDelegate",
        cpp_class="LuaUploadPopupDelegate", create_fn="LuaUploadPopupDelegate::create",
        methods=(
            DelegateMethodSpec("onClosePopup", "()", ('UploadActionPopup',)),
        ),
    ),
    "UserInfoDelegate": DelegateSpec(
        cxx_type="UserInfoDelegate", lua_name="UserInfoDelegate",
        cpp_class="LuaUserInfoDelegate", create_fn="LuaUserInfoDelegate::create",
        methods=(
            DelegateMethodSpec("getUserInfoFinished", "()", ('GJUserScore',)),
            DelegateMethodSpec("getUserInfoFailed", "()", ('number',)),
            DelegateMethodSpec("userInfoChanged", "()", ('GJUserScore',)),
        ),
    ),
    "UserListDelegate": DelegateSpec(
        cxx_type="UserListDelegate", lua_name="UserListDelegate",
        cpp_class="LuaUserListDelegate", create_fn="LuaUserListDelegate::create",
        methods=(
            DelegateMethodSpec("getUserListFinished", "()", ('CCArray', 'number')),
            DelegateMethodSpec("userListChanged", "()", ('CCArray', 'number')),
            DelegateMethodSpec("forceReloadList", "()", ('number',)),
        ),
    ),
    "cocos2d::CCDirectorDelegate": DelegateSpec(
        cxx_type="cocos2d::CCDirectorDelegate", lua_name="CCDirectorDelegate",
        cpp_class="LuaCCDirectorDelegate", create_fn="LuaCCDirectorDelegate::create",
        methods=(
            DelegateMethodSpec("updateProjection", "()", ()),
        ),
    ),
    "cocos2d::CCIMEDelegate": DelegateSpec(
        cxx_type="cocos2d::CCIMEDelegate", lua_name="CCIMEDelegate",
        cpp_class="LuaCCIMEDelegate", create_fn="LuaCCIMEDelegate::create",
        methods=(
            DelegateMethodSpec("attachWithIME", "boolean", ()),
            DelegateMethodSpec("detachWithIME", "boolean", ()),
        ),
    ),
    "cocos2d::CCKeyboardDelegate": DelegateSpec(
        cxx_type="cocos2d::CCKeyboardDelegate", lua_name="CCKeyboardDelegate",
        cpp_class="LuaCCKeyboardDelegate", create_fn="LuaCCKeyboardDelegate::create",
        methods=(
            DelegateMethodSpec("keyDown", "()", ('number', 'number')),
            DelegateMethodSpec("keyUp", "()", ('number', 'number')),
        ),
    ),
    "cocos2d::CCKeypadDelegate": DelegateSpec(
        cxx_type="cocos2d::CCKeypadDelegate", lua_name="CCKeypadDelegate",
        cpp_class="LuaCCKeypadDelegate", create_fn="LuaCCKeypadDelegate::create",
        methods=(
            DelegateMethodSpec("keyBackClicked", "()", ()),
            DelegateMethodSpec("keyMenuClicked", "()", ()),
        ),
    ),
    "cocos2d::CCMouseDelegate": DelegateSpec(
        cxx_type="cocos2d::CCMouseDelegate", lua_name="CCMouseDelegate",
        cpp_class="LuaCCMouseDelegate", create_fn="LuaCCMouseDelegate::create",
        methods=(
            DelegateMethodSpec("rightKeyDown", "()", ()),
            DelegateMethodSpec("rightKeyUp", "()", ()),
            DelegateMethodSpec("scrollWheel", "()", ('number', 'number')),
        ),
    ),
    "cocos2d::CCTextFieldDelegate": DelegateSpec(
        cxx_type="cocos2d::CCTextFieldDelegate", lua_name="CCTextFieldDelegate",
        cpp_class="LuaCCTextFieldDelegate", create_fn="LuaCCTextFieldDelegate::create",
        methods=(
            DelegateMethodSpec("onTextFieldAttachWithIME", "boolean", ('CCTextFieldTTF',)),
            DelegateMethodSpec("onTextFieldDetachWithIME", "boolean", ('CCTextFieldTTF',)),
            DelegateMethodSpec("onTextFieldInsertText", "boolean", ('CCTextFieldTTF', 'string', 'number', 'number')),
            DelegateMethodSpec("onTextFieldDeleteBackward", "boolean", ('CCTextFieldTTF', 'string', 'number')),
            DelegateMethodSpec("onDraw", "boolean", ('CCTextFieldTTF',)),
            DelegateMethodSpec("textChanged", "()", ()),
        ),
    ),
    "cocos2d::CCTouchDelegate": DelegateSpec(
        cxx_type="cocos2d::CCTouchDelegate", lua_name="CCTouchDelegate",
        cpp_class="LuaCCTouchDelegate", create_fn="LuaCCTouchDelegate::create",
        methods=(
            DelegateMethodSpec("ccTouchBegan", "boolean", ('CCTouch', 'CCEvent')),
            DelegateMethodSpec("ccTouchMoved", "()", ('CCTouch', 'CCEvent')),
            DelegateMethodSpec("ccTouchEnded", "()", ('CCTouch', 'CCEvent')),
            DelegateMethodSpec("ccTouchCancelled", "()", ('CCTouch', 'CCEvent')),
            DelegateMethodSpec("ccTouchesBegan", "()", ('CCSet', 'CCEvent')),
            DelegateMethodSpec("ccTouchesMoved", "()", ('CCSet', 'CCEvent')),
            DelegateMethodSpec("ccTouchesEnded", "()", ('CCSet', 'CCEvent')),
            DelegateMethodSpec("ccTouchesCancelled", "()", ('CCSet', 'CCEvent')),
            DelegateMethodSpec("setPreviousPriority", "()", ('number',)),
            DelegateMethodSpec("getPreviousPriority", "number", ()),
        ),
    ),
    "cocos2d::extension::CCEditBoxDelegate": DelegateSpec(
        cxx_type="cocos2d::extension::CCEditBoxDelegate", lua_name="CCEditBoxDelegate",
        cpp_class="LuaCCEditBoxDelegate", create_fn="LuaCCEditBoxDelegate::create",
        methods=(
            DelegateMethodSpec("editBoxEditingDidBegin", "()", ('CCEditBox',)),
            DelegateMethodSpec("editBoxEditingDidEnd", "()", ('CCEditBox',)),
            DelegateMethodSpec("editBoxReturn", "()", ('CCEditBox',)),
        ),
    ),
}
DELEGATE_CXX_TYPES: FrozenSet[str] = frozenset(DELEGATE_SPECS.keys())

def lookup_delegate(cxx_ptr_type: str) -> Optional[DelegateSpec]:
    n = cxx_ptr_type.strip().removesuffix('*').strip()
    return DELEGATE_SPECS.get(n) or DELEGATE_SPECS.get(n.split('::')[-1])
