local Easing = require("easing")

local Tween = {}
Tween.__index = Tween

function Tween.new(object, properties, duration, easing, onComplete)
    return setmetatable({
        _object = object,
        _startValues = {},
        _endValues = properties,
        _duration = duration or 1,
        _easing = easing or Easing.QuadOut,
        _onComplete = onComplete,
        _elapsed = 0,
        _playing = false,
        _paused = false,
        _completed = false
    }, Tween)
end

function Tween:Play()
    if self._playing then return end
    self._playing = true
    for k, v in pairs(self._endValues) do
        local current = self._object[k]
        if current == nil then
            self._startValues[k] = 0
        elseif type(current) == "table" and current.Clone then
            self._startValues[k] = current:Clone()
        else
            self._startValues[k] = current
        end
    end
end

function Tween:Pause()
    self._paused = true
end

function Tween:Resume()
    self._paused = false
end

function Tween:Stop()
    self._playing = false
    self._completed = true
end

function Tween:Update(dt)
    if not self._playing or self._paused or self._completed then return end
    self._elapsed = self._elapsed + dt
    local t = math.min(self._elapsed / self._duration, 1.0)
    local easedT = self._easing(t)

    for k, endVal in pairs(self._endValues) do
        local startVal = self._startValues[k]

        if type(endVal) == "table" and endVal.Lerp and endVal.Type then
            if startVal.Type ~= endVal.Type then
                error("Tween: type mismatch for key '" .. k .. "': "
                    .. (startVal.Type or "?") .. " vs " .. (endVal.Type or "?"))
            end
            self._object[k] = startVal:Lerp(endVal, easedT)
        elseif type(endVal) == "number" then
            if type(startVal) ~= "number" then
                error("Tween: expected number for key '" .. k .. "', got " .. type(startVal))
            end
            self._object[k] = startVal + (endVal - startVal) * easedT
        else
            error("Tween: unsupported type for key '" .. k .. "': " .. type(endVal))
        end
    end

    if t >= 1.0 then
        self._completed = true
        self._playing = false
        if self._onComplete then self._onComplete() end
    end
end

return Tween