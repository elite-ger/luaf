local run = {}
run._heartbeatCallbacks = {}
run._steppedCallbacks = {}
run._renderSteppedCallbacks = {}
run._lateCallbacks = {}

local function makeConnection(list, callback)
    table.insert(list, callback)
    return {
        Disconnect = function()
            for i, cb in ipairs(list) do
                if cb == callback then
                    table.remove(list, i)
                    return
                end
            end
        end
    }
end

function run:OnHeartbeat(callback)
    return makeConnection(self._heartbeatCallbacks, callback)
end

function run:OnStepped(callback)
    return makeConnection(self._steppedCallbacks, callback)
end

function run:OnRenderStepped(callback)
    return makeConnection(self._renderSteppedCallbacks, callback)
end

function run:OnLateUpdate(callback)
    return makeConnection(self._lateCallbacks, callback)
end

return run