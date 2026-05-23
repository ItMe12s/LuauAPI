-- .vscode/settings.json
-- "luau-lsp.types.definitionFiles": { "@geode": "${workspaceFolder}/types/geode.d.luau" }
-- Source of truth for each declaration is the matching binding under src/lua/bindings/*.cpp

declare class CCObject
    function getTag(self): number
    function setTag(self, tag: number)
    function retain(self)
    function release(self)
    function retainCount(self): number
end

declare class CCNode extends CCObject
    function addChild(self, child: CCNode, zOrder: number?, tag: number?)
    function removeChild(self, child: CCNode, cleanup: boolean?)
    function removeChildByTag(self, tag: number, cleanup: boolean?)
    function removeAllChildrenWithCleanup(self, cleanup: boolean)
    function removeFromParentAndCleanup(self, cleanup: boolean)
    function getChildByTag(self, tag: number): CCNode?
    function getChildByID(self, id: string): CCNode?
    function getChildrenCount(self): number
    function getParent(self): CCNode?
    function setPosition(self, x: number, y: number)
    function getPositionX(self): number
    function getPositionY(self): number
    function setPositionX(self, x: number)
    function setPositionY(self, y: number)
    function getScale(self): number
    function setScale(self, s: number)
    function getScaleX(self): number
    function setScaleX(self, s: number)
    function getScaleY(self): number
    function setScaleY(self, s: number)
    function getRotation(self): number
    function setRotation(self, deg: number)
    function getAnchorPoint(self): (number, number)
    function setAnchorPoint(self, x: number, y: number)
    function getContentSize(self): (number, number)
    function setContentSize(self, w: number, h: number)
    function isVisible(self): boolean
    function setVisible(self, v: boolean)
    function getZOrder(self): number
    function setZOrder(self, z: number)
    function getID(self): string
    function setID(self, id: string)
    function isRunning(self): boolean
    function runAction(self, action: CCAction): CCAction
    function stopAllActions(self)
    function scheduleUpdate(self)
    function unscheduleUpdate(self)
end

declare class CCSprite extends CCNode
    function setFlipX(self, v: boolean)
    function setFlipY(self, v: boolean)
    function isFlipX(self): boolean
    function isFlipY(self): boolean
    function setDisplayFrame(self, frame: CCSpriteFrame)
    function setAliasTexParameters(self)
end

declare class CCLayer extends CCNode end

declare class CCSpriteFrame extends CCObject end

declare class CCSpriteFrameCache extends CCObject
    function addSpriteFramesWithFile(self, plist: string)
    function spriteFrameByName(self, name: string): CCSpriteFrame?
end

declare class CCLabelBMFont extends CCNode
    function setString(self, text: string)
    function getString(self): string
    function setColor(self, color: RGBColor)
end

declare class CCDirector extends CCObject
    function getWinSize(self): CCSize
    function getVisibleOrigin(self): CCPoint
    function getVisibleSize(self): CCSize
end

declare class CCAction extends CCObject
    function getTag(self): number
    function setTag(self, tag: number)
    function isDone(self): boolean
end

declare class CCFiniteTimeAction extends CCAction
    function getDuration(self): number
    function setDuration(self, d: number)
end

declare class CCActionInterval extends CCFiniteTimeAction
    function getElapsed(self): number
end

declare class CCMoveTo extends CCActionInterval end
declare class CCMoveBy extends CCActionInterval end
declare class CCScaleTo extends CCActionInterval end
declare class CCFadeIn extends CCActionInterval end
declare class CCFadeOut extends CCActionInterval end
declare class CCDelayTime extends CCActionInterval end
declare class CCSequence extends CCActionInterval end
declare class CCRepeatForever extends CCActionInterval end
declare class CCActionEase extends CCActionInterval end

declare class CCEaseRateAction extends CCActionEase
    function getRate(self): number
    function setRate(self, r: number)
end

declare class CCEaseIn extends CCEaseRateAction end
declare class CCEaseOut extends CCEaseRateAction end
declare class CCEaseInOut extends CCEaseRateAction end
declare class CCEaseExponentialIn extends CCActionEase end
declare class CCEaseExponentialOut extends CCActionEase end
declare class CCEaseExponentialInOut extends CCActionEase end
declare class CCEaseSineIn extends CCActionEase end
declare class CCEaseSineOut extends CCActionEase end
declare class CCEaseSineInOut extends CCActionEase end

declare class CCEaseElastic extends CCActionEase
    function getPeriod(self): number
    function setPeriod(self, p: number)
end

declare class CCEaseElasticIn extends CCEaseElastic end
declare class CCEaseElasticOut extends CCEaseElastic end
declare class CCEaseElasticInOut extends CCEaseElastic end
declare class CCEaseBounce extends CCActionEase end
declare class CCEaseBounceIn extends CCEaseBounce end
declare class CCEaseBounceOut extends CCEaseBounce end
declare class CCEaseBounceInOut extends CCEaseBounce end
declare class CCEaseBackIn extends CCActionEase end
declare class CCEaseBackOut extends CCActionEase end
declare class CCEaseBackInOut extends CCActionEase end

declare class GameManager extends CCNode
    function getPlayerFrame(self): number
    function getPlayerColor(self): number
    function getPlayerColor2(self): number
    function getPlayerGlow(self): boolean
    function getPlayerGlowColor(self): number
    function colorForIdx(self, idx: number): RGBColor
end

declare class SimplePlayer extends CCSprite
    function updatePlayerFrame(self, id: number, type: number)
    function setColor(self, color: RGBColor)
    function setSecondColor(self, color: RGBColor)
    function setColors(self, primary: RGBColor, secondary: RGBColor)
    function updateColors(self)
    function hideSecondary(self)
    function hideAll(self)
    function setGlowOutline(self, color: RGBColor)
    function enableCustomGlowColor(self, color: RGBColor)
    function disableCustomGlowColor(self)
end

declare class OverlayManager extends CCNode end

export type RGBColor = { r: number, g: number, b: number }
export type CCSize = { width: number, height: number }
export type CCPoint = { x: number, y: number }

export type TapEvent = {
    x: number,
    y: number,
    localX: number,
    localY: number,
    targetId: number,
}

declare class TouchLayer extends CCLayer
    function addTapTarget(self, sprite: CCSprite, callback: (tap: TapEvent) -> ()): number
    function removeTapTarget(self, id: number): boolean
    function clearTapTargets(self)
    function setTouchPriority(self, priority: number)
end

export type CCSpriteFactory = {
    create: () -> CCSprite,
    createWithSpriteFrameName: (name: string) -> CCSprite,
    createWithSpriteFrame: (frame: CCSpriteFrame) -> CCSprite,
}
export type CCLayerFactory = { create: () -> CCLayer }
export type CCDirectorFactory = { sharedDirector: () -> CCDirector }
export type CCSpriteFrameCacheFactory = { sharedSpriteFrameCache: () -> CCSpriteFrameCache }
export type CCLabelBMFontFactory = { create: (text: string, font: string) -> CCLabelBMFont }

export type EaseFactory<T> = { create: (action: CCActionInterval) -> T }
export type EaseRateFactory<T> = { create: (action: CCActionInterval, rate: number) -> T }
export type EaseElasticFactory<T> = { create: (action: CCActionInterval, period: number?) -> T }

export type ActionsNamespace = {
    CCMoveTo: { create: (duration: number, x: number, y: number) -> CCMoveTo },
    CCMoveBy: { create: (duration: number, dx: number, dy: number) -> CCMoveBy },
    CCScaleTo: { create: ((duration: number, scale: number) -> CCScaleTo) & ((duration: number, sx: number, sy: number) -> CCScaleTo) },
    CCFadeIn: { create: (duration: number) -> CCFadeIn },
    CCFadeOut: { create: (duration: number) -> CCFadeOut },
    CCDelayTime: { create: (duration: number) -> CCDelayTime },
    CCSequence: { create: (actions: { CCFiniteTimeAction }) -> CCSequence },
    CCRepeatForever: { create: (action: CCActionInterval) -> CCRepeatForever },
    CCActionEase: EaseFactory<CCActionEase>,
    CCEaseRateAction: EaseRateFactory<CCEaseRateAction>,
    CCEaseIn: EaseRateFactory<CCEaseIn>,
    CCEaseOut: EaseRateFactory<CCEaseOut>,
    CCEaseInOut: EaseRateFactory<CCEaseInOut>,
    CCEaseExponentialIn: EaseFactory<CCEaseExponentialIn>,
    CCEaseExponentialOut: EaseFactory<CCEaseExponentialOut>,
    CCEaseExponentialInOut: EaseFactory<CCEaseExponentialInOut>,
    CCEaseSineIn: EaseFactory<CCEaseSineIn>,
    CCEaseSineOut: EaseFactory<CCEaseSineOut>,
    CCEaseSineInOut: EaseFactory<CCEaseSineInOut>,
    CCEaseElastic: EaseElasticFactory<CCEaseElastic>,
    CCEaseElasticIn: EaseElasticFactory<CCEaseElasticIn>,
    CCEaseElasticOut: EaseElasticFactory<CCEaseElasticOut>,
    CCEaseElasticInOut: EaseElasticFactory<CCEaseElasticInOut>,
    CCEaseBounce: EaseFactory<CCEaseBounce>,
    CCEaseBounceIn: EaseFactory<CCEaseBounceIn>,
    CCEaseBounceOut: EaseFactory<CCEaseBounceOut>,
    CCEaseBounceInOut: EaseFactory<CCEaseBounceInOut>,
    CCEaseBackIn: EaseFactory<CCEaseBackIn>,
    CCEaseBackOut: EaseFactory<CCEaseBackOut>,
    CCEaseBackInOut: EaseFactory<CCEaseBackInOut>,
}

export type SchedulerNamespace = {
    schedule: (callback: () -> (), interval: number) -> number,
    unschedule: (id: number) -> boolean,
}

export type Cocos2dNamespace = {
    CCDirector: CCDirectorFactory,
    CCSprite: CCSpriteFactory,
    CCLayer: CCLayerFactory,
    CCSpriteFrame: {},
    CCSpriteFrameCache: CCSpriteFrameCacheFactory,
    CCLabelBMFont: CCLabelBMFontFactory,
    scheduler: SchedulerNamespace,
    actions: ActionsNamespace,
}

export type IconTypeEnum = {
    Cube: number,
    Ship: number,
    Ball: number,
    Ufo: number,
    Wave: number,
    Robot: number,
    Spider: number,
    Swing: number,
    Jetpack: number,
    DeathEffect: number,
    Special: number,
    Item: number,
    ShipFire: number,
}

export type GeodeNamespace = {
    expandSpriteName: (name: string) -> string,
    modResourcesPath: () -> string,
    GameManager: { get: () -> GameManager },
    SimplePlayer: { create: (iconId: number) -> SimplePlayer },
    IconType: IconTypeEnum,
    OverlayManager: { get: () -> OverlayManager },
    TouchLayer: { create: () -> TouchLayer },
    cocos2d: Cocos2dNamespace,
}

declare geode: GeodeNamespace
