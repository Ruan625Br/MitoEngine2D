#include "GameObject.h"
#include "../Events/CollisionEnterEvent.h"
#include "../Events/CollisionStayEvent.h"
#include "../Logger/Logger.h"

class CollisionExitEvent;
class CollisionEvent;

int GameObject::GetId() const
{
	return m_id;
}

std::string GameObject::GetName() const
{
	return m_name;
}
std::vector<std::unique_ptr<GameObject>> Registry::m_gameObjects;
int Registry::m_nextFreeId;
std::vector<int> Registry::m_idsToDestroy;

std::unique_ptr<GameObject>& Registry::CreateGameObject(std::string name)
{
	if(name.empty())
	{
		name = "GameObject";
	}
	m_gameObjects.emplace_back(std::make_unique<GameObject>(m_nextFreeId,name));
	Logger::Log("GameObject with id:" + std::to_string(m_nextFreeId) + " created");
	m_nextFreeId++;
	return m_gameObjects.back();
}

void Registry::DestroyGameObject(int id)
{
	m_idsToDestroy.emplace_back(id);
}

void Registry::Start()
{
	for (auto& entity: m_gameObjects) {
		if(entity->HasComponent<ScriptComponent>())
		{
			auto script = entity->GetComponent<ScriptComponent>();
			script->CallStart();
		}
	}
}

void Registry::Update(float deltaTime)
{
	//Update GameObjects
	for (auto& gameObject : m_gameObjects)
	{
		for (auto& componentPair : gameObject->Components)
		{
			if (componentPair.second)
			{
				componentPair.second->Update(deltaTime);
			}
		}
	}
	for (auto& entity: m_gameObjects) {
		if(entity->HasComponent<ScriptComponent>())
		{
			auto script = entity->GetComponent<ScriptComponent>();
			script->CallUpdate(deltaTime);
		}
	}
	
	CalculateCollisions();

	for(auto& id : m_idsToDestroy)
	{
		Logger::Log("Trying to destroy id:" + std::to_string(id));
		auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
							   [id](const std::unique_ptr<GameObject>& obj) {
								   return obj->GetId() == id;
							   });
        
		if (it != m_gameObjects.end())
		{
			Logger::Log("GameObject with id:" + std::to_string(id) + " destroyed");
			if(it->get()->HasComponent<ScriptComponent>())
			{
				GlobalEventBus::UnsubscribeFromOwner(it->get()->GetComponent<ScriptComponent>());
				it->get()->LocalEventBus.Reset();
			}
			
			m_gameObjects.erase(it);
		}
		else
		{
			Logger::Log("GameObject with id:" + std::to_string(id) + " not found");
		}
	}
	m_idsToDestroy.clear();
}

const std::vector<std::unique_ptr<GameObject>>& Registry::GetAllGameObjects() const
{
	return m_gameObjects;
}

const std::unique_ptr<GameObject>& Registry::GetGameObjectFromId(int id)
{
	auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
			[id](const std::unique_ptr<GameObject>& obj) {
				return obj->GetId() == id;
			});

	if (it != m_gameObjects.end()) {
		return *it;
	} else {
		return nullptr; // Return nullptr if not found
	}
}

bool Registry::CheckAABBCollision(double aX, double aY, double aW, double aH, double bX, double bY, double bW,
                                  double bH)
{
	return (
		aX < bX + bW &&
		aX + aW > bX &&
		aY < bY + bH &&
		aY + aH > bY
		);
}

void Registry::CalculateCollisions()
{
	for (auto i = m_gameObjects.begin(); i != m_gameObjects.end(); ++i)
	{
		const std::unique_ptr<GameObject>& a = *i;

		if (!a->HasComponent<TransformComponent>() || !a->HasComponent<BoxCollider2DComponent>())
			continue;

		TransformComponent* aTransform = a->GetComponent<TransformComponent>();
		BoxCollider2DComponent* aCollider = a->GetComponent<BoxCollider2DComponent>();

		// Loop all the entities that still need to be checked (to the right of i)
		for (auto j = i; j != m_gameObjects.end(); ++j)
		{
			const std::unique_ptr<GameObject>& b = *j;

			if (!b->HasComponent<TransformComponent>() || !b->HasComponent<BoxCollider2DComponent>())
				continue;

			// Bypass if we are trying to test the same entity
			if (a == b)
				continue;

			auto bTransform = b->GetComponent<TransformComponent>();
			auto bCollider = b->GetComponent<BoxCollider2DComponent>();

			// Perform the AABB collision check between entities a and b
			bool collisionHappened = CheckAABBCollision(
				aTransform->Position.x + aCollider->Offset.x,
				aTransform->Position.y + aCollider->Offset.y,
				aCollider->Width,
				aCollider->Height,
				bTransform->Position.x + bCollider->Offset.x,
				bTransform->Position.y + bCollider->Offset.y,
				bCollider->Width,
				bCollider->Height
			);
			
			bool wasColliding = false;
			auto range = m_objectsColliding.equal_range(a->GetId());
			for (auto it = range.first; it != range.second; ++it)
			{
				if (it->second == b->GetId())
				{
					wasColliding = true;
					break;
				}
			}
			if (collisionHappened)
			{
				if(!wasColliding)
				{
					Logger::Log("Entities " + std::to_string(a->GetId()) + " and " + std::to_string(b->GetId()) + " started colliding!");
					a->LocalEventBus.EmitEvent<CollisionEnterEvent>(b);
					b->LocalEventBus.EmitEvent<CollisionEnterEvent>(a);
					a->LocalEventBus.EmitEvent<CollisionStayEvent>(b);
					b->LocalEventBus.EmitEvent<CollisionStayEvent>(a);
					m_objectsColliding.insert(std::make_pair(a->GetId(),b->GetId()));
				}else
				{
					a->LocalEventBus.EmitEvent<CollisionStayEvent>(b);
					b->LocalEventBus.EmitEvent<CollisionStayEvent>(a);
				}
			}
			else
			{
				if(wasColliding)
				{
					Logger::Log("Entities " + std::to_string(a->GetId()) + " and " + std::to_string(b->GetId()) + " exit colliding!");
					a->LocalEventBus.EmitEvent<CollisionExitEvent>(b);
                    b->LocalEventBus.EmitEvent<CollisionExitEvent>(a);
					
					// Find and remove the specific key-value pair
					auto range = m_objectsColliding.equal_range(a->GetId());
					for (auto it = range.first; it != range.second; ++it) {
						if (it->second == b->GetId()) {
							m_objectsColliding.erase(it);
							break;
						}
					}
				}
			}
		}
	}
}


