type RGB = { r: number, g: number, b: number }

local LuauAPILoadingLayer = {}

local cc2d = geode.cocos2d
local cc = geode.cocos
local modId = geode.Mod.getID()
local ID_PREFIX = modId .. "/loading-"

local POLL_INTERVAL = 0.01
local FADE_DURATION = 1.2
local FADE_EASE_RATE = 2

local GEODE_STEPS = 3
local GD_STEPS = 14

local BAR_W = 240
local BAR_H = 10
local BAR_PAD = 1.5
local BAR_STROKE = 0.333
local INNER_W = BAR_W - BAR_PAD * 2
local INNER_H = BAR_H - BAR_PAD * 2
local BAR_Y_FRAC = 0.25
local STATUS_Y_OFFSET = 6
local STATUS_SCALE = 0.45
local LOGO_MAX_FRAC = 0.5
local LOGO_Y_FRAC = 0.025
local HIDE_OFFSET = 80085
local OVERLAY_Z = 9999

local Z_GRADIENT = -1
local Z_LOGO = 1
local Z_BAR = 2
local Z_FILL = 3

local LOGO_PATH = geode.Mod.getResourcesDir() .. "/loadingLogo.png"
local STATUS_FONT = "chatFont.fnt"
local OPAQUE = 255

local OVERLAY_ID = ID_PREFIX .. "overlay"
local GRADIENT_ID = ID_PREFIX .. "gradient"
local LOGO_ID = ID_PREFIX .. "logo"
local BAR_FILL_ID = ID_PREFIX .. "bar-fill"
local BAR_STATUS_ID = ID_PREFIX .. "bar-status"
local BAR_EDGE_BOTTOM_ID = ID_PREFIX .. "bar-edge-bottom"
local BAR_EDGE_TOP_ID = ID_PREFIX .. "bar-edge-top"
local BAR_EDGE_LEFT_ID = ID_PREFIX .. "bar-edge-left"
local BAR_EDGE_RIGHT_ID = ID_PREFIX .. "bar-edge-right"

local HIDDEN_GEODE_LABEL_IDS = { "geode-small-label", "geode-small-label-2" }

local menuOverlayShown = false
local lastProgress = 0

local function hex(value: string): RGB
    local color, err = cc.cc3bFromHexString(value)
    assert(color, err)
    return color
end

local COLORS = {
    bgTop = hex("286ABF"),
    bgBot = hex("213091"),
    barFill = hex("FFFFFF"),
}

local function modStats(): (number, number, number, string?)
    local loaded, shouldLoad, total = 0, 0, 0
    local current = nil :: string?
    for _, mod in geode.Loader.getAllMods() do
        total += 1
        if mod.shouldLoad then shouldLoad += 1 end
        if mod.enabled then
            loaded += 1
        elseif mod.shouldLoad and not current then
            current = mod.name
        end
    end
    return loaded, shouldLoad, total, current
end

local function getProgress(layer: LoadingLayer, loaded: number, enabled: number): number
    local total = math.max(1, GEODE_STEPS + GD_STEPS + enabled)
    local geodeStep = if layer.m_loadStep > 0 then GEODE_STEPS elseif enabled > 0 and loaded >= enabled then 1 else 0
    return math.clamp((geodeStep + layer.m_loadStep + loaded) / total, 0, 1)
end

local function progressLabel(pct: number): string
    return ("%d%%"):format(math.floor(pct * 100))
end

local function statusText(
    layer: LoadingLayer,
    loaded: number,
    enabled: number,
    total: number,
    current: string?
): string
    local pct = getProgress(layer, loaded, enabled)
    local lines = { progressLabel(pct) }
    local seen = { [lines[1]] = true }
    local function push(text: string?)
        if not text or text == "" or seen[text] then return end
        seen[text] = true
        table.insert(lines, text)
    end

    if loaded < total then
        push(("Geode: Loaded %d/%d mods"):format(loaded, total))
        push(current)
    end
    push(layer:getLoadingString())
    return table.concat(lines, "\n")
end

local function layout(node: CCNode): (CCPoint?, CCSize?)
    local director = cc2d.CCDirector.sharedDirector()
    if not director then return nil, nil end
    local origin = director:getVisibleOrigin()
    local visible = director:getVisibleSize()
    local world = { x = origin.x + visible.width / 2, y = origin.y + visible.height / 2 }
    return node:convertToNodeSpace(world), visible
