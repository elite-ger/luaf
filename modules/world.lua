local world = {}
world.Gravity = 9.81
world.Blocks = {}

function world:SpawnBlock(typeName, position, size, color)
    return nil
end

function world:GetBlock(id)
    return nil
end

function world:GetAllBlocks()
    return {}
end

function world:Raycast(origin, direction, maxDistance)
    return nil
end

function world:Clear()
end

return world