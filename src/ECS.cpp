#include "ECS.h"

using namespace ECS;

std::unique_ptr<Manager, ECS::Deleter<Manager>> ECS::Manager::instance = nullptr;
const std::string ECS::Manager::DEFAULT_POOL_NAME = "DEFAULT";

ECS::Manager::Manager()
{
	// Create default entity pool
	this->entityPools.push_back(std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>(new EntityPool(ECS::Manager::DEFAULT_POOL_NAME), ECS::Deleter<EntityPool>()));
}

ECS::Manager::~Manager()
{
	// Unique ptr will release all for us
	this->entityPools.clear();
}

Manager* ECS::Manager::getInstance()
{
	if (ECS::Manager::instance == nullptr)
	{
		ECS::Manager::instance = std::unique_ptr<Manager, ECS::Deleter<Manager>>(new Manager(), ECS::Deleter<Manager>());
	}

	return ECS::Manager::instance.get();
}

void ECS::Manager::deleteInstance()
{
	if (ECS::Manager::instance != nullptr)
	{
		Manager* managerPtr = ECS::Manager::instance.release();
		ECS::Manager::instance = nullptr;
		if (managerPtr != nullptr)
		{
			delete managerPtr;
		}
		managerPtr = nullptr;
	}
}

void ECS::Manager::sendError(const ERROR_CODE errorCode)
{
	if (this->errorCallback != nullptr)
	{
		std::string msg;
		switch (errorCode)
		{
		case ECS::ERROR_CODE::ECS_INVALID_POOL_NAME:
			msg = "ECS_ERROR: EntityPool name can't be empty or \"" + ECS::Manager::DEFAULT_POOL_NAME + "\".";
			break;
		case ECS::ERROR_CODE::ECS_DUPLICATED_POOL_NAME:
			msg = "ECS_ERROR: There is a pool already with the same name.";
			break;
		case ECS::ERROR_CODE::ECS_POOL_NOT_FOUND:
			msg = "ECS_ERROR: Failed to find pool.";
			break;
		case ECS::ERROR_CODE::ECS_NO_ERROR:
		default:
			return;
			break;
		}

		this->errorCallback(errorCode, msg);
	}
}

const bool ECS::Manager::hasPoolName(const std::string & name)
{
	// Check if there is a pool with same name.
	for (auto& pool : this->entityPools)
	{
		if (pool->name == name)
		{
			return true;
		}
	}

	return false;
}

const bool ECS::Manager::createEntityPool(const std::string& name, const int maxSize)
{
	if (name.empty() || name == ECS::Manager::DEFAULT_POOL_NAME)
	{
		// Pool name can't be empty or default
		this->sendError(ERROR_CODE::ECS_INVALID_POOL_NAME);
		return false;
	}

	if (this->hasPoolName(name))
	{
		// There is a pool with same name
		this->sendError(ERROR_CODE::ECS_DUPLICATED_POOL_NAME);
		return false;
	}

	// Create new
	this->entityPools.push_back(std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>(new EntityPool(name, maxSize), ECS::Deleter<EntityPool>()));
	return true;
}

const bool ECS::Manager::deleteEntityPool(const std::string& name)
{
	if (name.empty() || name == ECS::Manager::DEFAULT_POOL_NAME)
	{
		// Can't delete with empty pool name or default name
		this->sendError(ERROR_CODE::ECS_DUPLICATED_POOL_NAME);
		return false;
	}

	EntityPool* pool = this->detachPool(name);
	if (pool == nullptr)
	{
		// Error is handled in detachPool
		return false;
	}
	else
	{
		delete pool;
		pool = nullptr;
		return true;
	}
}

EntityPool* ECS::Manager::detachPool(const std::string & name)
{
	auto it = this->entityPools.begin();
	for (; it != this->entityPools.end();)
	{
		if ((*it)->name == name)
		{
			EntityPool* poolPtr = (*it).release();
			it = this->entityPools.erase(it);
			return poolPtr;
		}
		else
		{
			it++;
		}
	}

	this->sendError(ERROR_CODE::ECS_POOL_NOT_FOUND);
	return nullptr;
}

