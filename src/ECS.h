#ifndef ECS_H
#define ECS_H

#include <vector>
#include <list>
#include <unordered_map>
#include <memory>
#include <queue>
#include <functional>

namespace ECS
{
	// Classes
	class EntityPool;
	class Entity;
	class Component;
	class System;
	class Manager;

	// Const
	const unsigned int MAX_ENTITY_SIZE = 2048;			// Maximum entity size

	// Typedefs
	typedef unsigned long EID;							// Entity ID. 
	typedef unsigned int EINDEX;						// Entity Index
	typedef unsigned int CID;							// Component ID
	typedef unsigned int CINDEX;						// Component Index

	// Error code
	enum class ERROR_CODE
	{
		ECS_NO_ERROR,
		ECS_INVALID_POOL_NAME,
		ECS_DUPLICATED_POOL_NAME,
		ECS_POOL_NOT_FOUND,
		ECS_POOL_IS_FULL,
		ECS_ENTITY_NOT_FOUND,
	};

	// Custom deleter for unique_ptr.
	// By making this, user can't call destructor(delete) on any instances.
	template<class T> class Deleter
	{
		friend std::unique_ptr<ECS::Entity, Deleter>;
		friend std::unique_ptr<ECS::EntityPool, Deleter>;
		friend std::unique_ptr<ECS::Manager, Deleter>;
	private:
		void operator()(T* t) { delete t; }
	};	

	/**
	*	@class EntityPool
	*	@brief A simple std::vector wrapper with few rules.
	*	@note This is similar to pool implementation.
	*/
	class EntityPool
	{
	private:
		friend class Manager;
		friend class Deleter<EntityPool>;
	public:
		// Default max size
		static const unsigned int DEFAULT_MAX_POOL_SIZE = 2048;
	private:
		// Name of the pool.
		std::string name;

		// container
		std::vector<std::unique_ptr<ECS::Entity, Deleter<Entity>>> pool;

		// Private constructor. Use manager to create pool
		EntityPool(const std::string& name, const unsigned int maxSize = EntityPool::DEFAULT_MAX_POOL_SIZE);

		// Private constructor. Use manager to delete pool
		~EntityPool();

		// Disable all other constructors.
		EntityPool(const EntityPool& arg) = delete;						// Copy constructor
		EntityPool(const EntityPool&& arg) = delete;					// Move constructor
		EntityPool& operator=(const EntityPool& arg) = delete;			// Assignment operator
		EntityPool& operator=(const EntityPool&& arg) = delete;			// Move operator

		// fresh entity indicies in pool. This keep tracks the left most entity that is ready to be used
		std::queue<EINDEX> nextIndicies;

		// Maximum size of this pool.
		int maxPoolSize;

		// Check if index is valid(in range)
		const bool isValidIndex(const unsigned int index);
	public:
		// Get how many entities are active in this pool
		const unsigned int getEntityCount(const bool onlyAlive = true);
	};

	/**
	*	@class Entity
	*	@brief Entity is simply an pack of numbers.
	*	@note Entity doesn't carries Components.
	*/
	class Entity
	{
	private:
		friend class Manager;
		friend class EntityPool;
		friend class Deleter<Entity>;
	private:
		// Constructor
		Entity();

		// User can't call delete on entity. Must call kill.
		~Entity();

		// Disable all other constructors.
		Entity(const Entity& arg) = delete;						// Copy constructor
		Entity(const Entity&& arg) = delete;							// Move constructor
		Entity& operator=(const Entity& arg) = delete;			// Assignment operator
		Entity& operator=(const Entity&& arg) = delete;			// Move operator
		// ID counter. Starts from 0

		// Id counter
		static EID idCounter;

		// ID of entity. increases till 4294967295 (0xffffffff) then wrapped to 0. 
		EID eId;

		// Index of entity pool for fast access. This is fixed.
		EINDEX eIndex;

