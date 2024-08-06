﻿#pragma once
#include "GameObject.h"
#include "../AssetStore/AssetManager.h"
#include "../AssetStore/RendererManager.h"

#include "../Events/KeyPressedEvent.h"
#include "../Events/CollisionStayEvent.h"
#include "../Events/GlobalEventBus.h"

class GameObject;

class Component
{
public:
    explicit Component(GameObject* owner) : m_owner(owner)
    {
    }

    virtual ~Component() = default;
    virtual void Update(float deltaTime) = 0;

protected:
    GameObject* m_owner;
};

class TransformComponent : public Component
{
public:
    explicit TransformComponent(GameObject* owner,
                                std::optional<glm::vec2> position = std::nullopt,
                                std::optional<double> scale = std::nullopt,
                                std::optional<double> rotation = std::nullopt)
        : Component(owner), 
          Position(position.value_or(glm::vec2{})), 
          Scale(scale.value_or(1.0)), 
          Rotation(rotation.value_or(0.0))
    {
    }

    void Update(float deltaTime) override;

    glm::vec2 Position;
    double Scale;
    double Rotation;
};

class SpriteComponent : public Component
{
public:
    explicit SpriteComponent(GameObject* owner, 
                             std::string sprite,
                             std::optional<int> width = std::nullopt, 
                             std::optional<int> height = std::nullopt,
                             std::optional<std::uint8_t> red = std::nullopt, 
                             std::optional<std::uint8_t> green = std::nullopt, 
                             std::optional<std::uint8_t> blue = std::nullopt) 
        : Component(owner), 
          m_sprite(sprite),
          m_width(width.value_or(0)), 
          m_height(height.value_or(0)), 
          m_color(Color{red.value_or(255), green.value_or(255), blue.value_or(255)})
    {
    }

    void Update(float deltaTime) override;

private:
    std::string m_sprite;
    int m_width;
    int m_height;
    Color m_color;
};

class ControllerComponent : public Component
{
public:
    explicit ControllerComponent(GameObject* owner);
    void Update(float deltaTime) override;
    void OnCollisionStay(CollisionStayEvent& event);
    void OnCollisionEnter(CollisionEnterEvent& event);
    void OnCollisionExit(CollisionExitEvent& event);
    void OnKeyPressedEvent(KeyPressedEvent& event);
    void OnKeyReleasedEvent(KeyReleasedEvent& event);

private:
    bool m_colliding = false;
    glm::vec2 m_currentVelocity;
    glm::vec2 m_previousVelocity;
};

class RigidBody2DComponent : public Component
{
public:
    explicit RigidBody2DComponent(GameObject* owner, 
                                  std::optional<glm::vec2> velocity = std::nullopt) 
        : Component(owner), 
          Velocity(velocity.value_or(glm::vec2(0.0, 0.0)))
    {
    }

    void Update(float deltaTime) override;
    glm::vec2 Velocity;
};

class BoxCollider2DComponent : public Component
{
public:
    explicit BoxCollider2DComponent(GameObject* owner, 
                                    std::optional<int> width = std::nullopt, 
                                    std::optional<int> height = std::nullopt, 
                                    std::optional<glm::vec2> offset = std::nullopt)
        : Component(owner), 
          Width(width.value_or(0)), 
          Height(height.value_or(0)), 
          Offset(offset.value_or(glm::vec2(0)))
    {
    }

    void Update(float deltaTime) override;
    int Width;
    int Height;
    glm::vec2 Offset;
};

class ScriptComponent : public Component
{
public:
    ScriptComponent(GameObject* owner)
        : Component(owner)
    {}
    void OnCollisionStay(CollisionStayEvent& event);
    void OnCollisionEnter(CollisionEnterEvent& event);
    void OnCollisionExit(CollisionExitEvent& event);
    void OnKeyPressedEvent(KeyPressedEvent& event);
    void OnKeyReleasedEvent(KeyReleasedEvent& event);
    void AddScript(sol::environment& luaEnv);
    sol::function GetScriptFunction(const std::string& name);
    void Update(float deltaTime) override;
    void CallUpdate(float deltaTime);
    void CallStart();
    std::vector<sol::function> StartFunc;
    std::vector<sol::function> UpdateFunc;
    std::map<std::string, sol::function> scriptFunctions;
    std::unordered_map<std::string, std::function<void(Event&)>> scriptFunctions2;
};

template<typename... Args>
    void CallLuaFunction(sol::function& func, Args&&... args) {
    if (func == sol::lua_nil)
        return;

    sol::protected_function_result safe_result = func(std::forward<Args>(args)...);
    if (!safe_result.valid()) {
        sol::error err = safe_result;
        std::cout << "Error calling Lua function: " << err.what() << '\n';
    }
}