EntityPool* ECS::Manager::getPool(const std::string& name)
{
	auto it = this->entityPools.begin();
	for (; it != this->entityPools.end();)
	{
		if ((*it)->name == name)
		{
			// Delete pool
			return (*it).get();
		}
		else
		{
			it++;
		}
	}

	this->sendError(ERROR_CODE::ECS_POOL_NOT_FOUND);
	return nullptr;
}

Entity* ECS::Manager::createEntity(const std::string& poolName)
{
	if (poolName.empty())
	{
		// Pool name can't be empty
		this->sendError(ERROR_CODE::ECS_INVALID_POOL_NAME);
		return nullptr;
	}

	for (auto& entityPool : this->entityPools)
	{
		if (entityPool->name == poolName)
		{
			if (entityPool->nextIndicies.empty())
			{
				// queue is empty. Pool is full
				this->sendError(ERROR_CODE::ECS_POOL_IS_FULL);
				return nullptr;
			}
			else
			{
				unsigned int index = entityPool->nextIndicies.front();
				entityPool->nextIndicies.pop();

				// Assume index is always valid.
				entityPool->pool.at(index)->revive();
				return entityPool->pool.at(index).get();
			}
		}
	}

	// failed to find pool
	this->sendError(ERROR_CODE::ECS_POOL_NOT_FOUND);
	return nullptr;
}

Entity* ECS::Manager::getEntityById(const EID entityId, const std::string& poolName)
{
	if (poolName.empty())
	{
		// Pool name can't be empty
		this->sendError(ERROR_CODE::ECS_INVALID_POOL_NAME);
		return nullptr;
	}

	bool poolFound = false;

	for (auto& entityPool : this->entityPools)
	{
		if (entityPool->name == poolName)
		{
			poolFound = true;
			for (auto& entity : entityPool->pool)
			{
				if (entity->eId == entityId)
				{
					return entity.get();
				}
			}
		}
	}

	if (!poolFound)
	{
		this->sendError(ERROR_CODE::ECS_POOL_NOT_FOUND);
		return nullptr;
	}

	// Entity not found
	this->sendError(ERROR_CODE::ECS_ENTITY_NOT_FOUND);
	return nullptr;
}

const CID ECS::Manager::getCIDFromTypeInfo(const std::type_info& t)
{
	const std::type_index typeIndex = std::type_index(t);
	auto find_it = this->CIDMap.find(typeIndex);
	if (find_it != this->CIDMap.end())
	{
		return find_it->second;
	}
	else
	{
		return INVALID_CID;
	}
}

const bool ECS::Manager::hasComponent(Entity* e, const std::type_info & t)
{
	const CID cId = this->getCIDFromTypeInfo(t);
	if (cId != INVALID_CID)
	{
		return true;
	}
	else
	{
		return false;
	}
}

Component* ECS::Manager::getComponent(Entity* e, const std::type_info& t)
{
	const CID cId = this->getCIDFromTypeInfo(t);
	if (cId != INVALID_CID)
	{
		std::set<CINDEX> cIndicies;
		e->getCINDEXByCID(cId, cIndicies);
		if (cIndicies.empty())
		{
			return nullptr;
		}
		else
		{
			try
			{
				// Just return first one becuse this isn't getCompoents
				return this->components.at(cId).at(*cIndicies.begin()).get();
			}
			catch (...)
			{
				return nullptr;
			}
		}
	}
	else
	{
		return nullptr;
	}
}

std::vector<Component*> ECS::Manager::getComponents(Entity* e, const std::type_info& t)
{
	std::vector<Component*> ret;

	const CID cId = this->getCIDFromTypeInfo(t);
	if (cId != INVALID_CID)
	{
		std::set<CINDEX> cIndicies;
		e->getCINDEXByCID(cId, cIndicies);
		if (cIndicies.empty())
		{
			return ret;
		}
		else
		{
			try
			{
				// Just return first one becuse this isn't getCompoents
				std::vector<std::unique_ptr<Component, ComponentDeleter<Component>>>& components = this->components.at(cId);

				for (auto& component : components)
				{
					ret.push_back(component.get());
				}

				return ret;
			}
			catch (...)
			{
				return ret;
			}
		}
	}
	else
	{
		return ret;
	}
}

