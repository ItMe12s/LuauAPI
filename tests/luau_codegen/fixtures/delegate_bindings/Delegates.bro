// Offline CI fixture, minimal delegate bindings snapshot
class AnimatedSpriteDelegate {
    virtual void animationFinished(char const* key) {}
};

class BoomScrollLayerDelegate {
    virtual void scrollLayerScrollingStarted(BoomScrollLayer* layer) {}
    virtual void scrollLayerScrolledToPage(BoomScrollLayer* layer, int page) {}
    virtual void scrollLayerMoved(cocos2d::CCPoint position) {}
    virtual void scrollLayerWillScrollToPage(BoomScrollLayer* layer, int page) {}
};

class CCCircleWaveDelegate {
    virtual void circleWaveWillBeRemoved(CCCircleWave* circleWave) {}
};

class CCScrollLayerExtDelegate {
    virtual void scrllViewWillBeginDecelerating(CCScrollLayerExt* layer) {}
    virtual void scrollViewDidEndDecelerating(CCScrollLayerExt* layer) {}
    virtual void scrollViewTouchMoving(CCScrollLayerExt* layer) {}
    virtual void scrollViewDidEndMoving(CCScrollLayerExt* layer) {}
    virtual void scrollViewTouchBegin(CCScrollLayerExt* layer) {}
    virtual void scrollViewTouchEnd(CCScrollLayerExt* layer) {}
};

class CharacterColorDelegate {
    virtual void playerColorChanged() {}
    virtual void showUnlockPopup(int id, UnlockType type) {}
};

class ColorSelectDelegate {
    virtual void colorSelectClosed(cocos2d::CCNode* popup) {}
};

class ColorSetupDelegate {
    virtual void colorSetupClosed(int id) {}
};

class CommentUploadDelegate {
    virtual void commentUploadFinished(int parentID) {}
    virtual void commentUploadFailed(int parentID, CommentError errorType) {}
    virtual void commentDeleteFailed(int id, int parentID) {}
};

class ConfigureValuePopupDelegate {
    virtual void valuePopupClosed(ConfigureValuePopup* popup, float value) {}
};

class CurrencyRewardDelegate {
    virtual void currencyWillExit(CurrencyRewardLayer* layer) {}
};

class CustomSFXDelegate {
    virtual void sfxObjectSelected(SFXInfoObject* object) {}
    virtual int getActiveSFXID() {}
    virtual bool overridePlaySFX(SFXInfoObject* object) {}
};

class CustomSongDelegate {
    virtual void songIDChanged(int id) {}
    virtual int getActiveSongID() {}
    virtual gd::string getSongFileName() {}
    virtual LevelSettingsObject* getLevelSettings() {}
};

class CustomSongLayerDelegate {
    virtual void customSongLayerClosed() {}
};

class DemonFilterDelegate {
    virtual void demonFilterSelectClosed(int filter) {}
};

class DialogDelegate {
    virtual void dialogClosed(DialogLayer* layer) {}
};

class DownloadMessageDelegate {
    virtual void downloadMessageFinished(GJUserMessage* message) {}
    virtual void downloadMessageFailed(int id) {}
};

class DynamicScrollDelegate {
    virtual void updatePageWithObject(cocos2d::CCObject* layer, cocos2d::CCObject* object) {}
};

class FLAlertLayerProtocol {
    virtual void FLAlert_Clicked(FLAlertLayer* layer, bool btn2) {}
};

class FriendRequestDelegate {
    virtual void loadFRequestsFinished(cocos2d::CCArray* scores, char const* key) {}
    virtual void setupPageInfo(gd::string info, char const* key) {}
    virtual void forceReloadRequests(bool sent) {}
};

class GJAccountBackupDelegate {
    virtual void backupAccountFinished() {}
    virtual void backupAccountFailed(BackupAccountError errorType, int response) {}
};

class GJAccountDelegate {
    virtual void accountStatusChanged() {}
};

class GJAccountLoginDelegate {
    virtual void loginAccountFinished(int accountID, int userID) {}
    virtual void loginAccountFailed(AccountError errorType) {}
};