end

local function hideVanilla(layer: LoadingLayer)
    local children = layer:getChildren()
    if not children then return end
    for i = 0, children:count() - 1 do
        local child = children:objectAtIndex(i) :: CCNode?
        if not child then continue end
        local id = child:getID()
        if table.find(HIDDEN_GEODE_LABEL_IDS, id) then
            child:setVisible(false)
        else
            local pos = child:getPosition()
            child:setPosition({ x = pos.x + HIDE_OFFSET, y = pos.y })
        end
    end
end

local function addGradient(parent: CCNode, center: CCPoint, visible: CCSize)
    if parent:getChildByID(GRADIENT_ID) then return end
    local gradient = cc2d.CCLayerGradient.create(
        cc.to4B(COLORS.bgTop, OPAQUE),
        cc.to4B(COLORS.bgBot, OPAQUE),
        { x = 0, y = -1 }
    )
    if not gradient then return end
    gradient:setID(GRADIENT_ID)
    gradient:setContentSize(visible)
    gradient:ignoreAnchorPointForPosition(false)
    gradient:setAnchorPoint({ x = 0.5, y = 0.5 })
    gradient:setPosition(center)
    gradient:setZOrder(Z_GRADIENT)
    parent:addChild(gradient)
end

local function addLogo(parent: CCNode, center: CCPoint, visible: CCSize)
    if parent:getChildByID(LOGO_ID) then return end
    local texCache = cc2d.CCTextureCache.sharedTextureCache()
    if not texCache then return end
    local tex = texCache:addImage(LOGO_PATH, false)
    if not tex then return end
    local texSize = tex:getContentSize()
    local logo = cc2d.CCSprite.createWithTexture(tex, { x = 0, y = 0, width = texSize.width, height = texSize.height } :: any)
    if not logo then return end

    local scale = math.min(visible.width * LOGO_MAX_FRAC / texSize.width, visible.height * LOGO_MAX_FRAC / texSize.height)
    logo:setScale(scale)
    logo:setID(LOGO_ID)
    logo:setAnchorPoint({ x = 0.5, y = 0.5 })
    logo:setPosition({ x = center.x, y = center.y + visible.height * LOGO_Y_FRAC })
    logo:setZOrder(Z_LOGO)
    parent:addChild(logo)
end

local function addProgressBar(parent: CCNode, center: CCPoint, visible: CCSize)
    if parent:getChildByID(BAR_FILL_ID) then return end
    local barX = center.x - BAR_W / 2
    local barY = center.y - visible.height * BAR_Y_FRAC
    local x0, y0 = barX, barY - BAR_H / 2
    local x1, y1 = barX + BAR_W, barY + BAR_H / 2
    local stroke = BAR_STROKE * 2
    local white = cc.to4B(COLORS.barFill, OPAQUE)

    local function addEdge(id: string, x: number, y: number, w: number, h: number)
        local edge = cc2d.CCLayerColor.create(white, w, h)
        if not edge then return end
        edge:setID(id)
        edge:ignoreAnchorPointForPosition(false)
        edge:setAnchorPoint({ x = 0, y = 0 })
        edge:setPosition({ x = x, y = y })
        edge:setZOrder(Z_BAR)
        parent:addChild(edge)
    end

    addEdge(BAR_EDGE_BOTTOM_ID, x0, y0, BAR_W, stroke)
    addEdge(BAR_EDGE_TOP_ID, x0, y1 - stroke, BAR_W, stroke)
    addEdge(BAR_EDGE_LEFT_ID, x0, y0, stroke, BAR_H)
    addEdge(BAR_EDGE_RIGHT_ID, x1 - stroke, y0, stroke, BAR_H)

    local fill = cc2d.CCLayerColor.create(white, 1, INNER_H)
    if fill then
        fill:setID(BAR_FILL_ID)
        fill:ignoreAnchorPointForPosition(false)
        fill:setAnchorPoint({ x = 0, y = 0.5 })
        fill:setPosition({ x = barX + BAR_PAD, y = barY })
        fill:setZOrder(Z_FILL)
        parent:addChild(fill)
    end

    local status = cc2d.CCLabelBMFont.create("", STATUS_FONT)
    if status then
        status:setID(BAR_STATUS_ID)
        status:setScale(STATUS_SCALE)
        status:setColor(COLORS.barFill)
        status:setAlignment(1)
        status:setAnchorPoint({ x = 0.5, y = 1 })
        status:setPosition({ x = center.x, y = y0 - STATUS_Y_OFFSET })
        status:setZOrder(Z_BAR)
        parent:addChild(status)
    end
