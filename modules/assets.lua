local assets = {}
assets._cache = {}

function assets:LoadTexture(path)
    return nil
end

function assets:LoadSound(path)
    return nil
end

function assets:LoadModel(path)
    return nil
end

function assets:GetCached(path)
    return self._cache[path]
end

return assets