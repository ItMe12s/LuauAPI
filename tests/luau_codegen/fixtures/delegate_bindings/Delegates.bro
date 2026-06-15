// Offline CI fixture, minimal delegate bindings snapshot
class AnimatedSpriteDelegate {
    virtual void animationFinished(char const* key) = inline;
};

class BoomScrollLayerDelegate {
    virtual void scrollLayerScrollingStarted(BoomScrollLayer* layer) = inline;
    virtual void scrollLayerScrolledToPage(BoomScrollLayer* layer, int page) = inline;
    virtual void scrollLayerMoved(cocos2d::CCPoint position) = inline;
    virtual void scrollLayerWillScrollToPage(BoomScrollLayer* layer, int page) = inline;
};

class CCCircleWaveDelegate {
    virtual void circleWaveWillBeRemoved(CCCircleWave* circleWave) = inline;
};

class CCScrollLayerExtDelegate {
    virtual void scrllViewWillBeginDecelerating(CCScrollLayerExt* layer) = inline;
    virtual void scrollViewDidEndDecelerating(CCScrollLayerExt* layer) = inline;
    virtual void scrollViewTouchMoving(CCScrollLayerExt* layer) = inline;
    virtual void scrollViewDidEndMoving(CCScrollLayerExt* layer) = inline;
    virtual void scrollViewTouchBegin(CCScrollLayerExt* layer) = inline;
    virtual void scrollViewTouchEnd(CCScrollLayerExt* layer) = inline;
};

class CharacterColorDelegate {
    virtual void playerColorChanged() = inline;
    virtual void showUnlockPopup(int id, UnlockType type) = inline;
};

class ColorSelectDelegate {
    virtual void colorSelectClosed(cocos2d::CCNode* popup) = inline;
};

class ColorSetupDelegate {
    virtual void colorSetupClosed(int id) = inline;
};

class CommentUploadDelegate {
    virtual void commentUploadFinished(int parentID) = inline;
    virtual void commentUploadFailed(int parentID, CommentError errorType) = inline;
    virtual void commentDeleteFailed(int id, int parentID) = inline;
};

class ConfigureValuePopupDelegate {
    virtual void valuePopupClosed(ConfigureValuePopup* popup, float value) = inline;
};

class CurrencyRewardDelegate {
    virtual void currencyWillExit(CurrencyRewardLayer* layer) = inline;
};

class CustomSFXDelegate {
    virtual void sfxObjectSelected(SFXInfoObject* object) = inline;
    virtual int getActiveSFXID() = inline;
    virtual bool overridePlaySFX(SFXInfoObject* object) = inline;
};

class CustomSongDelegate {
    virtual void songIDChanged(int id) = inline;
    virtual int getActiveSongID() = inline;
    virtual gd::string getSongFileName() = inline;
    virtual LevelSettingsObject* getLevelSettings() = inline;
};

class CustomSongLayerDelegate {
    virtual void customSongLayerClosed() = inline;
};

class DemonFilterDelegate {
    virtual void demonFilterSelectClosed(int filter) = inline;
};

class DialogDelegate {
    virtual void dialogClosed(DialogLayer* layer) = inline;
};

class DownloadMessageDelegate {
    virtual void downloadMessageFinished(GJUserMessage* message) = inline;
    virtual void downloadMessageFailed(int id) = inline;
};

class DynamicScrollDelegate {
    virtual void updatePageWithObject(cocos2d::CCObject* layer, cocos2d::CCObject* object) = inline;
};

class FLAlertLayerProtocol {
    virtual void FLAlert_Clicked(FLAlertLayer* layer, bool btn2) = inline;
};

class FriendRequestDelegate {
    virtual void loadFRequestsFinished(cocos2d::CCArray* scores, char const* key) = inline;
    virtual void setupPageInfo(gd::string info, char const* key) = inline;
    virtual void forceReloadRequests(bool sent) = inline;
};

class GJAccountBackupDelegate {
    virtual void backupAccountFinished() = inline;
    virtual void backupAccountFailed(BackupAccountError errorType, int response) = inline;
};

class GJAccountDelegate {
    virtual void accountStatusChanged() = inline;
};

