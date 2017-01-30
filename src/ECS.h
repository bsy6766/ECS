#ifndef ECS_H
#define ECS_H

// containers
#include <vector>
#include <list>
#include <queue>
#include <unordered_map>	
// Util
#include <memory>				// unique_ptr
#include <functional>			// Error handling
#include <bitset>				// Signature
#include <limits>
#include <stdexcept>      // std::out_of_range
// Component
#include <typeinfo>
#include <typeindex>
#include <set>

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
	const unsigned int MAX_COMPONENT_PER_ENTITY = 128;	// Maximum number of component that entity can have
	const unsigned int MAX_CID = std::numeric_limits<unsigned int>::max();
	const unsigned int INVALID_CID = MAX_CID;
	const unsigned long MAX_EID = std::numeric_limits<unsigned long>::max();

	// Typedefs
	typedef unsigned long EID;							// Entity ID. 
	typedef unsigned int EINDEX;						// Entity Index
	typedef unsigned int CID;							// Component ID
	typedef unsigned int CINDEX;						// Component Index
	typedef std::bitset<MAX_COMPONENT_PER_ENTITY> Signature;


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

	template<class T> class ComponentDeleter
	{
		friend std::unique_ptr<ECS::Component, ComponentDeleter>;
	private:
		void operator()(Component* c)
		{
			if (T* t = dynamic_cast<T*>(c))
			{
				delete t;
			}
		}
	};

	class Component
	{
		friend class Manager;
	protected:
		Component();

		// ID counter. Starts from 0
		static CID idCounter;

		// Component Id
		CID id;

		void wrapIdCounter();
	public:
		virtual ~Component() = default;
	public:
		// Disable all other constructors.
		Component(const Component& arg) = delete;								// Copy constructor
		Component(const Component&& arg) = delete;								// Move constructor
		Component& operator=(const Component& arg) = delete;					// Assignment operator
		Component& operator=(const Component&& arg) = delete;					// Move operator

		const CID getId();
		virtual void update(const float delta) = 0;
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

		// Entity Pools
		std::list<std::unique_ptr<ECS::EntityPool, ECS::Deleter<EntityPool>>> entityPools;

		// Component ID Map. type_index <---> CID
		std::unordered_map<std::type_index, CID> CIDMap;
		// Components
		std::vector</*CID*/std::vector<std::unique_ptr<Component, ComponentDeleter<Component>>>> components;

		// Check if there is pool with name
		const bool hasPoolName(const std::string& name);

		// Send error
        void sendError(const ERROR_CODE errorCode);
        
        // get CID from type info
        const CID getCIDFromTypeInfo(const std::type_info& t);
        
        // Check if entity has component
        const bool hasComponent(Entity* e, const std::type_info& t);
        
        // Get component.
        Component* getComponent(Entity* e, const std::type_info& t);
        
        // Get components
        std::vector<Component*> getComponents(Entity* e, const std::type_info& t);
        
        // Add component to entity
        const bool addComponent(Entity* e, const std::type_info& t, Component* c);
        
        // Remove compoent from entity
        const bool removeComponents(Entity* e, const std::type_info& t);
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
        
        template<class T>
        const bool hasComponent(Entity* e)
        {
            return hasComponent(e, typeid(T));
        }
        
        template<class T>
        T* getComponent(Entity* e)
        {
            return dynamic_cast<T*>(getComponent(e, typeid(T)));
        }
        
        template<class T>
        std::vector<T*> getComponents(Entity* e)
        {
            std::vector<T*> ret;
            std::vector<Component*> component = getComponents(e, typeid(T));
            for(auto c : components)
            {
                ret.push_back(dynamic_cast<T*>(c));
            }
            
            return ret;
        }
        
        template<class T>
        const bool addComponent(Entity* e)
        {
            return addComponent(e, typeid(T), new T());
        }
        
        template<class T>
        const bool addComponent(Entity* e, Component* c)
        {
            return addComponent(e, typeid(T), c);
        }
        
        template<class T>
        const bool removeComponents(Entity* e)
        {
            return removeComponents(e, typeid(T));
        }
        
        /**
         *  @name hasComponent
         *  @brief Check if the entity has component
         *  @
         */

		// Error callback. 
		std::function<void(const ERROR_CODE, const std::string&)> errorCallback;
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
        ~Entity() = default;
        
        // Disable all other constructors.
        Entity(const Entity& arg) = delete;								// Copy constructor
        Entity(const Entity&& arg) = delete;							// Move constructor
        Entity& operator=(const Entity& arg) = delete;					// Assignment operator
        Entity& operator=(const Entity&& arg) = delete;					// Move operator
        
        // Signature.
        Signature signature;
        
        // Component Index map
        std::unordered_map<CID, std::set<CINDEX>> componentIndicies;
        
        // ID counter. Starts from 0
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
        
        // get CINDEX by CID
        void getCINDEXByCID(const CID cId, std::set<CINDEX>& cIndicies);
        
        const bool addCINDEXToCID(const CID cId, const CINDEX cIndex);
    public:
        // Kill entity. Once this is called, this entity will not be functional anymore.
        void kill();
        
        // Get entity Id
        const EID getEntityId();
        
        // Check if this entity is alive
        const bool isAlive();
        
        // Check if Entity has Component
        template<class T>
        const bool hasComponent()
        {
            Manager* m = Manager::getInstance();
            return m->hasComponent<T>(this);
        }
        
        template<class T>
        T* getComponent()
        {
            Manager* m = Manager::getInstance();
            return m->getComponents<T>(this);
        }
        
        template<class T>
        std::vector<T*> getComponents(Entity* e)
        {
            Manager* m = Manager::getInstance();
            return m->getComponents<T>(this);
        }
        
        template<class T>
        const bool addComponent(Entity* e)
        {
            Manager* m = Manager::getInstance();
            return m->addComponent<T>(this);
        }
        
        template<class T>
        const bool addComponent(Entity* e, Component* c)
        {
            Manager* m = Manager::getInstance();
            return m->addComponent<T>(this, c);
        }
        
        template<class T>
        const bool removeComponents(Entity* e)
        {
            Manager* m = Manager::getInstance();
            return m->removeComponents<T>(this);
        }
    };
}

#endif