		// If entity is only visible if it's alive. Dead entities will not be queried or accessible.
		bool alive;

		// Revive this entity and get ready to use
		void revive();

		// Wrap idcounter
		void wrapIdCounter();
	public:
		// Kill entity. Once this is called, this entity will not be functional anymore.
		void kill();
	};

	/**
	*	@class Manager
	*	@brief The manager class that manages entire ECS.
	*	@note This is singleton class. The instance automtically released on end of program.
	*/
	class Manager
	{
	private:
		friend class Deleter<Manager>;
		// const
		static const std::string DEFAULT_POOL_NAME;

		// Private constructor. Call getInstance for access.
		Manager();
		// Private constructor. Call deleteInstance to delete instance.
		~Manager();
		// Disable all other constructors.
		Manager(const Manager& arg) = delete;						// Copy constructor
		Manager(const Manager&& arg) = delete;						// Move constructor
		Manager& operator=(const Manager& arg) = delete;			// Assignment operator
		Manager& operator=(const Manager&& arg) = delete;			// Move operator

		// Singleton instance
		static std::unique_ptr<Manager, ECS::Deleter<Manager>> instance;

		std::list<std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>> entityPools;

		// Check if there is pool with name
		const bool hasPoolName(const std::string& name);

		// Send error
		void sendError(const ERROR_CODE errorCode);
	public:
		// Get instance.
		static Manager* getInstance();

		// Delete instance.
		static void deleteInstance();

		/**
		*	@name createEntityPool
		*	@brief Creates new entity pool with name.
		*	@note This is for adding additional entity pool than default one which automatically created on initialize.
		*	@param name New pool's string name.
		*	@param maxSize Pool's maximum size. Must be power of 2. Else, it rounds up to nearest power of 2. Set to EntityPool::DEFAULT_MAX_POOL_SIZE by default.
		*	@see EntityPool::DEFAULT_MAX_POOL_SIZE
		*	@return True if successfully creates. Else, false.
		*/
		const bool createEntityPool(const std::string& name, const int maxSize = EntityPool::DEFAULT_MAX_POOL_SIZE);

		/**
		*	@name deleteEntityPool
		*	@brieft Completely removes entity pool from Manager.
		*	@note Can't delete default entity pool. @see clearEntityPool()
		*	@param name A pool' string name to delete.
		*	@return True if successfully deletes pool.
		*/
		const bool deleteEntityPool(const std::string& name);

		/**
		*	@name detachPool
		*	@brief Get pool by name and remove from Manager
		*	@note Once it got deteached, manager no longer manges this EntityPool
		*	@param name A pool's string name to detach.
		*	@return EntityPool if exists. Else, nullptr.
		*/
		EntityPool* detachPool(const std::string& name);

		/**
		*	@name getPool
		*	@brief Get pool by name
		*	@param name A pool's string name to get.
		*	@return EntityPool if exists. Else, nullptr.
		*/
		EntityPool* getPool(const std::string& name);

		/**
		*	@name createEntity
		*	@brief Creates new entity and add to pool.
		*	@note This function will add Entity to pool.
		*	@param poolName A string name of pool to add new Entity
		*	@return Entity if successfully created. Else, nullptr.
		*/
		Entity* createEntity(const std::string& poolName = Manager::DEFAULT_POOL_NAME);

		/**
		*	@name getEntity
		*	@brief Get Entity by entity's Id.
		*	@note Specify the pool name if requried.
		*	@param entityId Entity Id to query.
		*	@param poolName A pool's name to query specifically. Set to Manager::DEFAULT_POOL_NAME by default.
		*	@see Manager::DEFAULT_POOL_NAME
		*	@return Entity if found. Else, nullptr.
		*/
		Entity* getEntityById(const EID entityId, const std::string& poolName = Manager::DEFAULT_POOL_NAME);

		// Error callback. 
		std::function<void(const ERROR_CODE, const std::string&)> errorCallback;
	};
}

#endif