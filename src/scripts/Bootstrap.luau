local isDeveloperModeEnabled = geode.Mod.getSettingValue("enable-developer-mode")

if not isDeveloperModeEnabled then return end

require("./Executor").init()