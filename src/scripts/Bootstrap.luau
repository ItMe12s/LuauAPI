local isDeveloperModeEnabled = geode.Mod.getSettingValue("enable-developer-mode")

print("Developer mode enabled:", isDeveloperModeEnabled)
if not isDeveloperModeEnabled then return end

require("./Executor").init()