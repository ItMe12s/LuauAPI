local code = 'print("hello from executor")'
local output = ""

imgui.onDraw(function()
    imgui.window("Script Executor", function()
        code = imgui.inputTextMultiline("##code", code, { x = 0, y = 240 })

        if imgui.button("Execute") then
            local fn, err = loadstring(code, "=executor")
            if not fn then
                output = "compile error: " .. tostring(err)
            else
                task.defer(function()
                    local ok, res = pcall(fn)
                    if ok then
                        output = res ~= nil and ("ok: " .. tostring(res)) or "ok"
                    else
                        output = "runtime error: " .. tostring(res)
                    end
                end)
            end
        end

        imgui.sameLine()
        if imgui.button("Clear") then
            output = ""
        end

        if output ~= "" then
            imgui.separator()
            imgui.text(output)
        end
    end, { size = { x = 460, y = 360 } })
end)