const bool ECS::Manager::addComponent(Entity* e, const std::type_info& t, Component* c)
{
	if (c == nullptr) return false;

	const CID cId = this->getCIDFromTypeInfo(t);
	if (cId != INVALID_CID)
	{
		// This component type was added before.
		try
		{
			this->components.at(cId).push_back(std::unique_ptr<Component, ComponentDeleter<Component>>(c, ComponentDeleter<Component>()));
			const CINDEX cIndex = this->components.at(cId).size();
			c->id = cId;
		}
		catch (...)
		{
			return false;
		}
	}
	else
	{
		// This component never had added. 
	}
	return true;
}

const bool ECS::Manager::removeComponent(Entity * e, const std::type_info & t)
{
	return false;
}


//============================================================================================

ECS::EntityPool::EntityPool(const std::string& name, const unsigned int maxSize) 
: maxPoolSize(maxSize)
{
	// Allocate entities
	for (unsigned int i = 0; i < maxSize; i++)
	{
		this->pool.push_back(std::unique_ptr<ECS::Entity, ECS::Deleter<Entity>>(new Entity(), ECS::Deleter<Entity>()));
		this->pool.back()->alive = false;
		this->pool.back()->eId = ++ECS::Entity::idCounter;
		this->pool.back()->eIndex = i;
		this->nextIndicies.push(i);
	}
}

ECS::EntityPool::~EntityPool()
{
	// unique ptr will release for us
	pool.clear();
	this->maxPoolSize = 0;
}

const bool ECS::EntityPool::isValidIndex(const unsigned int index)
{
	return (index >= 0) && (index < this->pool.size());
}

const unsigned int ECS::EntityPool::getEntityCount(const bool onlyAlive)
{
	unsigned int count = 0;
	for (auto& entity : this->pool)
	{
		if (onlyAlive)
		{
			if (entity->alive)
			{
				count++;
			}
		}
		else
		{
			count++;
		}
	}

	return count;
}

//============================================================================================

EID ECS::Entity::idCounter = 0;

ECS::Entity::Entity()
: alive(false)
, eId(++ECS::Entity::idCounter)
, eIndex(-1)
, signature(0)
{
	this->wrapIdCounter();
}

void ECS::Entity::wrapIdCounter()
{
	if (ECS::Entity::idCounter >= ECS::MAX_EID)
	{
		// id Counter reached max number. Wrap to 0. 
		// This means user created more 4294967295 entities, which really doesn't happen all that much.
		ECS::Entity::idCounter = 0;
	}
}

void ECS::Entity::getCINDEXByCID(const CID cId, std::set<CINDEX>& cIndicies)
{
	try
	{
		for (auto index : this->componentIndicies.at(cId))
		{
			cIndicies.insert(index);
		}
	}
	catch (...)
	{
		return;
	}
}

const bool ECS::Entity::addCINDEXToCID(const CID cId, const CINDEX cIndex)
{
	return true;
}

void ECS::Entity::revive()
{
	this->alive = true;
	this->eId = ++ECS::Entity::idCounter;
	this->wrapIdCounter();
}

void ECS::Entity::kill()
{
	// Wipe everything
	this->alive = false;
	this->eId = -1;
}

const EID ECS::Entity::getEntityId()
{
	return this->eId;
}

const bool ECS::Entity::isAlive()
{
	return this->alive;
}

//============================================================================================

CID ECS::Component::idCounter = 0;

Component::Component() 
: id(INVALID_CID) 
{}

void Component::wrapIdCounter()
{
	if (Component::idCounter >= INVALID_CID)
	{
		Component::idCounter = 0;
	}
}

const CID ECS::Component::getId()
{
	return this->id;
}