class GJAccountLoginDelegate {
    virtual void loginAccountFinished(int accountID, int userID) = inline;
    virtual void loginAccountFailed(AccountError errorType) = inline;
};

class GJAccountRegisterDelegate {
    virtual void registerAccountFinished() = inline;
    virtual void registerAccountFailed(AccountError errorType) = inline;
};

class GJAccountSettingsDelegate {
    virtual void updateSettingsFinished() = inline;
    virtual void updateSettingsFailed() = inline;
};

class GJAccountSyncDelegate {
    virtual void syncAccountFinished() = inline;
    virtual void syncAccountFailed(BackupAccountError errorType, int response) = inline;
};

class GJChallengeDelegate {
    virtual void challengeStatusFinished() = inline;
    virtual void challengeStatusFailed() = inline;
};

class GJDailyLevelDelegate {
    virtual void dailyStatusFinished(GJTimedLevelType type) = inline;
};

class GJDropDownLayerDelegate {
    virtual void dropDownLayerWillClose(GJDropDownLayer* layer) = inline;
};

class GJMPDelegate {
    virtual void joinLobbyFinished(int id) = inline;
    virtual void didUploadMPComment(int id) = inline;
    virtual void updateComments() = inline;
};

class GJOnlineRewardDelegate {
    virtual void onlineRewardStatusFinished(gd::string key) = inline;
    virtual void onlineRewardStatusFailed() = inline;
};

class GJPurchaseDelegate {
    virtual void didPurchaseItem(GJStoreItem* item) = inline;
};

class GJRewardDelegate {
    virtual void rewardsStatusFinished(int type) = inline;
    virtual void rewardsStatusFailed() = inline;
};

class GJRotationControlDelegate {
    virtual void angleChanged(float angle) = inline;
    virtual void angleChangeBegin() = inline;
    virtual void angleChangeEnded() = inline;
};

class GJScaleControlDelegate {
    virtual void scaleXChanged(float scaleX, bool lock) = inline;
    virtual void scaleYChanged(float scaleY, bool lock) = inline;
    virtual void scaleXYChanged(float scaleX, float scaleY, bool lock) = inline;
    virtual void scaleChangeBegin() = inline;
    virtual void scaleChangeEnded() = inline;
    virtual void updateScaleControl() = inline;
    virtual void anchorPointMoved(cocos2d::CCPoint newAnchor) = inline;
};

class GJSpecialColorSelectDelegate {
    virtual void colorSelectClosed(GJSpecialColorSelect* select, int id) = inline;
};

class GJTransformControlDelegate {
    virtual void transformScaleXChanged(float scaleX) = inline;
    virtual void transformScaleYChanged(float scaleY) = inline;
    virtual void transformScaleXYChanged(float scaleX, float scaleY) = inline;
    virtual void transformRotationXChanged(float rotationX) = inline;
    virtual void transformRotationYChanged(float rotationY) = inline;
    virtual void transformRotationChanged(float rotation) = inline;
    virtual void transformResetRotation() = inline;
    virtual void transformRestoreRotation() = inline;
    virtual void transformSkewXChanged(float skewX) = inline;
    virtual void transformSkewYChanged(float skewY) = inline;
    virtual void transformChangeBegin() = inline;
    virtual void transformChangeEnded() = inline;
    virtual void updateTransformControl() = inline;
    virtual void anchorPointMoved(cocos2d::CCPoint anchorPoint) = inline;
    virtual cocos2d::CCNode* getTransformNode() = inline;
    virtual EditorUI* getUI() = inline;
};

class GameRateDelegate {
    virtual void updateRate() = inline;
};

class GooglePlayDelegate {
    virtual void googlePlaySignedIn() = inline;
};

class HSVWidgetDelegate {
    virtual void hsvPopupClosed(HSVWidgetPopup* popup, cocos2d::ccHSVValue value) = inline;
    virtual void hsvChanged(ConfigureHSVWidget* widget) = inline;
};

class LeaderboardManagerDelegate {
    virtual void updateUserScoreFinished() = inline;
    virtual void updateUserScoreFailed() = inline;
    virtual void loadLeaderboardFinished(cocos2d::CCArray* scores, char const* key) = inline;
    virtual void loadLeaderboardFailed(char const* key) = inline;
};

