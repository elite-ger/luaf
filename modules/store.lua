local store = {}
store._data = {}

function store:Get(key)
    return self._data[key]
end

function store:Set(key, value)
    self._data[key] = value
end

function store:Delete(key)
    self._data[key] = nil
end

function store:Has(key)
    return self._data[key] ~= nil
end

function store:Clear()
    self._data = {}
end

function store:GetAll()
    local copy = {}
    for k, v in pairs(self._data) do
        copy[k] = v
    end
    return copy
end

return store