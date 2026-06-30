local LuauAPILoadingLayer = {}

local cc2d = geode.cocos2d
local modId = geode.Mod.getID()
local LOGO = geode.Mod.getResourcesDir() .. "/loadingLogo.png"
local GRADIENT_ID = modId .. "/loading-gradient"
local LOGO_ID = modId .. "/loading-logo"

local function screenCenter(layer: LoadingLayer): (CCPoint?, CCSize?)
    local director = cc2d.CCDirector.sharedDirector()
    if not director then return nil end
    local origin = director:getVisibleOrigin()
    local visible = director:getVisibleSize()
    return layer:convertToNodeSpace({
        x = origin.x + visible.width / 2,
        y = origin.y + visible.height / 2,
    }), visible
end

local function hideVanilla(layer: LoadingLayer)
    local bg = layer:getChildByID("bg-texture")
    if bg then bg:setVisible(false) end

    local children = layer:getChildren()
    if not children then return end
    for i = 0, children:count() - 1 do
        local child = children:objectAtIndex(i) :: CCNode?
        local id = child and child:getID()
        if child and id ~= "fmod-logo" and id ~= "cocos2d-logo" and id ~= GRADIENT_ID then
            child:setPositionX(child:getPositionX() + 80085)
        end
    end
end

local function addGradient(layer: LoadingLayer, center: CCPoint, visible: CCSize)
    if layer:getChildByID(GRADIENT_ID) then return end
    local gradient = cc2d.CCLayerGradient.create(
        { r = 5, g = 117, b = 230, a = 255 },
        { r = 2, g = 27, b = 121, a = 255 },
        { x = 0, y = -1 }
    )
    if not gradient then return end
    gradient:setID(GRADIENT_ID)
    gradient:setContentSize({ width = visible.width, height = visible.height })
    gradient:ignoreAnchorPointForPosition(false)
    gradient:setAnchorPoint({ x = 0.5, y = 0.5 })
    gradient:setPosition(center)
    gradient:setZOrder(-1)
    layer:addChild(gradient)
end

local function addLogo(layer: LoadingLayer, center: CCPoint, visible: CCSize)
    if layer:getChildByID(LOGO_ID) then return end
    local texCache = cc2d.CCTextureCache.sharedTextureCache()
    if not texCache then return end
    local tex = texCache:addImage(LOGO, false)
    if not tex then return end

    local size = tex:getContentSize()
    local logo = cc2d.CCSprite.createWithTexture(tex, {
        x = 0,
        y = 0,
        width = size.width,
        height = size.height,
    } :: any)
    if not logo then return end

    local content = logo:getContentSize()
    logo:setID(LOGO_ID)
    logo:setAnchorPoint({ x = 0.5, y = 0.5 })
    logo:setPosition(center)
    logo:setScale(math.min(visible.width * 0.4 / content.width, visible.height * 0.4 / content.height))
    layer:addChild(logo)
end

function LuauAPILoadingLayer.init()
    if not geode.Mod.getSettingValue("enable-custom-loading-layer") then return end

    geode.hook("geode.gd.LoadingLayer:init/1", {
        after = function(self: LoadingLayer, _: boolean, result: boolean)
            hideVanilla(self)
            local center, visible = screenCenter(self)
            if not center or not visible then return result end
            addGradient(self, center, visible)
            addLogo(self, center, visible)
            return result
        end,
    })
end

return LuauAPILoadingLayer