class LevelCommentDelegate {
    virtual void loadCommentsFinished(cocos2d::CCArray* comments, char const* key) = inline;
    virtual void loadCommentsFailed(char const* key) = inline;
    virtual void updateUserScoreFinished() = inline;
    virtual void setupPageInfo(gd::string info, char const* key) = inline;
};

class LevelDeleteDelegate {
    virtual void levelDeleteFinished(int id) = inline;
    virtual void levelDeleteFailed(int id) = inline;
};

class LevelDownloadDelegate {
    virtual void levelDownloadFinished(GJGameLevel* level) = inline;
    virtual void levelDownloadFailed(int response) = inline;
};

class LevelListDeleteDelegate {
    virtual void levelListDeleteFinished(int id) = inline;
    virtual void levelListDeleteFailed(int id) = inline;
};

class LevelManagerDelegate {
    virtual void loadLevelsFinished(cocos2d::CCArray* levels, char const* key) = inline;
    virtual void loadLevelsFailed(char const* key) = inline;
    virtual void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int type) = inline;
    virtual void loadLevelsFailed(char const* key, int type) = inline;
    virtual void setupPageInfo(gd::string info, char const* key) = inline;
};

class LevelRateInfoDelegate {
    virtual void rateInfoFinished(int id, gd::string info) = inline;
    virtual void rateInfoFailed(int id, int response) = inline;
};

class LevelSettingsDelegate {
    virtual void levelSettingsUpdated() = inline;
};

class LevelUpdateDelegate {
    virtual void levelUpdateFailed(int response) = inline;
};

class LevelUploadDelegate {
    virtual void levelUploadFinished(GJGameLevel* level) = inline;
    virtual void levelUploadFailed(GJGameLevel* level) = inline;
};

class LikeItemDelegate {
    virtual void likedItem(LikeItemType type, int id, bool liked) = inline;
};

class ListButtonBarDelegate {
    virtual void listButtonBarSwitchedPage(ListButtonBar* bar, int page) = inline;
};

class ListUploadDelegate {
    virtual void listUploadFinished(GJLevelList* list) = inline;
    virtual void listUploadFailed(GJLevelList* list, int response) = inline;
};

class MessageListDelegate {
    virtual void loadMessagesFinished(cocos2d::CCArray* messages, char const* key) = inline;
    virtual void forceReloadMessages(bool sent) = inline;
    virtual void setupPageInfo(gd::string info, char const* key) = inline;
};

class MusicBrowserDelegate {
    virtual void musicBrowserClosed(MusicBrowser* browser) = inline;
};

class MusicDownloadDelegate {
    virtual void loadSongInfoFinished(SongInfoObject* object) = inline;
    virtual void loadSongInfoFailed(int id, GJSongError errorType) = inline;
    virtual void downloadSongStarted(int id) = inline;
    virtual void downloadSongFinished(int id) = inline;
    virtual void downloadSongFailed(int id, GJSongError errorType) = inline;
    virtual void songStateChanged() = inline;
    virtual void downloadSFXFinished(int id) = inline;
    virtual void downloadSFXFailed(int id, GJSongError errorType) = inline;
    virtual void musicActionFinished(GJMusicAction action) = inline;
    virtual void musicActionFailed(GJMusicAction action) = inline;
};

class NumberInputDelegate {
    virtual void numberInputClosed(NumberInputLayer* layer) = inline;
};

class OnlineListDelegate {
    virtual void loadListFinished(cocos2d::CCArray* objects, char const* key) = inline;
    virtual void loadListFailed(char const* key) = inline;
    virtual void setupPageInfo(gd::string info, char const* key) = inline;
};

class OptionsObjectDelegate {
    virtual void stateChanged(OptionsObject* object) = inline;
};

class RateLevelDelegate {
    virtual void rateLevelClosed() = inline;
};

class RewardedVideoDelegate {
    virtual void rewardedVideoFinished() = inline;
    virtual bool shouldOffsetRewardCurrency() = inline;
};