end

local function addLoadingUI(parent: CCNode, center: CCPoint, visible: CCSize)
    addGradient(parent, center, visible)
    addLogo(parent, center, visible)
    addProgressBar(parent, center, visible)
end

local function setBar(parent: CCNode, pct: number, text: string?)
    local fill = parent:getChildByID(BAR_FILL_ID)
    if fill then fill:setContentSize({ width = math.max(1, INNER_W * pct), height = INNER_H }) end
    if text then
        local status = parent:getChildByID(BAR_STATUS_ID) :: CCLabelBMFont?
        if status then status:setString(text) end
    end
end

local function stopPoll(layer: LoadingLayer)
    local fields = geode.fields(layer)
    local poll = fields._poll :: TaskHandle?
    if poll then
        poll:cancel()
        fields._poll = nil
    end
end

local function syncBar(layer: LoadingLayer)
    local loaded, enabled, total, current = modStats()
    lastProgress = getProgress(layer, loaded, enabled)
    setBar(layer, lastProgress, statusText(layer, loaded, enabled, total, current))
    if lastProgress >= 1 then stopPoll(layer) end
end

local function startPoll(layer: LoadingLayer)
    local fields = geode.fields(layer)
    if fields._poll then return end
    fields._poll = task.every(POLL_INTERVAL, function()
        if not pcall(syncBar, layer) then stopPoll(layer) end
    end)
end

local function fadeOutOverlay(menu: MenuLayer)
    local overlay = menu:getChildByID(OVERLAY_ID)
    if not overlay then return end

    local children = overlay:getChildren()
    if children then
        for i = 0, children:count() - 1 do
            local child = children:objectAtIndex(i) :: CCNode?
            if not child then continue end
            local fade = cc2d.CCFadeOut.create(FADE_DURATION)
            if fade then
                local eased = cc2d.CCEaseIn.create(fade, FADE_EASE_RATE)
                if eased then child:runAction(eased) end
            end
        end
    end

    task.delay(FADE_DURATION + 0.1, function()
        local node = menu:getChildByID(OVERLAY_ID)
        if node then node:removeFromParentAndCleanup(true) end
    end)
end

local function showMenuLoadingOverlay(menu: MenuLayer)
    if menuOverlayShown then return end
    menuOverlayShown = true

    local center, visible = layout(menu)
    if not center or not visible then return end

    local overlay = cc2d.CCNode.create()
    assert(overlay)
    overlay:setID(OVERLAY_ID)
    overlay:setContentSize(visible)
    overlay:ignoreAnchorPointForPosition(false)
    overlay:setAnchorPoint({ x = 0.5, y = 0.5 })
    overlay:setPosition(center)
    overlay:setZOrder(OVERLAY_Z)
    menu:addChild(overlay)

    addLoadingUI(overlay, { x = visible.width / 2, y = visible.height / 2 }, visible)
    setBar(overlay, 1, "100%\nLoading complete!")
    fadeOutOverlay(menu)
end

function LuauAPILoadingLayer.init()
    if not geode.Mod.getSettingValue("enable-custom-loading-layer") then return end

    geode.hook("geode.gd.LoadingLayer:init/1", {
        after = function(self: LoadingLayer, _: boolean, result: boolean)
            hideVanilla(self)
            local center, visible = layout(self)
            if not center or not visible then return result end
            addLoadingUI(self, center, visible)
            syncBar(self)
            startPoll(self)
            return result
        end,
    })

    geode.hook("geode.gd.MenuLayer:init/0", {
        after = function(self: MenuLayer, result: boolean)
            if result then showMenuLoadingOverlay(self) end
            return result
        end,
    })
end

return LuauAPILoadingLayer
