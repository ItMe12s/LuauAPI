local Transform = gd3d.Transform
local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local mesh, meshErr = gd3d.gltf.loadMesh("resources", "test_donut.glb")
if not mesh then
    print("failed to load mesh:", meshErr)
    return
end

local vp = gd3d.ViewportFrame.new(300, 300)
if not vp then
    print("failed to create ViewportFrame")
    return
end

vp:setID(modId .. "/viewport-demo")
vp:setCamera(Transform.new({ x = 0, y = 1, z = 3 }, { x = 0, y = 0, z = 0 }), 90, 0.1, 100)
vp:setBackgroundColor({ r = 0.05, g = 0.06, b = 0.1, a = 0.85 })
vp:setLight({ x = -0.4, y = 0.9, z = 0.3 }, { x = 1, y = 0.95, z = 0.85 }, 1.2)
vp:setAmbient(0.12)

local triMesh = gd3d.mesh.new({
    positions = {
        { x = 0, y = 0.35, z = 0 },
        { x = -0.3, y = -0.25, z = 0 },
        { x = 0.3, y = -0.25, z = 0 },
    },
    indices = { 1, 2, 3 },
})
if triMesh then
    vp:addMesh(
        triMesh,
        Transform.new({ x = -1.2, y = 0, z = 0 }),
        gd3d.Material.new({ color = { r = 0.2, g = 0.7, b = 1, a = 1 } })
    )
end

vp:addDebugLine({ x = -1.5, y = -0.5, z = 0 }, { x = 1.5, y = 0.5, z = 0 }, { x = 1, y = 0.3, z = 0.2 })

local instanceId = vp:addMesh(mesh, Transform.new())
local angle = 0
local colorPhase = 2 * math.pi / 3

local spinTask
spinTask = task.every(1 / 120, function()
    if spinTask == nil then
        return
    end
    angle += 0.01
    vp:setInstanceTransform(instanceId, Transform.fromEuler(0.4, angle, 0))
    vp:setInstanceColor(instanceId, {
        x = 0.6 + 0.4 * math.cos(angle),
        y = 0.6 + 0.4 * math.cos(angle + colorPhase),
        z = 0.6 + 0.4 * math.cos(angle + 2 * colorPhase),
    })
end)

geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        local director = cc2d.CCDirector.sharedDirector()
        if not director then
            return result
        end

        local winSize = director:getWinSize()
        vp:setPosition({ x = winSize.width / 2, y = winSize.height / 2 })
        self:addChild(vp)
        return result
    end,
})
