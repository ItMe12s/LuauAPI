local isDeveloperModeEnabled = geode.Mod.getSettingValue("enable-developer-mode")

if not isDeveloperModeEnabled then return end

local Executor = require("./Executor")
if geode.Mod.getSettingValue("enable-executor") then
    Executor.init()
end
