Signal = {}
Signal.__index = Signal

function Signal.new()
    return setmetatable({
        _callbacks = {},
        _once = {}
    }, Signal)
end

function Signal:Connect(callback)
    table.insert(self._callbacks, callback)
    return {
        Disconnect = function()
            for i, cb in ipairs(self._callbacks) do
                if cb == callback then
                    table.remove(self._callbacks, i)
                    return
                end
            end
        end
    }
end

function Signal:Once(callback)
    table.insert(self._once, callback)
end

function Signal:Fire(...)
    for _, cb in ipairs(self._callbacks) do
        cb(...)
    end
    for i = #self._once, 1, -1 do
        self._once[i](...)
        table.remove(self._once, i)
    end
end

function Signal:DisconnectAll()
    self._callbacks = {}
    self._once = {}
end

function Signal:GetCount()
    return #self._callbacks + #self._once
end

return Signal
