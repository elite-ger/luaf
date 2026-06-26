local game = {}
game.IsRunning = false
game.IsPaused = false

function game:Start()
    self.IsRunning = true
    self.IsPaused = false
end

function game:End()
    self.IsRunning = false
end

function game:Pause()
    self.IsPaused = true
end

function game:Resume()
    self.IsPaused = false
end

function game:Reset()
    self.IsRunning = false
    self.IsPaused = false
    local world = require("world")
    world:Clear()
    local debris = require("debris")
    debris:Clear()
end

return game