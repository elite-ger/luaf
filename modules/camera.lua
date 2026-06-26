local camera = {}
camera.Position = Vector3.new(0, 10, 0)
camera.Target = Vector3.new(0, 0, 0)
camera.FOV = 70
camera.NearPlane = 0.1
camera.FarPlane = 1000

function camera:LookAt(position, target)
    self.Position = position
    self.Target = target
end

function camera:GetDirection()
    return (self.Target - self.Position):Unit()
end

function camera:GetRight()
    local forward = self:GetDirection()
    local worldUp = Vector3.new(0, 1, 0)
    return forward:Cross(worldUp):Unit()
end

function camera:GetUp()
    local forward = self:GetDirection()
    local right = self:GetRight()
    return right:Cross(forward):Unit()
end

return camera