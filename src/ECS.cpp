#include "ECS.h"
#include <cassert>

using namespace ECS;

std::unique_ptr<Manager, ECS::Deleter<Manager>> ECS::Manager::instance = nullptr;
const std::string ECS::Manager::DEFAULT_POOL_NAME = "DEFAULT";

ECS::Manager::Manager()
{
	// Create default entity pool
	this->entityPools.insert(std::pair<std::string, std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>>(ECS::Manager::DEFAULT_POOL_NAME, std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>(new EntityPool(ECS::Manager::DEFAULT_POOL_NAME), ECS::Deleter<EntityPool>())));
}

ECS::Manager::~Manager()
{
	// Unique ptr will release all for us
	this->clear();
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
	return this->entityPools.find(name) != this->entityPools.end();
}

EntityPool* ECS::Manager::createEntityPool(const std::string& name, const int maxSize)
{
	if (name.empty() || name == ECS::Manager::DEFAULT_POOL_NAME)
	{
		// Pool name can't be empty or default
		this->sendError(ERROR_CODE::ECS_INVALID_POOL_NAME);
		return nullptr;
	}

	if (this->hasPoolName(name))
	{
		// There is a pool with same name
		this->sendError(ERROR_CODE::ECS_DUPLICATED_POOL_NAME);
		return nullptr;
	}

	// Create new
	this->entityPools.insert(std::pair<std::string, std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>>(name, std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>(new EntityPool(name, maxSize), ECS::Deleter<EntityPool>())));
	return this->entityPools[name].get();
}

const bool ECS::Manager::addEntityPool(ECS::EntityPool* entityPool)
{
	if (entityPool == nullptr)
	{
		return false;
	}

	const std::string poolName = entityPool->getName();
	if (this->hasPoolName(poolName))
	{
		return false;
	}

	this->entityPools.insert(std::pair<std::string, std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>>(poolName, std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>(entityPool, ECS::Deleter<EntityPool>())));
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

	EntityPool* pool = this->detachEntityPool(name);
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

EntityPool* ECS::Manager::detachEntityPool(const std::string & name)
{
	if (name.empty())
	{
		return nullptr;
	}

	if (name == ECS::Manager::DEFAULT_POOL_NAME)
	{
		return nullptr;
	}

	auto find_it = this->entityPools.find(name);
	if (find_it == this->entityPools.end())
	{
		return nullptr;
	}
	else
	{
		EntityPool* poolPtr = find_it->second.release();
		this->entityPools.erase(find_it);
		return poolPtr;
	}
}

EntityPool* ECS::Manager::getEntityPool(const std::string& name)
{
	if (name.empty())
	{
		return nullptr;
	}

	auto find_it = this->entityPools.find(name);
	if (find_it == this->entityPools.end())
	{
		return nullptr;
	}
	else
	{
		return find_it->second.get();
	}
}

Entity* ECS::Manager::createEntity(const std::string& poolName)
{
	if (poolName.empty())
	{
		// Pool name can't be empty
		this->sendError(ERROR_CODE::ECS_INVALID_POOL_NAME);
		return nullptr;
	}

	auto find_it = this->entityPools.find(poolName);

	if (find_it == this->entityPools.end())
	{
		this->sendError(ERROR_CODE::ECS_POOL_NOT_FOUND);
		return nullptr;
	}

	if (find_it->second->nextIndicies.empty())
	{
		// queue is empty. Pool is full
		this->sendError(ERROR_CODE::ECS_POOL_IS_FULL);
		return nullptr;
	}
	else
	{
		unsigned int index = find_it->second->nextIndicies.front();
		find_it->second->nextIndicies.pop_front();

		// Assume index is always valid.
		find_it->second->pool.at(index)->revive();
		return find_it->second->pool.at(index).get();
	}

	// failed to find pool
	this->sendError(ERROR_CODE::ECS_POOL_NOT_FOUND);
	return nullptr;
}

Entity* ECS::Manager::getEntityById(const EID entityId)
{
	if (entityId == -1)
	{
		this->sendError(ERROR_CODE::ECS_INVALID_ENTITY_ID);
		return nullptr;
	}

	// Iterate all EntityPool and find it
	for (auto& entityPool : this->entityPools)
	{
		for (auto& entity : entityPool.second->pool)
		{
			if (entity->eId == entityId)
			{
				return entity.get();
			}
		}
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
		// This CID exists. Check if entity has it
		//try
		//{
		//	e->signature.test(cId);
		//}
	}
	else
	{
        // This CID doesn't exists yet.
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

			try
			{
				e->signature.test(cId);
			}
			catch (...)
			{

			}
		}
		catch (...)
		{
			return false;
		}
	}
	else
	{
		// This component never had added. 
		// Add as new component type
		c->id = ++Component::idCounter;
		Component::wrapIdCounter();

		// Add to CID map
		this->CIDMap.insert(std::pair<std::type_index, CID>(std::type_index(t), cId));

		// Add component
		this->components.push_back(std::vector<std::unique_ptr<Component, ComponentDeleter<Component>>>());
		this->components.back().push_back(std::unique_ptr<Component, ComponentDeleter<Component>>(c, ECS::ComponentDeleter<Component>()));
	}
	return true;
}

