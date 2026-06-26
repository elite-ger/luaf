Easing = {}

Easing.Linear = function(t) return t end

Easing.QuadIn = function(t) return t * t end
Easing.QuadOut = function(t) return 1 - (1 - t) * (1 - t) end
Easing.QuadInOut = function(t)
    if t < 0.5 then return 2 * t * t end
    return 1 - (-2 * t + 2) * (-2 * t + 2) / 2
end

Easing.CubicIn = function(t) return t * t * t end
Easing.CubicOut = function(t) return 1 - (1 - t) * (1 - t) * (1 - t) end
Easing.CubicInOut = function(t)
    if t < 0.5 then return 4 * t * t * t end
    return 1 - (-2 * t + 2) * (-2 * t + 2) * (-2 * t + 2) / 2
end

Easing.SineIn = function(t) return 1 - math.cos(t * math.pi / 2) end
Easing.SineOut = function(t) return math.sin(t * math.pi / 2) end
Easing.SineInOut = function(t) return -(math.cos(math.pi * t) - 1) / 2 end

Easing.ElasticOut = function(t)
    if t == 0 or t == 1 then return t end
    return math.pow(2, -10 * t) * math.sin((t - 0.075) * 2 * math.pi / 0.3) + 1
end

Easing.BounceOut = function(t)
    if t < 1 / 2.75 then return 7.5625 * t * t
    elseif t < 2 / 2.75 then t = t - 1.5 / 2.75; return 7.5625 * t * t + 0.75
    elseif t < 2.5 / 2.75 then t = t - 2.25 / 2.75; return 7.5625 * t * t + 0.9375
    else t = t - 2.625 / 2.75; return 7.5625 * t * t + 0.984375 end
end

Easing.BackOut = function(t)
    local c1 = 1.70158
    local c3 = c1 + 1
    return 1 + c3 * math.pow(t - 1, 3) + c1 * math.pow(t - 1, 2)
end

return Easing