class SFXBrowserDelegate {
    virtual void sfxBrowserClosed(SFXBrowser* browser) = inline;
};

class SelectArtDelegate {
    virtual void selectArtClosed(SelectArtLayer* layer) = inline;
};

class SelectListIconDelegate {
    virtual void iconSelectClosed(SelectListIconLayer* layer) = inline;
};

class SelectPremadeDelegate {
    virtual void selectPremadeClosed(SelectPremadeLayer* layer, int type) = inline;
};

class SelectSFXSortDelegate {
    virtual void sortSelectClosed(SelectSFXSortLayer* layer) = inline;
};

class SelectSettingDelegate {
    virtual void selectSettingClosed(SelectSettingLayer* layer) = inline;
};

class SetIDPopupDelegate {
    virtual void setIDPopupClosed(SetIDPopup* popup, int value) = inline;
};

class SetTextPopupDelegate {
    virtual void setTextPopupClosed(SetTextPopup* popup, gd::string text) = inline;
};

class ShareCommentDelegate {
    virtual void shareCommentClosed(gd::string text, ShareCommentLayer* layer) = inline;
};

class SliderDelegate {
    virtual void sliderBegan(Slider* slider) = inline;
    virtual void sliderEnded(Slider* slider) = inline;
};

class SongPlaybackDelegate {
    virtual void onPlayback(SongInfoObject* object) = inline;
};

class SpritePartDelegate {
    virtual void displayFrameChanged(cocos2d::CCObject* sprite, gd::string frameName) = inline;
};

class TableViewCellDelegate {
    virtual bool cellPerformedAction(TableViewCell* cell, int listType, CellAction action, cocos2d::CCNode* parent) = inline;
    virtual int getSelectedCellIdx() = inline;
    virtual bool shouldSnapToSelected() = inline;
    virtual int getCellDelegateType() = inline;
};

class TableViewDelegate {
    virtual void willTweenToIndexPath(CCIndexPath& indexPath, TableViewCell* cell, TableView* tableView) = inline;
    virtual void didEndTweenToIndexPath(CCIndexPath& indexPath, TableView* tableView) = inline;
    virtual void TableViewWillDisplayCellForRowAtIndexPath(CCIndexPath& indexPath, TableViewCell* cell, TableView* tableView) = inline;
    virtual void TableViewDidDisplayCellForRowAtIndexPath(CCIndexPath& indexPath, TableViewCell* cell, TableView* tableView) = inline;
    virtual void TableViewWillReloadCellForRowAtIndexPath(CCIndexPath& indexPath, TableViewCell* cell, TableView* tableView) = inline;
    virtual float cellHeightForRowAtIndexPath(CCIndexPath& indexPath, TableView* tableView) = inline;
    virtual void didSelectRowAtIndexPath(CCIndexPath& indexPath, TableView* tableView) = inline;
};

class TextAreaDelegate {
    virtual void fadeInTextFinished(TextArea* textArea) = inline;
};

class TextInputDelegate {
    virtual void textChanged(CCTextInputNode* node) = inline;
    virtual void textInputOpened(CCTextInputNode* node) = inline;
    virtual void textInputClosed(CCTextInputNode* node) = inline;
    virtual void textInputShouldOffset(CCTextInputNode* node, float yOffset) = inline;
    virtual void textInputReturn(CCTextInputNode* node) = inline;
    virtual bool allowTextInput(CCTextInputNode* node) = inline;
    virtual void enterPressed(CCTextInputNode* node) = inline;
};

class TriggerEffectDelegate {
    virtual bool checkSpawnAbuse() = inline;
};

class UploadActionDelegate {
    virtual void uploadActionFinished(int id, int response) = inline;
    virtual void uploadActionFailed(int id, int response) = inline;
};

class UploadMessageDelegate {
    virtual void uploadMessageFinished(int accountID) = inline;
    virtual void uploadMessageFailed(int accountID) = inline;
};

class UploadPopupDelegate {
    virtual void onClosePopup(UploadActionPopup* popup) = inline;
};

class UserInfoDelegate {
    virtual void getUserInfoFinished(GJUserScore* score) = inline;
    virtual void getUserInfoFailed(int id) = inline;
    virtual void userInfoChanged(GJUserScore* score) = inline;
};

