local Transform = gd3d.Transform
local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local colorPhase = 2 * math.pi / 3

local state = {
    width = 300,
    height = 300,
    posX = 0,
    posY = 0,
    composite = true,
    debugBounds = false,
    camPos = { x = 0, y = 1, z = 3 },
    lookAt = { x = 0, y = 0, z = 0 },
    fov = 90,
    near = 0.1,
    far = 100,
    bg = { r = 0.05, g = 0.06, b = 0.1, a = 0.85 },
    lightDir = { x = -0.4, y = 0.9, z = 0.3 },
    lightColor = { x = 1, y = 0.95, z = 0.85 },
    intensity = 1.2,
    ambient = 0.12,
    autoSpin = true,
    spinSpeed = 0.01,
    pitch = 0.4,
    yaw = 0,
    roll = 0,
    angle = 0,
    autoColor = true,
    color = { x = 1, y = 1, z = 1 },
    triPos = { x = -1.2, y = 0, z = 0 },
    triMat = { r = 0.2, g = 0.7, b = 1, a = 1 },
    lineFrom = { x = -1.5, y = -0.5, z = 0 },
    lineTo = { x = 1.5, y = 0.5, z = 0 },
    lineColor = { x = 1, y = 0.3, z = 0.2 },
    lineId = nil :: number?,
    instanceId = nil :: number?,
    triId = nil :: number?,
}

local mesh, meshErr = gd3d.gltf.loadMesh("resources", "test_donut.glb")
if not mesh then
    print("failed to load mesh:", meshErr)
    return
end

local vp = gd3d.ViewportFrame.new(state.width, state.height)
if not vp then
    print("failed to create ViewportFrame")
    return
end

vp:setID(modId .. "/viewport-demo")
vp:setZOrder(67)

local triMesh = gd3d.mesh.new({
    positions = {
        { x = 0, y = 0.35, z = 0 },
        { x = -0.3, y = -0.25, z = 0 },
        { x = 0.3, y = -0.25, z = 0 },
    },
    indices = { 1, 2, 3 },
})
if triMesh then
    state.triId = vp:addMesh(
        triMesh,
        Transform.new(state.triPos),
        gd3d.Material.new({ color = state.triMat })
    )
end

state.instanceId = vp:addMesh(mesh, Transform.new())

local syncedLine = {
    from = { x = 0, y = 0, z = 0 },
    to = { x = 0, y = 0, z = 0 },
    color = { x = 0, y = 0, z = 0 },
}

local function vec3Equal(a: { x: number, y: number, z: number }, b: { x: number, y: number, z: number }): boolean
    return a.x == b.x and a.y == b.y and a.z == b.z
end

local function applyCamera()
    vp:setCamera(Transform.new(state.camPos, state.lookAt), state.fov, state.near, state.far)
end

local function applyBackground()
    vp:setBackgroundColor(state.bg)
end

local function applyLight()
    vp:setLight(state.lightDir, state.lightColor, state.intensity)
    vp:setAmbient(state.ambient)
end

local function applyViewportNode()
    vp:setContentSize({ width = state.width, height = state.height })
    vp:setPosition({ x = state.posX, y = state.posY })
    vp:setCompositeEnabled(state.composite)
    vp:setDebugBounds(state.debugBounds)
end

local function applyDonutTransform()
    if not state.instanceId then
        return
    end
    vp:setInstanceTransform(state.instanceId, Transform.fromEuler(state.pitch, state.yaw, state.roll))
end

local function applyDonutColor()
    if not state.instanceId then
        return
    end
    if state.autoColor then
        local angle = state.angle
        vp:setInstanceColor(state.instanceId, {
            x = 0.6 + 0.4 * math.cos(angle),
            y = 0.6 + 0.4 * math.cos(angle + colorPhase),
            z = 0.6 + 0.4 * math.cos(angle + 2 * colorPhase),
        })
    else
        vp:setInstanceColor(state.instanceId, state.color)
    end
end

local function applyTriTransform()
    if not state.triId then
        return
    end
    vp:setInstanceTransform(state.triId, Transform.new(state.triPos))
end

local function applyTriMaterial()
    if not state.triId then
        return
    end
    vp:setInstanceMaterial(state.triId, gd3d.Material.new({ color = state.triMat }))
end

local function syncDebugLine()
    if vec3Equal(syncedLine.from, state.lineFrom)
        and vec3Equal(syncedLine.to, state.lineTo)
        and vec3Equal(syncedLine.color, state.lineColor)
    then
        return
    end

    if state.lineId then
        vp:removeDebugLine(state.lineId)
    end

    state.lineId = vp:addDebugLine(state.lineFrom, state.lineTo, state.lineColor)

    syncedLine.from.x = state.lineFrom.x
    syncedLine.from.y = state.lineFrom.y
    syncedLine.from.z = state.lineFrom.z
    syncedLine.to.x = state.lineTo.x
    syncedLine.to.y = state.lineTo.y
    syncedLine.to.z = state.lineTo.z
    syncedLine.color.x = state.lineColor.x
    syncedLine.color.y = state.lineColor.y
    syncedLine.color.z = state.lineColor.z
end

local function applyAll()
    applyCamera()
    applyBackground()
    applyLight()
    applyViewportNode()
    applyDonutTransform()
    applyDonutColor()
    applyTriTransform()
    applyTriMaterial()
    syncDebugLine()
