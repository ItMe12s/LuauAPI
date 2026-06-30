require("./LuauAPI_LoadingLayer").init()
require("./LuauAPI_MenuLayer").init()

local isDeveloperModeEnabled = geode.Mod.getSettingValue("enable-developer-mode")
if not isDeveloperModeEnabled then return end
warn("Developer Mode enabled! use with care :3")

local Executor = require("./Executor")
if geode.Mod.getSettingValue("enable-executor") then
    Executor.init()
end
