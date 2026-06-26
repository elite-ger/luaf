local time = {}
time.Delta = 0.016
time.Total = 0
time.FrameCount = 0
time.TimeScale = 1.0

function time:GetDelta()
    return self.Delta * self.TimeScale
end

function time:GetTotal()
    return self.Total
end

return time