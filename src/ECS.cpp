#include "ECS.h"
#include <cassert>

using namespace ECS;

std::unique_ptr<Manager, ECS::Deleter<Manager>> ECS::Manager::instance = nullptr;

ECS::Manager::Manager()
{
	// Create default entity pool
	this->entityPools.insert(std::pair<std::string, std::unique_ptr<ECS::EntityPool, ECS::Deleter<ECS::EntityPool>>>(ECS::DEFAULT_ENTITY_POOL_NAME, std::unique_ptr<ECS::EntityPool, ECS::Deleter<ECS::EntityPool>>(new ECS::EntityPool(ECS::DEFAULT_ENTITY_POOL_NAME, ECS::DEFAULT_ENTITY_POOL_SIZE), ECS::Deleter<ECS::EntityPool>())));
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
		instance->clear();
		Manager* managerPtr = ECS::Manager::instance.release();
		ECS::Manager::instance = nullptr;
		if (managerPtr != nullptr)
		{
			delete managerPtr;
		}
		managerPtr = nullptr;
	}
}

bool ECS::Manager::isValid()
{
	return ECS::Manager::instance != nullptr;
}

void ECS::Manager::update(const float delta)
{
}

void ECS::Manager::sendError(const ERROR_CODE errorCode)
{
	if (this->errorCallback != nullptr)
	{
		std::string msg;
		switch (errorCode)
		{
		case ECS::ERROR_CODE::ECS_INVALID_POOL_NAME:
			msg = "ECS_ERROR: EntityPool name can't be empty or \"" + DEFAULT_ENTITY_POOL_NAME + "\".";
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

void ECS::Manager::wrapIdCounter(const C_UNIQUE_ID cUniqueId)
{
	try
	{
		if (this->componentIdCounter.at(cUniqueId) >= ECS::INVALID_C_UNIQUE_ID)
		{
			this->componentIdCounter.at(cUniqueId) = 0;
		}
	}
	catch (...)
	{
		return;
	}
}

const bool ECS::Manager::hasPoolName(const std::string & name)
{
	// Check if there is a pool with same name.
	return this->entityPools.find(name) != this->entityPools.end();
}

ECS::EntityPool* ECS::Manager::createEntityPool(const std::string& name, const int maxSize)
{
	if (name.empty() || name == DEFAULT_ENTITY_POOL_NAME)
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
	this->entityPools.insert(std::pair<std::string, std::unique_ptr<ECS::EntityPool, ECS::Deleter<ECS::EntityPool>>>(name, std::unique_ptr<ECS::EntityPool, ECS::Deleter<ECS::EntityPool>>(new ECS::EntityPool(name, maxSize), ECS::Deleter<ECS::EntityPool>())));
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

	this->entityPools.insert(std::pair<std::string, std::unique_ptr<ECS::EntityPool, ECS::Deleter<ECS::EntityPool>>>(poolName, std::unique_ptr<ECS::EntityPool, ECS::Deleter<ECS::EntityPool>>(entityPool, ECS::Deleter<ECS::EntityPool>())));
	return true;
}

const bool ECS::Manager::deleteEntityPool(const std::string& name)
{
	if (name.empty() || name == DEFAULT_ENTITY_POOL_NAME)
	{
		// Can't delete with empty pool name or default name
		this->sendError(ERROR_CODE::ECS_DUPLICATED_POOL_NAME);
		return false;
	}

	ECS::EntityPool* pool = this->detachEntityPool(name);
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

ECS::EntityPool* ECS::Manager::detachEntityPool(const std::string & name)
{
	if (name.empty())
	{
		return nullptr;
	}

	if (name == DEFAULT_ENTITY_POOL_NAME)
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
		ECS::EntityPool* poolPtr = find_it->second.release();
		this->entityPools.erase(find_it);
		return poolPtr;
	}
}

ECS::EntityPool* ECS::Manager::getEntityPool(const std::string& name)
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

Entity* ECS::Manager::getEntityById(const E_ID entityId)
{
	if (entityId == ECS::INVALID_E_ID)
	{
		this->sendError(ERROR_CODE::ECS_INVALID_ENTITY_ID);
		return nullptr;
	}

	// Iterate all EntityPool and find it
	for (auto& entityPool : this->entityPools)
	{
		for (auto& entity : entityPool.second->pool)
		{
			if (entity->id == entityId)
			{
				return entity.get();
			}
		}
	}

	// Entity not found
	this->sendError(ERROR_CODE::ECS_ENTITY_NOT_FOUND);
	return nullptr;
}

const C_ID ECS::Manager::getComponentUniqueId(const std::type_info& t)
{
	const std::type_index typeIndex = std::type_index(t);
	auto find_it = this->C_UNIQUE_IDMap.find(typeIndex);
	if (find_it != this->C_UNIQUE_IDMap.end())
	{
		return find_it->second;
	}
	else
	{
		return INVALID_C_UNIQUE_ID;
	}
}

const C_UNIQUE_ID ECS::Manager::registerComponent(const std::type_info& t)
{
	C_UNIQUE_ID cUniqueID = this->getComponentUniqueId(t);
	if (cUniqueID == ECS::INVALID_C_UNIQUE_ID)
	{
		// This is new type of component.
		cUniqueID = Component::uniqueIdCounter++;
		if (cUniqueID == ECS::MAX_C_UNIQUE_ID)
		{
			// Reached maximum number of component type. 
			Component::uniqueIdCounter--;
			return ECS::INVALID_C_UNIQUE_ID;
		}

		// Add new unique id with type_index key to map
		this->C_UNIQUE_IDMap.insert(std::pair<std::type_index, C_UNIQUE_ID>(std::type_index(t), cUniqueID));

		// Add new component pool
		this->components.push_back(std::unique_ptr<ECS::ComponentPool, ECS::Deleter<ECS::ComponentPool>>(new ECS::ComponentPool(t.name(), ECS::MAX_C_INDEX)));
		
		// Add new counter to this componet type
		this->componentIdCounter.push_back(0);
	}

	return cUniqueID;
}

const bool ECS::Manager::deleteComponent(Component*& c, const std::type_info& t)
{

	// Check component
	if (c == nullptr)
	{
		return false;
	}

	if (c->uniqueId == ECS::INVALID_C_UNIQUE_ID)
	{
		// Unique id is invalid
		delete c;
		return true;
	}
	else
	{
		if (c->ownerId == ECS::INVALID_E_ID)
		{
			// Doesn't have owner
			delete c;
			return true;
		}
		else
		{
			// Get owner
			ECS::Entity* e = this->getEntityById(c->ownerId);
			if (e == nullptr)
			{
				// Owner doesn't exists
				delete c;
				return true;
			}
			else
			{
				// have owner. Check if manager has this component
				C_INDEX index = ECS::INVALID_C_INDEX;
				for (auto& component : this->components[c->uniqueId]->pool)
				{
					if (c == component.get())
					{
						// save index
						index = c->index;

						// Remove from signature
						e->signature[c->uniqueId] = 0;
						// Remove from indcies vector
						e->componentIndicies[c->uniqueId].erase(index);
						
						// Delete the component
						delete component.release();

						// Make it null
						component = nullptr;

						return true;
					}
				}

				if (index == ECS::INVALID_C_INDEX)
				{
					// Failed to find same component
					delete c;
					c = nullptr;
					return true;
				}

				return true;
			}
		}
	}
}

const bool ECS::Manager::hasComponent(Entity* e, const std::type_info& t)
{
	if (e == nullptr) return false;

	const ECS::C_UNIQUE_ID cUniqueId = this->getComponentUniqueId(t);

	if (cUniqueId == ECS::INVALID_C_UNIQUE_ID)
	{
		// This type of component is unknown.
		return false;
	}
	else
	{
		Signature& s = e->signature;

		try
		{
			s.test(cUniqueId);
			if (s[cUniqueId] == true)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
	}
}

const bool ECS::Manager::hasComponent(Entity* e, const std::type_info& t, Component* c)
{
	if (e == nullptr) return false;
	if (c == nullptr) return false;
	if (c->id == ECS::INVALID_C_ID) return false;
	if (c->index == ECS::INVALID_C_INDEX) return false;
	if (c->uniqueId != this->getComponentUniqueId(t)) return false;

	if (hasComponent(e, t))
	{
		for (auto cIndex : e->componentIndicies[this->getComponentUniqueId(t)])
		{
			if (c->index == cIndex)
			{
				return true;
			}
		}

		return false;
	}
	else
	{
		return false;
	}
}

Component* ECS::Manager::getComponent(Entity* e, const std::type_info& t)
{
	if (e == nullptr) return false;

	// Get unique id
	const ECS::C_UNIQUE_ID cUniqueId = this->getComponentUniqueId(t);
	if (cUniqueId == ECS::INVALID_C_UNIQUE_ID)
	{
		// This type of component is unknown.
		return nullptr;
	}
	else
	{
		// This type of component is already known. We have unique id and pool ready.
		std::set<C_INDEX> cIndicies;
		// Get C_INDEXs
		e->getComponentIndiicesByUniqueId(cUniqueId, cIndicies);

		if (cIndicies.empty())
		{
			// Doesn't have any. hmm..
			return nullptr;
		}
		else
		{
			// Because this is getComponent not getComponents, return first one
			const C_INDEX firstIndex = (*cIndicies.begin());
			return this->components[cUniqueId]->pool.at(firstIndex).get();
		}
	}
}

std::vector<Component*> ECS::Manager::getComponents(Entity* e, const std::type_info& t)
{
	std::vector<Component*> ret;

	if (e == nullptr) return ret;

	const ECS::C_UNIQUE_ID cUniqueId = this->getComponentUniqueId(t);
	if (cUniqueId == ECS::INVALID_C_UNIQUE_ID)
	{
		// This type of component is unknown.
		return ret;
	}
	else
	{
		// This type of component is already known. We have unique id and pool ready.
		std::set<C_INDEX> cIndicies;
		// Get C_INDEXs
		e->getComponentIndiicesByUniqueId(cUniqueId, cIndicies);

		if (cIndicies.empty())
		{
			// Doesn't have any. hmm..
			return ret;
		}
		else
		{
			// Return all components
			for (auto& component : this->components.at(cUniqueId)->pool)
			{
				if (component != nullptr)
				{
					ret.push_back(component.get());
				}
			}

			return ret;
		}
	}
}

const bool ECS::Manager::addComponent(Entity* e, const std::type_info& t, Component* c)
{
	if (e == nullptr) return false;

	// Reject null component
	if (c == nullptr) return false;

	// Get component unique id
	C_UNIQUE_ID cUniqueId = c->uniqueId;
	if (cUniqueId == ECS::INVALID_C_UNIQUE_ID)
	{
		// Bad component type
		return false;
	}

	// Check if manager already has the same entity. Reject if there is same one
	for (auto& component : this->components.at(cUniqueId)->pool)
	{
		if (component != nullptr)
		{
			if (component->getId() == c->getId())
			{
				return false;
			}
		}
	}

	// Component is valid.
	// Add component
	this->components.at(cUniqueId)->pool.push_back(std::unique_ptr<Component, Deleter<Component>>(c, Deleter<Component>()));

	// Get index. pool size - 1
	const C_INDEX cIndex = this->components.at(cUniqueId)->pool.size() - 1;

	// Add index to entity
	e->componentIndicies[cUniqueId].insert(cIndex);

	// update signature
	e->signature[cUniqueId] = 1;

	// Update component indicies
	auto find_it = e->componentIndicies.find(cUniqueId);
	if (find_it == e->componentIndicies.end())
	{
		e->componentIndicies.insert(std::pair<C_UNIQUE_ID, std::set<C_INDEX>>(cUniqueId, std::set<C_INDEX>()));
	}
	e->componentIndicies[cUniqueId].insert(cIndex);

	// Assign unique id
	c->uniqueId = cUniqueId;
	// Assign id
	c->id = this->componentIdCounter.at(cUniqueId)++;
	// Store index
	c->index = cIndex;
	// Set owner id
	c->ownerId = e->getId();
	// wrap counter 
	this->wrapIdCounter(cUniqueId);

	return true;
}

const bool ECS::Manager::removeComponent(Entity* e, const std::type_info& t, const C_ID componentId)
{
	if (e == nullptr) return false;

	// Reject null component
	if (componentId == ECS::INVALID_C_ID) return false;

	// Get component unique id
	C_UNIQUE_ID cUniqueId = this->getComponentUniqueId(t);
	if (cUniqueId == ECS::INVALID_C_UNIQUE_ID)
	{
		// Bad component type
		return false;
	}

	// Find component
	unsigned int index = 0;
	for (auto& component : this->components[cUniqueId]->pool)
	{
		if (component == nullptr)
		{
			continue;
		}

		if (component->id == componentId)
		{
			// Found the component. 
			if (component->ownerId == e->id)
			{
				// Owner match
				Component* c = component.release();
				component = nullptr;
				this->components[cUniqueId]->nextIndicies.push_back(index);
				delete c;
				e->componentIndicies.at(cUniqueId).erase(index);

				if (e->componentIndicies.at(cUniqueId).empty())
				{
					e->signature[cUniqueId] = 0;
				}

				return true;
			}
			else
			{
				return false;
			}
		}

		index++;
	}

	return false;
}

const bool ECS::Manager::removeComponent(Entity* e, const std::type_info& t, Component* c)
{
	if (e == nullptr) return false;

	if (c == nullptr) return false;
	if (c->id == ECS::INVALID_C_ID) return false;
	if (c->index == ECS::INVALID_C_INDEX) return false;
	if (c->uniqueId != this->getComponentUniqueId(t)) return false;
	if (c->ownerId != e->id) return false;

	// Get component unique id
	C_UNIQUE_ID cUniqueId = c->uniqueId;

	if (e->signature[cUniqueId] == 0)
	{
		return false;
	}

	if (this->components[cUniqueId]->pool.at(c->index) != nullptr)
	{
		auto comp = this->components[cUniqueId]->pool.at(c->index).release();
		this->components[cUniqueId]->pool.at(c->index) = nullptr;
		delete comp;
		this->components[cUniqueId]->nextIndicies.push_back(c->index);
		e->componentIndicies.at(cUniqueId).erase(c->index);

		if (e->componentIndicies.at(cUniqueId).empty())
		{
			e->signature[cUniqueId] = 0;
		}
	}
	else
	{
		return false;
	}

	return true;
}

const bool ECS::Manager::removeComponents(Entity* e, const std::type_info& t)
{
	if (e == nullptr) return false;

	// Get component unique id
	C_UNIQUE_ID cUniqueId = this->getComponentUniqueId(t);
	if (cUniqueId == ECS::INVALID_C_UNIQUE_ID)
	{
		// Bad component type
		return false;
	}

	if (e->signature[cUniqueId] == 0)
	{
		return false;
	}

	std::set<C_INDEX> cIndicies;
	e->getComponentIndiicesByUniqueId(cUniqueId, cIndicies);

	if (cIndicies.empty())
	{
		return false;
	}
	else
	{
		for (C_INDEX index : cIndicies)
		{
			if (this->components[cUniqueId]->pool.at(index) != nullptr)
			{
				Component* c = this->components[cUniqueId]->pool.at(index).release();
				this->components[cUniqueId]->pool.at(index) = nullptr;
				delete c;
			}
		}

		e->signature[cUniqueId] = 0;
		e->componentIndicies.erase(cUniqueId);
	}

	return true;
}

void ECS::Manager::clear()
{
	ECS::EntityPool* defaultPool = this->entityPools[ECS::DEFAULT_ENTITY_POOL_NAME].release();
	this->entityPools.clear();

	defaultPool->nextIndicies.clear();

	const unsigned int size = defaultPool->pool.size();

	for (unsigned int i = 0; i < size; i++)
	{
		defaultPool->pool.at(i)->id = ECS::INVALID_E_ID;
		defaultPool->pool.at(i)->componentIndicies.clear();
		defaultPool->pool.at(i)->signature = 0;
		defaultPool->pool.at(i)->index = i;
		defaultPool->nextIndicies.push_back(i);
	}

	this->entityPools.insert(std::pair<std::string, std::unique_ptr<ECS::EntityPool, ECS::Deleter<ECS::EntityPool>>>(ECS::DEFAULT_ENTITY_POOL_NAME, defaultPool));

	//this->entityPools.insert(std::pair<std::string, std::unique_ptr<ECS::EntityPool, ECS::Deleter<ECS::EntityPool>>>(ECS::DEFAULT_ENTITY_POOL_NAME, std::unique_ptr<ECS::EntityPool, ECS::Deleter<ECS::EntityPool>>(new ECS::EntityPool(ECS::DEFAULT_ENTITY_POOL_NAME, ECS::DEFAULT_ENTITY_POOL_SIZE), ECS::Deleter<ECS::EntityPool>())));

	this->components.clear();
	this->componentIdCounter.clear();
	this->C_UNIQUE_IDMap.clear();

	Component::uniqueIdCounter = 0;
	Entity::idCounter = 0;
}

void ECS::Manager::printComponentsInfo()
{
	std::cout << std::endl;
	std::cout << "ECS::Printing Components informations" << std::endl;
	std::cout << "Total Component types: " << this->C_UNIQUE_IDMap.size() << std::endl;
	std::cout << "Types -------------------------------" << std::endl;

	for (auto type : this->C_UNIQUE_IDMap)
	{
		const unsigned int size = this->components.at(type.second)->pool.size();
		std::cout << "Name: " << type.first.name() << ", Unique ID: " << type.second << ", Count/Size: " << this->components.at(type.second)->count() << "/" << size << std::endl;
		std::cout << "-- Component details. Unique ID: " << type.second << " --" << std::endl;
		for (unsigned int i = 0; i < size; i++)
		{
			if (this->components.at(type.second)->pool.at(i) == nullptr)
			{
				std::cout << "__EMPTY__" << std::endl;
			}
			else
			{
				std::cout << "ID: " << this->components.at(type.second)->pool.at(i)->getId() << ", Owner ID = " << this->components.at(type.second)->pool.at(i)->ownerId << std::endl;
			}
		}
		std::cout << "-------------------------------------" << std::endl;
	}

	std::cout << "-------------------------------------" << std::endl;
	std::cout << std::endl;
}

//============================================================================================

ECS::EntityPool::EntityPool(const std::string& name, const unsigned int size) : Pool(name, size)
{
	// Allocate entities
	for (unsigned int i = 0; i < this->poolSize; i++)
	{
		this->pool.push_back(std::unique_ptr<ECS::Entity, ECS::Deleter<Entity>>(new Entity(), ECS::Deleter<Entity>()));
		this->nextIndicies.push_back(i);
		this->pool.back()->index = i;
		this->pool.back()->entityPoolName = name;
	}
}

ECS::EntityPool::~EntityPool()
{
	this->clear();
}

void ECS::EntityPool::clear()
{
	this->pool.clear();
	this->nextIndicies.clear();
}

const int ECS::EntityPool::countAliveEntity()
{
	int count = 0;
	for (auto& entity : pool)
	{
		if (entity->alive)
		{
			count++;
		}
	}

	return count;
}

ECS::Entity* ECS::EntityPool::getEntityById(const E_ID eId)
{
	for (auto& entity : pool)
	{
		if (entity->id == eId)
		{
			return entity.get();
		}
	}

	return nullptr;
}



ECS::ComponentPool::ComponentPool(const std::string& name, const unsigned int size) : Pool(name, size) 
{

}

ECS::ComponentPool::~ComponentPool()
{
	this->clear();
}

void ECS::ComponentPool::clear()
{
	this->pool.clear();
	this->nextIndicies.clear();
}

const unsigned int ECS::ComponentPool::count()
{
	int count = 0;
	for (auto& component : this->pool)
	{
		if (component != nullptr)
		{
			count++;
		}
	}

	return count;
}




ECS::Pool::Pool(const std::string& name, const unsigned int size) 
: poolSize(size), name(name)
{
	if (this->isPowerOfTwo(this->poolSize) == false)
	{
		this->roundToNearestPowerOfTwo(this->poolSize);
	}
}

ECS::Pool::~Pool()
{
	// unique ptr will release for us
	this->poolSize = 0;
}

const bool ECS::Pool::isValidIndex(const unsigned int index)
{
	return (index >= 0) && (index < this->poolSize);
}

const bool ECS::Pool::isPowerOfTwo(const unsigned int n)
{
	return ((n != 0) && !(n & (n - 1)));
}

void ECS::Pool::roundToNearestPowerOfTwo(unsigned int& n)
{
	n = pow(2, ceil(log(n) / log(2)));
}

const std::string ECS::Pool::getName()
{
	return this->name;
}

const unsigned int ECS::Pool::getPoolSize()
{
	return this->poolSize;
}

/*
const bool ECS::Pool::resize(const unsigned int size)
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
*/

//============================================================================================

E_ID ECS::Entity::idCounter = 0;

ECS::Entity::Entity()
: alive(false)
, sleep(false)
, id(ECS::INVALID_E_ID)
, index(-1)
, signature(0)
, entityPoolName(std::string())
{}

void ECS::Entity::wrapIdCounter()
{
	if (ECS::Entity::idCounter >= ECS::MAX_E_ID)
	{
		// id Counter reached max number. Wrap to 0. 
		// This means user created more 4294967295 entities, which really doesn't happen all that much.
		ECS::Entity::idCounter = 0;
	}
}

void ECS::Entity::getComponentIndiicesByUniqueId(const C_UNIQUE_ID cUniqueId, std::set<C_INDEX>& cIndicies)
{
	try
	{
		for (auto index : this->componentIndicies.at(cUniqueId))
		{
			cIndicies.insert(index);
		}
	}
	catch (...)
	{
		return;
	}
}

void ECS::Entity::revive()
{
	this->alive = true;
	this->sleep = false;
	this->id = Entity::idCounter++;
	this->wrapIdCounter();
}

void ECS::Entity::kill()
{
	// Add index to deque
	ECS::Manager* m = Manager::getInstance();
	ECS::EntityPool* ePool = m->getEntityPool(this->entityPoolName);
	if (ePool != nullptr)
	{
		ePool->nextIndicies.push_front(this->index);
	}

	// Wipe everything
	this->alive = false;
	this->sleep = false;
	this->id = -1;
	this->componentIndicies.clear();
}

const E_ID ECS::Entity::getId()
{
	return this->id;
}

const std::string ECS::Entity::getEntityPoolName()
{
	return this->entityPoolName;
}

const bool ECS::Entity::isAlive()
{
	return this->alive;
}

const Signature ECS::Entity::getSignature()
{
	return this->signature;
}

//============================================================================================

C_ID ECS::Component::uniqueIdCounter = 0;

Component::Component() 
: id(INVALID_C_ID) 
, index(INVALID_C_INDEX)
, uniqueId(INVALID_C_UNIQUE_ID)
, ownerId(INVALID_E_ID)
{}

void ECS::Component::wrapUniqueIdCounter()
{
	if (Component::uniqueIdCounter >= INVALID_C_UNIQUE_ID)
	{
		Component::uniqueIdCounter = 0;
	}
}

const C_ID ECS::Component::getId()
{
	return this->id;
}

const C_UNIQUE_ID ECS::Component::getUniqueId()
{
	return this->uniqueId;
}

const E_ID ECS::Component::getOwnerId()
{
	return this->ownerId;
}
