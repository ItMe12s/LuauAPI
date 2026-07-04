require("./luauapi_MenuLayer").init()

local isDeveloperModeEnabled = geode.Mod.getSettingValue("enable-developer-mode")
if not isDeveloperModeEnabled then return end
warn("Developer Mode enabled! use with care :3")

local Executor = require("./luauapi_Executor")
if geode.Mod.getSettingValue("enable-executor") then
    Executor.init()
end