class UserListDelegate {
    virtual void getUserListFinished(cocos2d::CCArray* scores, UserListType type) = inline;
    virtual void userListChanged(cocos2d::CCArray* scores, UserListType type) = inline;
    virtual void forceReloadList(UserListType type) = inline;
};

class CCIMEDelegate {
    virtual bool attachWithIME() = inline;
    virtual bool detachWithIME() = inline;
};

class CCTouchDelegate {
    virtual void setPreviousPriority(int arg) = inline;
    virtual int getPreviousPriority() = inline;
};

class DelegatePtrHost {
    void set_AnimatedSpriteDelegate(AnimatedSpriteDelegate* delegate);
    void set_AppDelegate(AppDelegate* delegate);
    void set_BoomScrollLayerDelegate(BoomScrollLayerDelegate* delegate);
    void set_CCCircleWaveDelegate(CCCircleWaveDelegate* delegate);
    void set_CCScrollLayerExtDelegate(CCScrollLayerExtDelegate* delegate);
    void set_CharacterColorDelegate(CharacterColorDelegate* delegate);
    void set_ColorSelectDelegate(ColorSelectDelegate* delegate);
    void set_ColorSetupDelegate(ColorSetupDelegate* delegate);
    void set_CommentUploadDelegate(CommentUploadDelegate* delegate);
    void set_ConfigureValuePopupDelegate(ConfigureValuePopupDelegate* delegate);
    void set_CurrencyRewardDelegate(CurrencyRewardDelegate* delegate);
    void set_CustomSFXDelegate(CustomSFXDelegate* delegate);
    void set_CustomSongDelegate(CustomSongDelegate* delegate);
    void set_CustomSongLayerDelegate(CustomSongLayerDelegate* delegate);
    void set_DemonFilterDelegate(DemonFilterDelegate* delegate);
    void set_DialogDelegate(DialogDelegate* delegate);
    void set_DownloadMessageDelegate(DownloadMessageDelegate* delegate);
    void set_DynamicScrollDelegate(DynamicScrollDelegate* delegate);
    void set_FLAlertLayerProtocol(FLAlertLayerProtocol* delegate);
    void set_FriendRequestDelegate(FriendRequestDelegate* delegate);
    void set_GJAccountBackupDelegate(GJAccountBackupDelegate* delegate);
    void set_GJAccountDelegate(GJAccountDelegate* delegate);
    void set_GJAccountLoginDelegate(GJAccountLoginDelegate* delegate);
    void set_GJAccountRegisterDelegate(GJAccountRegisterDelegate* delegate);
    void set_GJAccountSettingsDelegate(GJAccountSettingsDelegate* delegate);
    void set_GJAccountSyncDelegate(GJAccountSyncDelegate* delegate);
    void set_GJChallengeDelegate(GJChallengeDelegate* delegate);
    void set_GJDailyLevelDelegate(GJDailyLevelDelegate* delegate);
    void set_GJDropDownLayerDelegate(GJDropDownLayerDelegate* delegate);
    void set_GJMPDelegate(GJMPDelegate* delegate);
    void set_GJOnlineRewardDelegate(GJOnlineRewardDelegate* delegate);
    void set_GJPurchaseDelegate(GJPurchaseDelegate* delegate);
    void set_GJRewardDelegate(GJRewardDelegate* delegate);
    void set_GJRotationControlDelegate(GJRotationControlDelegate* delegate);
    void set_GJScaleControlDelegate(GJScaleControlDelegate* delegate);
    void set_GJSpecialColorSelectDelegate(GJSpecialColorSelectDelegate* delegate);
    void set_GJTransformControlDelegate(GJTransformControlDelegate* delegate);
    void set_GameRateDelegate(GameRateDelegate* delegate);
    void set_GooglePlayDelegate(GooglePlayDelegate* delegate);
    void set_HSVWidgetDelegate(HSVWidgetDelegate* delegate);
    void set_LeaderboardManagerDelegate(LeaderboardManagerDelegate* delegate);
    void set_LevelCommentDelegate(LevelCommentDelegate* delegate);
    void set_LevelDeleteDelegate(LevelDeleteDelegate* delegate);
    void set_LevelDownloadDelegate(LevelDownloadDelegate* delegate);
    void set_LevelListDeleteDelegate(LevelListDeleteDelegate* delegate);
    void set_LevelManagerDelegate(LevelManagerDelegate* delegate);
    void set_LevelRateInfoDelegate(LevelRateInfoDelegate* delegate);
    void set_LevelSettingsDelegate(LevelSettingsDelegate* delegate);
    void set_LevelUpdateDelegate(LevelUpdateDelegate* delegate);
    void set_LevelUploadDelegate(LevelUploadDelegate* delegate);
    void set_LikeItemDelegate(LikeItemDelegate* delegate);
    void set_ListButtonBarDelegate(ListButtonBarDelegate* delegate);
    void set_ListUploadDelegate(ListUploadDelegate* delegate);
    void set_MessageListDelegate(MessageListDelegate* delegate);
    void set_MusicBrowserDelegate(MusicBrowserDelegate* delegate);
    void set_MusicDownloadDelegate(MusicDownloadDelegate* delegate);
    void set_NumberInputDelegate(NumberInputDelegate* delegate);
    void set_OnlineListDelegate(OnlineListDelegate* delegate);
    void set_OptionsObjectDelegate(OptionsObjectDelegate* delegate);
    void set_RateLevelDelegate(RateLevelDelegate* delegate);
    void set_RewardedVideoDelegate(RewardedVideoDelegate* delegate);
    void set_SFXBrowserDelegate(SFXBrowserDelegate* delegate);
    void set_SelectArtDelegate(SelectArtDelegate* delegate);
    void set_SelectListIconDelegate(SelectListIconDelegate* delegate);
    void set_SelectPremadeDelegate(SelectPremadeDelegate* delegate);
    void set_SelectSFXSortDelegate(SelectSFXSortDelegate* delegate);
    void set_SelectSettingDelegate(SelectSettingDelegate* delegate);
    void set_SetIDPopupDelegate(SetIDPopupDelegate* delegate);
    void set_SetTextPopupDelegate(SetTextPopupDelegate* delegate);
    void set_ShareCommentDelegate(ShareCommentDelegate* delegate);
    void set_SliderDelegate(SliderDelegate* delegate);
    void set_SongPlaybackDelegate(SongPlaybackDelegate* delegate);
    void set_SpritePartDelegate(SpritePartDelegate* delegate);
    void set_TableViewCellDelegate(TableViewCellDelegate* delegate);
    void set_TableViewDelegate(TableViewDelegate* delegate);
    void set_TextAreaDelegate(TextAreaDelegate* delegate);
    void set_TextInputDelegate(TextInputDelegate* delegate);
    void set_TriggerEffectDelegate(TriggerEffectDelegate* delegate);
    void set_UploadActionDelegate(UploadActionDelegate* delegate);
    void set_UploadMessageDelegate(UploadMessageDelegate* delegate);
    void set_UploadPopupDelegate(UploadPopupDelegate* delegate);
    void set_UserInfoDelegate(UserInfoDelegate* delegate);
    void set_UserListDelegate(UserListDelegate* delegate);
    void set_CCDirectorDelegate(cocos2d::CCDirectorDelegate* delegate);
    void set_CCIMEDelegate(cocos2d::CCIMEDelegate* delegate);
    void set_CCKeyboardDelegate(cocos2d::CCKeyboardDelegate* delegate);
    void set_CCKeypadDelegate(cocos2d::CCKeypadDelegate* delegate);
    void set_CCMouseDelegate(cocos2d::CCMouseDelegate* delegate);
    void set_CCScriptEngineProtocol(cocos2d::CCScriptEngineProtocol* delegate);
    void set_CCTextFieldDelegate(cocos2d::CCTextFieldDelegate* delegate);
    void set_CCTouchDelegate(cocos2d::CCTouchDelegate* delegate);
    void set_EGLTouchDelegate(cocos2d::EGLTouchDelegate* delegate);
    void set_CCEditBoxDelegate(cocos2d::extension::CCEditBoxDelegate* delegate);
};
