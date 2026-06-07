local Executor = {}

Executor.spacing = 8
Executor.footer = 48
Executor.ranCount = 0
Executor.code = "print('hello from executor :3')"
Executor.status = ""
Executor.initialized = false
Executor.enabled = geode.Mod.getSettingValue("enable-executor")

geode.Mod.listenForSettingChanges("enable-executor", function(value)
    Executor.enabled = value
end)

function Executor.setStatus(successful: boolean, output: string)
    local result
    if successful then
        result = "Success: " .. output
    else
        result = "Error: " .. output
    end
    Executor.status = "Status (#" .. Executor.ranCount .. ") " .. result
    print(Executor.status)
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

    Executor.ranCount += 1

    task.defer(function()
        local ok, res = pcall(fn)
        if ok then
            Executor.setStatus(true, "OK")
        else
            Executor.setStatus(false, tostring(res))
        end
    end)
end

function Executor.draw()
    imgui.window("Executor", function()
        local avail = imgui.getContentRegionAvail()
        local buttonWidth = math.max((avail.x) / 3, 1) - Executor.spacing / 1.5

        Executor.code = imgui.inputTextMultiline("##code", Executor.code, { x = -1, y = -Executor.footer })

        if imgui.button("Execute", { size = { x = buttonWidth, y = 0 } }) then
            Executor.execute(Executor.code)
        end

        imgui.sameLine()
        if imgui.button("Paste", { size = { x = buttonWidth, y = 0 } }) then
            local paste = geode.utils.clipboard.read()
            if paste then
                Executor.code = paste
            end
        end
        
        imgui.sameLine()
        if imgui.button("Clear", { size = { x = buttonWidth, y = 0 } }) then
            Executor.code = ""
            Executor.setStatus(true, "Code cleared")
        end

        imgui.text(Executor.status)
    end, { size = { x = 460, y = 360 } })
end

function Executor.init()
    if Executor.initialized then return end
    Executor.initialized = true

    imgui.onDraw(function()
        if not Executor.enabled then return end
        Executor.draw()
    end)
    Executor.setStatus(true, "Executor is ready")
end

return Executor