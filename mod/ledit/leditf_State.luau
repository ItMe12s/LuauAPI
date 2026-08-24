local State = {}

function State.new()
	return {
		active = false,
		doc = nil,
		toasts = {},
	}
end

return State
