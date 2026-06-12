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

local instanceId = vp:addMesh(mesh, Transform.new())
vp:setInstanceColor(instanceId, { x = 1, y = 0.85, z = 0.7 })
local angle = 0

local spinTask
spinTask = task.every(1 / 60, function()
    if spinTask == nil then
        return
    end
    angle += 0.02
    vp:setInstanceTransform(instanceId, Transform.fromEuler(0.4, angle, 0))
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
