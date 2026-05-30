local code = 'print("hello from executor :3")'
local output = ""
local ranCount = 0

imgui.onDraw(function()
    imgui.window("LuauAPI Script Executor", function()
        local avail = imgui.getContentRegionAvail()
        local spacing = 8
        local footer = 64

        code = imgui.inputTextMultiline("##code", code, { x = -1, y = -footer })

        local buttonWidth = math.max((avail.x - spacing) / 2, 1)

        if imgui.button("Execute", { size = { x = buttonWidth, y = 0 } }) then
            local fn, err = loadstring(code, "=executor")
            if not fn then
                output = "compile error: " .. tostring(err)
                print("[executor] " .. output)
            else
                task.defer(function()
                    ranCount = ranCount + 1
                    local ok, res = pcall(fn)
                    if ok then
                        output = res ~= nil and ("ok: " .. tostring(res)) or "ok"
                    else
                        output = "runtime error: " .. tostring(res)
                        print("[executor] " .. output)
                    end
                end)
            end
        end

        imgui.sameLine()
        if imgui.button("Clear", { size = { x = buttonWidth, y = 0 } }) then
            output = ""
        end

        imgui.text("Status (#" .. ranCount .. "): " .. output)
    end, { size = { x = 460, y = 360 } })
end)