const bool ECS::Manager::removeComponents(Entity * e, const std::type_info & t)
{
	return false;
}

void ECS::Manager::clear()
{
	this->entityPools.clear();
	this->entityPools.insert(std::pair<std::string, std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>>(ECS::Manager::DEFAULT_POOL_NAME, std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>(new EntityPool(ECS::Manager::DEFAULT_POOL_NAME), ECS::Deleter<EntityPool>())));

	this->components.clear();
	this->CIDMap.clear();

	Component::idCounter = 0;
	Entity::idCounter = 0;
}

//============================================================================================

ECS::EntityPool::EntityPool(const std::string& name, const unsigned int size) 
: poolSize(size), name(name)
{
	if (this->isPowerOfTwo(this->poolSize) == false)
	{
		this->roundToNearestPowerOfTwo(this->poolSize);
	}

	// Allocate entities
	for (unsigned int i = 0; i < this->poolSize; i++)
	{
		this->pool.push_back(std::unique_ptr<ECS::Entity, ECS::Deleter<Entity>>(new Entity(), ECS::Deleter<Entity>()));
		this->nextIndicies.push_back(i);
		this->pool.back()->eIndex = i;
		this->pool.back()->entityPoolName = name;
	}
}

ECS::EntityPool::~EntityPool()
{
	// unique ptr will release for us
	pool.clear();
	this->poolSize = 0;
}

const bool ECS::EntityPool::isValidIndex(const unsigned int index)
{
	return (index >= 0) && (index < this->pool.size());
}

const bool ECS::EntityPool::isPowerOfTwo(const unsigned int n)
{
	return ((n != 0) && !(n & (n - 1)));
}

void ECS::EntityPool::roundToNearestPowerOfTwo(unsigned int& n)
{
	n = pow(2, ceil(log(n) / log(2)));
}

const unsigned int ECS::EntityPool::getAliveEntityCount()
{
	unsigned int count = 0;
	for (auto& entity : this->pool)
	{
		if (entity->alive)
		{
			count++;
		}
	}

	return count;
}

const unsigned int ECS::EntityPool::getPoolSize()
{
	return this->pool.size();
}

const std::string ECS::EntityPool::getName()
{
	return this->name;
}

const bool ECS::EntityPool::resize(const unsigned int size)
{
	unsigned int newSize = size;
	if (this->isPowerOfTwo(newSize))
	{
		this->roundToNearestPowerOfTwo(newSize);
	}

	const unsigned int currentSize = this->getPoolSize();

	if (currentSize == newSize)
	{
		// New size is same as current size.
		return true;
	}

	this->poolSize = newSize;

	if (currentSize < newSize)
	{
		// Increase size
		for (unsigned int i = currentSize; i < newSize; i++)
		{
			this->pool.push_back(std::unique_ptr<ECS::Entity, ECS::Deleter<Entity>>(new Entity(), ECS::Deleter<ECS::Entity>()));
			this->nextIndicies.push_back(i);
		}

		assert(this->poolSize == this->pool.size());
	}
	else
	{
		// Decrease size
		this->pool.resize(this->poolSize);

		auto it = this->nextIndicies.begin();
		for (; it != this->nextIndicies.end();)
		{
			if (this->poolSize <= (*it) && (*it) <= newSize)
			{
				it = this->nextIndicies.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

	return true;
}

void ECS::EntityPool::reset()
{
	this->pool.clear();
	this->nextIndicies.clear();
}

ECS::Entity* ECS::EntityPool::getEntityById(const EID eId)
{
	for (auto& entity : this->pool)
	{
		if (entity->eId == eId)
		{
			return entity.get();
		}
	}

	return nullptr;
}

//============================================================================================

EID ECS::Entity::idCounter = 0;

ECS::Entity::Entity()
: alive(false)
, sleep(false)
, eId(-1)
, eIndex(-1)
, signature(0)
, entityPoolName(std::string())
{}

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
	this->sleep = false;
	this->eId = Entity::idCounter++;
	this->wrapIdCounter();
}

void ECS::Entity::kill()
{
	// Add index to deque
	ECS::Manager* m = Manager::getInstance();
	ECS::EntityPool* ePool = m->getEntityPool(this->entityPoolName);
	if (ePool != nullptr)
	{
		ePool->nextIndicies.push_front(this->eIndex);
	}

	// Wipe everything
	this->alive = false;
	this->sleep = false;
	this->eId = -1;
	this->componentIndicies.clear();
}

const EID ECS::Entity::getId()
{
	return this->eId;
}

const std::string ECS::Entity::getEntityPoolName()
{
	return this->entityPoolName;
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
