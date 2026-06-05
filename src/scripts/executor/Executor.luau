local Executor = {}

Executor.ranCount = 0
Executor.status = ""
Executor.enabled = geode.Mod.getSettingValue("enable-executor")

function Executor.setStatus(successful: boolean, output: string)
    local prefix
    if successful then
        prefix = "Success: "
    else
        prefix = "Error: "
    end
    Executor.status = "Status (#" .. Executor.ranCount .. ") " .. prefix .. output
end

function Executor.execute(code: string)
    if code == "" then
        Executor.setStatus(false, "Code is empty")
        return
    end

    local fn, err = loadstring(code, "=executor")
    if not fn then
        Executor.setStatus(false, tostring(err))
        return
    end

    Executor.setStatus(true, "OK")
end

function Executor.init()
    imgui.onDraw(function()
        if not Executor.enabled then return end
        
        imgui.window("Executor", function()
            imgui.text(Executor.status)
        end)
    end)
end

return Executor