_G.everyNodeTask = nil

geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        _G.everyNodeTask = task.everyNode(self, 1, function()
            print("menu is open", time.unix())
        end)
        return result
    end,
})

print("enter the main menu and leave to see what it does.")