class GJAccountRegisterDelegate {
    virtual void registerAccountFinished() {}
    virtual void registerAccountFailed(AccountError errorType) {}
};

class GJAccountSettingsDelegate {
    virtual void updateSettingsFinished() {}
    virtual void updateSettingsFailed() {}
};

class GJAccountSyncDelegate {
    virtual void syncAccountFinished() {}
    virtual void syncAccountFailed(BackupAccountError errorType, int response) {}
};

class GJChallengeDelegate {
    virtual void challengeStatusFinished() {}
    virtual void challengeStatusFailed() {}
};

class GJDailyLevelDelegate {
    virtual void dailyStatusFinished(GJTimedLevelType type) {}
};

class GJDropDownLayerDelegate {
    virtual void dropDownLayerWillClose(GJDropDownLayer* layer) {}
};

class GJMPDelegate {
    virtual void joinLobbyFinished(int id) {}
    virtual void didUploadMPComment(int id) {}
    virtual void updateComments() {}
};

class GJOnlineRewardDelegate {
    virtual void onlineRewardStatusFinished(gd::string key) {}
    virtual void onlineRewardStatusFailed() {}
};

class GJPurchaseDelegate {
    virtual void didPurchaseItem(GJStoreItem* item) {}
};

class GJRewardDelegate {
    virtual void rewardsStatusFinished(int type) {}
    virtual void rewardsStatusFailed() {}
};

class GJRotationControlDelegate {
    virtual void angleChanged(float angle) {}
    virtual void angleChangeBegin() {}
    virtual void angleChangeEnded() {}
};

class GJScaleControlDelegate {
    virtual void scaleXChanged(float scaleX, bool lock) {}
    virtual void scaleYChanged(float scaleY, bool lock) {}
    virtual void scaleXYChanged(float scaleX, float scaleY, bool lock) {}
    virtual void scaleChangeBegin() {}
    virtual void scaleChangeEnded() {}
    virtual void updateScaleControl() {}
    virtual void anchorPointMoved(cocos2d::CCPoint newAnchor) {}
};

class GJSpecialColorSelectDelegate {
    virtual void colorSelectClosed(GJSpecialColorSelect* select, int id) {}
};

class GJTransformControlDelegate {
    virtual void transformScaleXChanged(float scaleX) {}
    virtual void transformScaleYChanged(float scaleY) {}
    virtual void transformScaleXYChanged(float scaleX, float scaleY) {}
    virtual void transformRotationXChanged(float rotationX) {}
    virtual void transformRotationYChanged(float rotationY) {}
    virtual void transformRotationChanged(float rotation) {}
    virtual void transformResetRotation() {}
    virtual void transformRestoreRotation() {}
    virtual void transformSkewXChanged(float skewX) {}
    virtual void transformSkewYChanged(float skewY) {}
    virtual void transformChangeBegin() {}
    virtual void transformChangeEnded() {}
    virtual void updateTransformControl() {}
    virtual void anchorPointMoved(cocos2d::CCPoint anchorPoint) {}
    virtual cocos2d::CCNode* getTransformNode() {}
    virtual EditorUI* getUI() {}
};

class GameRateDelegate {
    virtual void updateRate() {}
};

class GooglePlayDelegate {
    virtual void googlePlaySignedIn() {}
};

class HSVWidgetDelegate {
    virtual void hsvPopupClosed(HSVWidgetPopup* popup, cocos2d::ccHSVValue value) {}
    virtual void hsvChanged(ConfigureHSVWidget* widget) {}
};

class LeaderboardManagerDelegate {
    virtual void updateUserScoreFinished() {}
    virtual void updateUserScoreFailed() {}
    virtual void loadLeaderboardFinished(cocos2d::CCArray* scores, char const* key) {}
    virtual void loadLeaderboardFailed(char const* key) {}
};

class LevelCommentDelegate {
    virtual void loadCommentsFinished(cocos2d::CCArray* comments, char const* key) {}
    virtual void loadCommentsFailed(char const* key) {}
    virtual void updateUserScoreFinished() {}
    virtual void setupPageInfo(gd::string info, char const* key) {}
};

