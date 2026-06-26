local input = {}
input.MousePosition = Vector2.new(0, 0)
input.MouseDelta = Vector2.new(0, 0)
input.MouseWheel = 0

function input:IsKeyDown(key)
    return false
end

function input:IsKeyPressed(key)
    return false
end

function input:IsKeyReleased(key)
    return false
end

function input:IsMouseDown(button)
    return false
end

function input:OnKeyPressed(callback)
end

function input:OnKeyReleased(callback)
end

function input:OnMouseMoved(callback)
end

function input:OnMouseButton(callback)
end

return input