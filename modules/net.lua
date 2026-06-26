local net = {}
net._listeners = {}

function net:Broadcast(event, data)
end

function net:Send(player, event, data)
end

function net:OnEvent(event, callback)
    if not self._listeners[event] then
        self._listeners[event] = {}
    end
    table.insert(self._listeners[event], callback)
    return {
        Disconnect = function()
            for i, cb in ipairs(self._listeners[event]) do
                if cb == callback then
                    table.remove(self._listeners[event], i)
                    return
                end
            end
        end
    }
end

function net._fireEvent(event, ...)
    if self._listeners[event] then
        for _, cb in ipairs(self._listeners[event]) do
            cb(...)
        end
    end
end

return net