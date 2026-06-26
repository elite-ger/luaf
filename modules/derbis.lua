local debris = {}
debris._items = {}

function debris:Add(item, lifetime)
    local time = require("time")
    table.insert(self._items, {
        item = item,
        destroyTime = time.Total + lifetime
    })
end

function debris:Clear()
    self._items = {}
end

function debris._update(currentTime)
    local i = 1
    while i <= #self._items do
        if currentTime >= self._items[i].destroyTime then
            local item = self._items[i].item
            if item and item.Destroy then
                item:Destroy()
            end
            table.remove(self._items, i)
        else
            i = i + 1
        end
    end
end

return debris