end

applyAll()

local spinTask
spinTask = task.every(1 / 120, function()
    if spinTask == nil then
        return
    end
    if state.autoSpin then
        state.angle += state.spinSpeed
        state.yaw = state.angle
    end
    applyDonutTransform()
    applyDonutColor()
end)

geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        local director = cc2d.CCDirector.sharedDirector()
        if not director then
            return result
        end

        local winSize = director:getWinSize()
        state.posX = winSize.width / 2
        state.posY = winSize.height / 2
        applyViewportNode()
        self:addChild(vp)
        return result
    end,
})

local function sliderVec3(label: string, v: { x: number, y: number, z: number }, min: number, max: number)
    v.x = imgui.sliderFloat(label .. " X", v.x, min, max)
    v.y = imgui.sliderFloat(label .. " Y", v.y, min, max)
    v.z = imgui.sliderFloat(label .. " Z", v.z, min, max)
end

local function sliderRgba(label: string, c: { r: number, g: number, b: number, a: number })
    c.r = imgui.sliderFloat(label .. " R", c.r, 0, 1)
    c.g = imgui.sliderFloat(label .. " G", c.g, 0, 1)
    c.b = imgui.sliderFloat(label .. " B", c.b, 0, 1)
    c.a = imgui.sliderFloat(label .. " A", c.a, 0, 1)
end

local drawOpen = true

_G.viewportdemo_imguiHandle = imgui.onDraw(function()
    if not drawOpen then
        return
    end
    drawOpen = imgui.window("Viewport Demo", function()
        imgui.text("Viewport")
        state.width = imgui.sliderInt("Width", state.width, 64, 1024)
        state.height = imgui.sliderInt("Height", state.height, 64, 1024)
        state.posX = imgui.sliderFloat("Pos X", state.posX, 0, 1920)
        state.posY = imgui.sliderFloat("Pos Y", state.posY, 0, 1080)
        state.composite = imgui.checkbox("Composite enabled", state.composite)
        state.debugBounds = imgui.checkbox("Debug bounds", state.debugBounds)
        if imgui.button("Center on screen") then
            local director = cc2d.CCDirector.sharedDirector()
            if director then
                local winSize = director:getWinSize()
                state.posX = winSize.width / 2
                state.posY = winSize.height / 2
            end
        end
        applyViewportNode()

        imgui.separator()
        imgui.text("Camera")
        sliderVec3("Cam pos", state.camPos, -10, 10)
        sliderVec3("Look at", state.lookAt, -5, 5)
        state.fov = imgui.sliderFloat("FOV", state.fov, 10, 120)
        state.near = imgui.sliderFloat("Near", state.near, 0.01, 10, "%.2f")
        state.far = imgui.sliderFloat("Far", state.far, 10, 500)
        applyCamera()

        imgui.separator()
        imgui.text("Background")
        sliderRgba("BG", state.bg)
        applyBackground()

        imgui.separator()
        imgui.text("Lighting")
        sliderVec3("Light dir", state.lightDir, -1, 1)
        sliderVec3("Light color", state.lightColor, 0, 2)
        state.intensity = imgui.sliderFloat("Intensity", state.intensity, 0, 4)
        state.ambient = imgui.sliderFloat("Ambient", state.ambient, 0, 4)
        applyLight()

        imgui.separator()
        imgui.text("Donut instance")
        state.autoSpin = imgui.checkbox("Auto spin", state.autoSpin)
        state.spinSpeed = imgui.sliderFloat("Spin speed", state.spinSpeed, 0, 0.1, "%.3f")
        if not state.autoSpin then
            state.pitch = imgui.sliderFloat("Pitch", state.pitch, -math.pi, math.pi, "%.2f")
            state.yaw = imgui.sliderFloat("Yaw", state.yaw, -math.pi, math.pi, "%.2f")
            state.roll = imgui.sliderFloat("Roll", state.roll, -math.pi, math.pi, "%.2f")
        else
            state.pitch = imgui.sliderFloat("Pitch", state.pitch, -math.pi, math.pi, "%.2f")
            imgui.text("Yaw driven by auto spin")
        end
        state.autoColor = imgui.checkbox("Auto color", state.autoColor)
        if not state.autoColor then
            sliderVec3("Color", state.color, 0, 2)
        end
        applyDonutTransform()
        applyDonutColor()

        imgui.separator()
        imgui.text("Triangle instance")
        if state.triId then
            sliderVec3("Tri pos", state.triPos, -5, 5)
            sliderRgba("Tri mat", state.triMat)
            applyTriTransform()
            applyTriMaterial()
        else
            imgui.text("Triangle mesh unavailable")
        end

        imgui.separator()
        imgui.text("Debug line")
        sliderVec3("From", state.lineFrom, -3, 3)
        sliderVec3("To", state.lineTo, -3, 3)
        sliderVec3("Line color", state.lineColor, 0, 2)
        syncDebugLine()

        imgui.separator()
        imgui.text("Info")
        imgui.text("Donut id: " .. tostring(state.instanceId))
        imgui.text("Tri id: " .. tostring(state.triId))
        imgui.text("Line id: " .. tostring(state.lineId))
        imgui.text("Instance count: " .. tostring(vp:instanceCount()))
    end, { size = { x = 440, y = 680 }, closable = true })
end)
