local level = nil

_G.seedvalue_demo_draw = imgui.onDraw(function()
    imgui.window("SeedValue demo", function()
        if not level then
            imgui.text("Open a level info screen.")
            return
        end
        imgui.text(string.format(
            "ID %d  objs %d  attempts %d",
            level.m_levelID,
            level.m_objectCount,
            level.m_attempts
        ))
        if imgui.button("+1 attempt") then
            level.m_attempts += 1
        end
    end)
end)

_G.seedvalue_demo_hook = geode.hook("geode.gd.LevelInfoLayer:init/2", {
    after = function(self, _, _, result)
        if result then
            level = self.m_level
        end
        return result
    end,
})