class LevelDeleteDelegate {
    virtual void levelDeleteFinished(int id) {}
    virtual void levelDeleteFailed(int id) {}
};

class LevelDownloadDelegate {
    virtual void levelDownloadFinished(GJGameLevel* level) {}
    virtual void levelDownloadFailed(int response) {}
};

class LevelListDeleteDelegate {
    virtual void levelListDeleteFinished(int id) {}
    virtual void levelListDeleteFailed(int id) {}
};

class LevelManagerDelegate {
    virtual void loadLevelsFinished(cocos2d::CCArray* levels, char const* key) {}
    virtual void loadLevelsFailed(char const* key) {}
    virtual void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int type) {}
    virtual void loadLevelsFailed(char const* key, int type) {}
    virtual void setupPageInfo(gd::string info, char const* key) {}
};

class LevelRateInfoDelegate {
    virtual void rateInfoFinished(int id, gd::string info) {}
    virtual void rateInfoFailed(int id, int response) {}
};

class LevelSettingsDelegate {
    virtual void levelSettingsUpdated() {}
};

class LevelUpdateDelegate {
    virtual void levelUpdateFailed(int response) {}
};

class LevelUploadDelegate {
    virtual void levelUploadFinished(GJGameLevel* level) {}
    virtual void levelUploadFailed(GJGameLevel* level) {}
};

class LikeItemDelegate {
    virtual void likedItem(LikeItemType type, int id, bool liked) {}
};

class ListButtonBarDelegate {
    virtual void listButtonBarSwitchedPage(ListButtonBar* bar, int page) {}
};

class ListUploadDelegate {
    virtual void listUploadFinished(GJLevelList* list) {}
    virtual void listUploadFailed(GJLevelList* list, int response) {}
};

class MessageListDelegate {
    virtual void loadMessagesFinished(cocos2d::CCArray* messages, char const* key) {}
    virtual void forceReloadMessages(bool sent) {}
    virtual void setupPageInfo(gd::string info, char const* key) {}
};

class MusicBrowserDelegate {
    virtual void musicBrowserClosed(MusicBrowser* browser) {}
};

class MusicDownloadDelegate {
    virtual void loadSongInfoFinished(SongInfoObject* object) {}
    virtual void loadSongInfoFailed(int id, GJSongError errorType) {}
    virtual void downloadSongStarted(int id) {}
    virtual void downloadSongFinished(int id) {}
    virtual void downloadSongFailed(int id, GJSongError errorType) {}
    virtual void songStateChanged() {}
    virtual void downloadSFXFinished(int id) {}
    virtual void downloadSFXFailed(int id, GJSongError errorType) {}
    virtual void musicActionFinished(GJMusicAction action) {}
    virtual void musicActionFailed(GJMusicAction action) {}
};

class NumberInputDelegate {
    virtual void numberInputClosed(NumberInputLayer* layer) {}
};

class OnlineListDelegate {
    virtual void loadListFinished(cocos2d::CCArray* objects, char const* key) {}
    virtual void loadListFailed(char const* key) {}
    virtual void setupPageInfo(gd::string info, char const* key) {}
};

class OptionsObjectDelegate {
    virtual void stateChanged(OptionsObject* object) {}
};

class RateLevelDelegate {
    virtual void rateLevelClosed() {}
};

class RewardedVideoDelegate {
    virtual void rewardedVideoFinished() {}
    virtual bool shouldOffsetRewardCurrency() {}
};

class SFXBrowserDelegate {
    virtual void sfxBrowserClosed(SFXBrowser* browser) {}
};

class SelectArtDelegate {
    virtual void selectArtClosed(SelectArtLayer* layer) {}
};

class SelectListIconDelegate {
    virtual void iconSelectClosed(SelectListIconLayer* layer) {}
};

class SelectPremadeDelegate {
    virtual void selectPremadeClosed(SelectPremadeLayer* layer, int type) {}
};

