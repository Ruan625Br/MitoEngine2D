﻿local velocity = vec2:new(0, -200)
local rigidbody, transform, sprite
local goingUp

function start(gameobject)
    mito_log(gameobject.name .. "start called")
    transform = gameobject:get_component_transform()
    rigidbody = gameobject:get_component_rigidbody()
    sprite = gameobject:get_component_sprite()
end

function update(gameobject, deltaTime)
    if goingUp then
        rigidbody.velocity.y = -200
    else
        rigidbody.velocity.y = 200
    end
    
    if transform.position.y < 0 then
        goingUp = false
    end
    if transform.position.y > Window.height then
        --gameover
        mito_log("GAME OVER")
    end
    
    if transform.position.x > Window.width - sprite.width or transform.position.x<0 then
        rigidbody.velocity.x = -rigidbody.velocity.x
    end
end

function on_collision_enter(gameobject, other, direction)
    mito_log(other.name .. " with direction: " .. tostring(direction.x)..","..tostring(direction.y))
    if other.name == "player" then  
        local x_dif = other:get_component_transform().position.x - transform.position.x + other:get_component_sprite().width/2
        goingUp = true
        rigidbody.velocity.x = -x_dif
    end
    if string.find(other.name, "brick") then
        if direction.x ~= 0 and -0 then
            rigidbody.velocity.x = -rigidbody.velocity.x
        else
            goingUp = not goingUp
        end
        destroy(other)
        globals.bricks_destroyed = globals.bricks_destroyed + 1
        end
end

function test()
    mito_log("test function called")
end
