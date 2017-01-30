#include "ECS.h"

using namespace ECS;

std::unique_ptr<Manager, ECS::Deleter<Manager>> Manager::instance = nullptr;
const std::string Manager::DEFAULT_POOL_NAME = "DEFAULT";

Manager::Manager()
{
	// Create default entity pool
	this->entityPools.push_back(std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>(new EntityPool(Manager::DEFAULT_POOL_NAME), ECS::Deleter<EntityPool>()));
}

Manager::~Manager()
{
	// Unique ptr will release all for us
	this->entityPools.clear();
}

Manager* Manager::getInstance()
{
	if (Manager::instance == nullptr)
	{
		Manager::instance = std::unique_ptr<Manager, ECS::Deleter<Manager>>(new Manager(), ECS::Deleter<Manager>());
	}

	return Manager::instance.get();
}

void Manager::deleteInstance()
{
	if (Manager::instance != nullptr)
	{
		Manager* managerPtr = Manager::instance.release();
		Manager::instance = nullptr;
		if (managerPtr != nullptr)
		{
			delete managerPtr;
		}
		managerPtr = nullptr;
	}
}

void Manager::sendError(const ERROR_CODE errorCode)
{
	if (this->errorCallback != nullptr)
	{
		std::string msg;
		switch (errorCode)
		{
		case ECS::ERROR_CODE::ECS_INVALID_POOL_NAME:
			msg = "ECS_ERROR: EntityPool name can't be empty or \"" + Manager::DEFAULT_POOL_NAME + "\".";
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
	if (name.empty() || name == Manager::DEFAULT_POOL_NAME)
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
}

const bool ECS::Manager::deleteEntityPool(const std::string& name)
{
	if (name.empty() || name == Manager::DEFAULT_POOL_NAME)
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


//============================================================================================

EntityPool::EntityPool(const std::string& name, const unsigned int maxSize) 
: maxPoolSize(maxSize)
{
	// Allocate entities
	for (unsigned int i = 0; i < maxSize; i++)
	{
		this->pool.push_back(std::unique_ptr<ECS::Entity, ECS::Deleter<Entity>>(new Entity(), ECS::Deleter<Entity>()));
		this->pool.back()->alive = false;
		this->pool.back()->eId = ++Entity::idCounter;
		this->pool.back()->eIndex = i;
		this->nextIndicies.push(i);
	}
}

EntityPool::~EntityPool()
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

EID Entity::idCounter = 0;

Entity::Entity()
: alive(false)
, eId(++Entity::idCounter)
, eIndex(-1)
{
	this->wrapIdCounter();
}

// Really nothing to call on death of instance.
Entity::~Entity() {}

void ECS::Entity::wrapIdCounter()
{
	if (Entity::idCounter >= 0xffffffff)
	{
		// id Counter reached max number. Wrap to 0. 
		// This means user created more 4294967295 entities, which really doesn't happen all that much.
		Entity::idCounter = 0;
	}
}

void ECS::Entity::revive()
{
	this->alive = true;
	this->eId = ++Entity::idCounter;
	this->wrapIdCounter();
}

void Entity::kill()
{
	// Wipe everything
	this->alive = false;
	this->eId = -1;
}