class SelectSFXSortDelegate {
    virtual void sortSelectClosed(SelectSFXSortLayer* layer) {}
};

class SelectSettingDelegate {
    virtual void selectSettingClosed(SelectSettingLayer* layer) {}
};

class SetIDPopupDelegate {
    virtual void setIDPopupClosed(SetIDPopup* popup, int value) {}
};

class SetTextPopupDelegate {
    virtual void setTextPopupClosed(SetTextPopup* popup, gd::string text) {}
};

class ShareCommentDelegate {
    virtual void shareCommentClosed(gd::string text, ShareCommentLayer* layer) {}
};

class SliderDelegate {
    virtual void sliderBegan(Slider* slider) {}
    virtual void sliderEnded(Slider* slider) {}
};

class SongPlaybackDelegate {
    virtual void onPlayback(SongInfoObject* object) {}
};

class SpritePartDelegate {
    virtual void displayFrameChanged(cocos2d::CCObject* sprite, gd::string frameName) {}
};

class TableViewCellDelegate {
    virtual bool cellPerformedAction(TableViewCell* cell, int listType, CellAction action, cocos2d::CCNode* parent) {}
    virtual int getSelectedCellIdx() {}
    virtual bool shouldSnapToSelected() {}
    virtual int getCellDelegateType() {}
};

class TableViewDelegate {
    virtual void willTweenToIndexPath(CCIndexPath indexPath, TableViewCell* cell, TableView* tableView) {}
    virtual void didEndTweenToIndexPath(CCIndexPath indexPath, TableView* tableView) {}
    virtual void TableViewWillDisplayCellForRowAtIndexPath(CCIndexPath indexPath, TableViewCell* cell, TableView* tableView) {}
    virtual void TableViewDidDisplayCellForRowAtIndexPath(CCIndexPath indexPath, TableViewCell* cell, TableView* tableView) {}
    virtual void TableViewWillReloadCellForRowAtIndexPath(CCIndexPath indexPath, TableViewCell* cell, TableView* tableView) {}
    virtual float cellHeightForRowAtIndexPath(CCIndexPath indexPath, TableView* tableView) {}
    virtual void didSelectRowAtIndexPath(CCIndexPath indexPath, TableView* tableView) {}
};

class TextAreaDelegate {
    virtual void fadeInTextFinished(TextArea* textArea) {}
};

class TextInputDelegate {
    virtual void textChanged(CCTextInputNode* node) {}
    virtual void textInputOpened(CCTextInputNode* node) {}
    virtual void textInputClosed(CCTextInputNode* node) {}
    virtual void textInputShouldOffset(CCTextInputNode* node, float yOffset) {}
    virtual void textInputReturn(CCTextInputNode* node) {}
    virtual bool allowTextInput(CCTextInputNode* node) {}
    virtual void enterPressed(CCTextInputNode* node) {}
};

class TriggerEffectDelegate {
    virtual bool checkSpawnAbuse() {}
};

class UploadActionDelegate {
    virtual void uploadActionFinished(int id, int response) {}
    virtual void uploadActionFailed(int id, int response) {}
};

class UploadMessageDelegate {
    virtual void uploadMessageFinished(int accountID) {}
    virtual void uploadMessageFailed(int accountID) {}
};

class UploadPopupDelegate {
    virtual void onClosePopup(UploadActionPopup* popup) {}
};

class UserInfoDelegate {
    virtual void getUserInfoFinished(GJUserScore* score) {}
    virtual void getUserInfoFailed(int id) {}
    virtual void userInfoChanged(GJUserScore* score) {}
};

class UserListDelegate {
    virtual void getUserListFinished(cocos2d::CCArray* scores, UserListType type) {}
    virtual void userListChanged(cocos2d::CCArray* scores, UserListType type) {}
    virtual void forceReloadList(UserListType type) {}
};

class CCIMEDelegate {
    virtual bool attachWithIME() {}
    virtual bool detachWithIME() {}
};

class CCTouchDelegate {
    virtual void setPreviousPriority(int arg) {}
    virtual int getPreviousPriority() {}
};

class DelegatePtrHost {
    void set_AnimatedSpriteDelegate(AnimatedSpriteDelegate* delegate) {}
    void set_AppDelegate(AppDelegate* delegate) {}
    void set_BoomScrollLayerDelegate(BoomScrollLayerDelegate* delegate) {}
    void set_CCCircleWaveDelegate(CCCircleWaveDelegate* delegate) {}
    void set_CCScrollLayerExtDelegate(CCScrollLayerExtDelegate* delegate) {}
    void set_CharacterColorDelegate(CharacterColorDelegate* delegate) {}
    void set_ColorSelectDelegate(ColorSelectDelegate* delegate) {}
    void set_ColorSetupDelegate(ColorSetupDelegate* delegate) {}
    void set_CommentUploadDelegate(CommentUploadDelegate* delegate) {}
    void set_ConfigureValuePopupDelegate(ConfigureValuePopupDelegate* delegate) {}
    void set_CurrencyRewardDelegate(CurrencyRewardDelegate* delegate) {}
    void set_CustomSFXDelegate(CustomSFXDelegate* delegate) {}
    void set_CustomSongDelegate(CustomSongDelegate* delegate) {}
    void set_CustomSongLayerDelegate(CustomSongLayerDelegate* delegate) {}
    void set_DemonFilterDelegate(DemonFilterDelegate* delegate) {}
    void set_DialogDelegate(DialogDelegate* delegate) {}
    void set_DownloadMessageDelegate(DownloadMessageDelegate* delegate) {}
    void set_DynamicScrollDelegate(DynamicScrollDelegate* delegate) {}
    void set_FLAlertLayerProtocol(FLAlertLayerProtocol* delegate) {}
    void set_FriendRequestDelegate(FriendRequestDelegate* delegate) {}
    void set_GJAccountBackupDelegate(GJAccountBackupDelegate* delegate) {}
    void set_GJAccountDelegate(GJAccountDelegate* delegate) {}
    void set_GJAccountLoginDelegate(GJAccountLoginDelegate* delegate) {}
    void set_GJAccountRegisterDelegate(GJAccountRegisterDelegate* delegate) {}
    void set_GJAccountSettingsDelegate(GJAccountSettingsDelegate* delegate) {}
    void set_GJAccountSyncDelegate(GJAccountSyncDelegate* delegate) {}
    void set_GJChallengeDelegate(GJChallengeDelegate* delegate) {}
    void set_GJDailyLevelDelegate(GJDailyLevelDelegate* delegate) {}
    void set_GJDropDownLayerDelegate(GJDropDownLayerDelegate* delegate) {}
    void set_GJMPDelegate(GJMPDelegate* delegate) {}
    void set_GJOnlineRewardDelegate(GJOnlineRewardDelegate* delegate) {}
    void set_GJPurchaseDelegate(GJPurchaseDelegate* delegate) {}
    void set_GJRewardDelegate(GJRewardDelegate* delegate) {}
    void set_GJRotationControlDelegate(GJRotationControlDelegate* delegate) {}
    void set_GJScaleControlDelegate(GJScaleControlDelegate* delegate) {}
    void set_GJSpecialColorSelectDelegate(GJSpecialColorSelectDelegate* delegate) {}
    void set_GJTransformControlDelegate(GJTransformControlDelegate* delegate) {}
    void set_GameRateDelegate(GameRateDelegate* delegate) {}
    void set_GooglePlayDelegate(GooglePlayDelegate* delegate) {}
    void set_HSVWidgetDelegate(HSVWidgetDelegate* delegate) {}
    void set_LeaderboardManagerDelegate(LeaderboardManagerDelegate* delegate) {}
    void set_LevelCommentDelegate(LevelCommentDelegate* delegate) {}
    void set_LevelDeleteDelegate(LevelDeleteDelegate* delegate) {}
    void set_LevelDownloadDelegate(LevelDownloadDelegate* delegate) {}
    void set_LevelListDeleteDelegate(LevelListDeleteDelegate* delegate) {}
    void set_LevelManagerDelegate(LevelManagerDelegate* delegate) {}
    void set_LevelRateInfoDelegate(LevelRateInfoDelegate* delegate) {}
    void set_LevelSettingsDelegate(LevelSettingsDelegate* delegate) {}
    void set_LevelUpdateDelegate(LevelUpdateDelegate* delegate) {}
    void set_LevelUploadDelegate(LevelUploadDelegate* delegate) {}
    void set_LikeItemDelegate(LikeItemDelegate* delegate) {}
    void set_ListButtonBarDelegate(ListButtonBarDelegate* delegate) {}
    void set_ListUploadDelegate(ListUploadDelegate* delegate) {}
    void set_MessageListDelegate(MessageListDelegate* delegate) {}
    void set_MusicBrowserDelegate(MusicBrowserDelegate* delegate) {}
    void set_MusicDownloadDelegate(MusicDownloadDelegate* delegate) {}
    void set_NumberInputDelegate(NumberInputDelegate* delegate) {}
    void set_OnlineListDelegate(OnlineListDelegate* delegate) {}
    void set_OptionsObjectDelegate(OptionsObjectDelegate* delegate) {}
    void set_RateLevelDelegate(RateLevelDelegate* delegate) {}
    void set_RewardedVideoDelegate(RewardedVideoDelegate* delegate) {}
    void set_SFXBrowserDelegate(SFXBrowserDelegate* delegate) {}
    void set_SelectArtDelegate(SelectArtDelegate* delegate) {}
    void set_SelectListIconDelegate(SelectListIconDelegate* delegate) {}
    void set_SelectPremadeDelegate(SelectPremadeDelegate* delegate) {}
    void set_SelectSFXSortDelegate(SelectSFXSortDelegate* delegate) {}
    void set_SelectSettingDelegate(SelectSettingDelegate* delegate) {}
    void set_SetIDPopupDelegate(SetIDPopupDelegate* delegate) {}
    void set_SetTextPopupDelegate(SetTextPopupDelegate* delegate) {}
    void set_ShareCommentDelegate(ShareCommentDelegate* delegate) {}
    void set_SliderDelegate(SliderDelegate* delegate) {}
    void set_SongPlaybackDelegate(SongPlaybackDelegate* delegate) {}
    void set_SpritePartDelegate(SpritePartDelegate* delegate) {}
    void set_TableViewCellDelegate(TableViewCellDelegate* delegate) {}
    void set_TableViewDelegate(TableViewDelegate* delegate) {}
    void set_TextAreaDelegate(TextAreaDelegate* delegate) {}
    void set_TextInputDelegate(TextInputDelegate* delegate) {}
    void set_TriggerEffectDelegate(TriggerEffectDelegate* delegate) {}
    void set_UploadActionDelegate(UploadActionDelegate* delegate) {}
    void set_UploadMessageDelegate(UploadMessageDelegate* delegate) {}
    void set_UploadPopupDelegate(UploadPopupDelegate* delegate) {}
    void set_UserInfoDelegate(UserInfoDelegate* delegate) {}
    void set_UserListDelegate(UserListDelegate* delegate) {}
    void set_CCDirectorDelegate(cocos2d::CCDirectorDelegate* delegate) {}
    void set_CCIMEDelegate(cocos2d::CCIMEDelegate* delegate) {}
    void set_CCKeyboardDelegate(cocos2d::CCKeyboardDelegate* delegate) {}
    void set_CCKeypadDelegate(cocos2d::CCKeypadDelegate* delegate) {}
    void set_CCMouseDelegate(cocos2d::CCMouseDelegate* delegate) {}
    void set_CCScriptEngineProtocol(cocos2d::CCScriptEngineProtocol* delegate) {}
    void set_CCTextFieldDelegate(cocos2d::CCTextFieldDelegate* delegate) {}
    void set_CCTouchDelegate(cocos2d::CCTouchDelegate* delegate) {}
    void set_EGLTouchDelegate(cocos2d::EGLTouchDelegate* delegate) {}
    void set_CCEditBoxDelegate(cocos2d::extension::CCEditBoxDelegate* delegate) {}
};
