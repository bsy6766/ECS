#include <memory>
#include <iostream>
#include "src/ECS.h"
#include <typeinfo>
#include <typeindex>
#include <limits>
#include <cassert>


int main()
{
	// Get Manager.
	ECS::Manager* m = nullptr;

	// Getting manager class
	{
		// Just call getInstance
		m = ECS::Manager::getInstance();
		// Calling this first time will intialize every default setting.
		// You can't create your own. Manager is singleton class
		//ECS::Manager* otherM = new ECS::Manager();		// illegal
		// You can't delete manager by calling delete
		//delete m;											// illegal
		// At the end of this file, there is a way to release(delete) manager.
	}

	// 1. Create entity
	// How to create entity.
	{
		// Create entity. It returns the entity you created. 
		ECS::Entity* e = m->createEntity();
		// That's it! But let's check several this.
		// This entity's id must be 0 because it's the very first one we created
		const ECS::E_ID eId = e->getId();
		assert(eId == 0);
		// Since we didn't tell which EntityPool to use, it's default.
		const std::string entityPoolName = e->getEntityPoolName();
		assert(entityPoolName == ECS::Manager::DEFAULT_POOL_NAME);
		// Check if this is alive
		const bool alive = e->isAlive();
		assert(alive);
	}

	// Clear manager.
	m->clear();

	// 2. Kill entity
	{
		ECS::Entity* e = m->createEntity();
		// Id must be 0 because we cleared manager above
		ECS::E_ID eId = e->getId();
		assert(eId == 0);
		// You can't call delete on entity. 
		// delete e;
		// Use kill() and let manager take care of it.
		e->kill();
		// Check entity info
		eId = e->getId();
		assert(eId == -1);
		const bool alive = e->isAlive();
		assert(alive == false);
		// You can still try to add components, but it will fail.
		// Try to create again.
		ECS::Entity* e2 = m->createEntity();
		// id is 1 
		const ECS::E_ID eId2 = e->getId();
		assert(eId2 == 1);

	}

	// Clear manager.
	m->clear();

	// 3. Query entity.
	// Query entity by id.
	// My recommendation is to keep Entity instance once you create it, but you can just remeber id number and query it anytime
	{
		ECS::Entity* e = m->createEntity();
		// Id must be 0 because we cleared manager above
		ECS::E_ID eId = e->getId();
		assert(eId == 0);
		// Let's pretend you lost entity.
		e = nullptr;
		// Welp...but we have id!
		e = m->getEntityById(eId);
		assert(e != nullptr);
		assert(e->getId() == eId);
		// :)
	}

	// Clear manager.
	m->clear();

	const std::string newPoolName = "CUSTOM_POOL";
	// 4. Create EntityPool
	// Create your own EntityPool in ECS. You can create entities on specific pool and let System to use specific pool on update.
	// For example, you can put all the static object in the game and let only collision system to update that pool.
	{
		// Create EntityPool. Requries string name and the size of pool. 
		// Name can not be default ("DEFAULT") name or empty. Also it's case sensitive.
		// Size is set to 2048 by default and has to be power of 2.
		ECS::EntityPool* newPool = m->createEntityPool(newPoolName, 200);
		assert(newPool != nullptr);
		assert(newPool->getName() == newPoolName);
		// 200 will rounded to 256
		unsigned int poolSize = newPool->getPoolSize();
		assert(poolSize == 256);

		// Creating entity without entity pool name will create on default entity pool
		ECS::Entity* defaultEntity = m->createEntity();
		assert(defaultEntity->getEntityPoolName() == ECS::Manager::DEFAULT_POOL_NAME);

		// Using entity pool name will create entity on that entity pool
		ECS::Entity* customEntity = m->createEntity(newPoolName);
		assert(customEntity->getEntityPoolName() == newPoolName);

		// Empty entity pool name can't be used
		ECS::Entity* badEntity = m->createEntity("");
		assert(badEntity == nullptr);

		// And you can query with name
		ECS::EntityPool* customPool = m->getEntityPool(newPoolName);
		assert(customPool != nullptr);
		assert(customPool->getName() == newPoolName);
	}

	// 5. Delete EntityPool
	// Delete EntityPool from manager.
	{
		// We didn't clear manager yet, so we still have CUSTOM_POOL.
		bool success = m->deleteEntityPool(newPoolName);
		assert(success);

		// Querying deleted entity pool will return nullptr
		ECS::EntityPool* customPool = m->getEntityPool(newPoolName);
		assert(customPool == nullptr);
		
		// You can't delete default entity pool
		success = m->deleteEntityPool(ECS::Manager::DEFAULT_POOL_NAME);
		assert(!success);
	}

	ECS::EntityPool* newPool = nullptr;
	// 6. Detach EntityPool
	// Detach EntityPool from manager. This will not delete EntityPool but simply detach from manager so you can have it on your hand.
	{
		// First add new pool because there are none. Use CUSTOM_POOL as name
		newPool = m->createEntityPool(newPoolName, 20);
		// Size will be rounded up to 32
		assert(newPool->getPoolSize() == 32);

		// Just for fun, add entity
		ECS::Entity* e = m->createEntity(newPoolName);

		// Detach
		ECS::EntityPool* detachedPool = m->detachEntityPool(newPoolName);
		assert(detachedPool->getName() == newPoolName);

		// Once you detach, you can't no longer query from manager
		assert(m->getEntityPool(newPoolName) == nullptr);

		// Also you can't detach default entity pool, empty or uneixisting name
		assert(m->detachEntityPool(ECS::Manager::DEFAULT_POOL_NAME) == nullptr);
		assert(m->detachEntityPool("") == nullptr);
		assert(m->detachEntityPool("Hello world!") == nullptr);

		// Same name. same size
		assert(newPool->getName() == detachedPool->getName());
		assert(newPool->getPoolSize() == detachedPool->getPoolSize());
		assert(newPool->getAliveEntityCount() == detachedPool->getAliveEntityCount());
		
		// Query entity and compare
		ECS::Entity* e2 = detachedPool->getEntityById(e->getId());
		assert(e2->getId() == e->getId());
		assert(e2->getEntityPoolName() == e->getEntityPoolName());
	}

	// 7. Add EntityPool
	// We can add EntityPool itself rather than creating.
	{
		// Use newPool from above.
		assert(newPool->getName() == newPoolName);

		// Add EntityPool to manager
		bool success = m->addEntityPool(newPool);
		assert(success);

		// You can't added it again with same name
		ECS::EntityPool* sameNamePool = m->createEntityPool(newPoolName);
		assert(sameNamePool == nullptr);

		// You can't add same thing twice
		success = m->addEntityPool(newPool);
		assert(success == false);
	}

	// clear manager
	m->clear();

	// Example of creating test component
	class TestComponent : public ECS::Component
	{
	public:
		TestComponent::TestComponent() : ECS::Component(), data(0) {};
		TestComponent::~TestComponent() {};
		int data;

		virtual void update(const float delta) override {}
	};

	// 8. Create component
	// Create component
	{
		// Create instance
		TestComponent* tc = new TestComponent();
		// Try to get component id, but it's invalid
		assert(tc->getId() == ECS::INVALID_C_ID);
		// What? This is becuase manager doesn't know if TestComponent exists.
		// Manager only tracks that has be exposed to manager class. 

		// You can't just create base component
		//ECS::Component* badComp = new ECS::Component();			// illegal

		// delete for now.
		delete tc;
	}

	// 9. Add one component
	// Add one component to entity. 
	{
		// Create instance
		TestComponent* tc = new TestComponent();
		// Try to get component id, but it's invalid
		assert(tc->getId() == ECS::INVALID_C_ID);
		// New entity
		ECS::Entity* e = m->createEntity();

		// Check if entity already has this type of component
		bool hasTestComp = e->hasComponent<TestComponent>();
		// Ofcourse doesn't have it because we naver added it.
		assert(hasTestComp == false);

		// Add component to entity
		bool success = e->addComponent<TestComponent>(tc);
		assert(success);

		// Because this is the first TestComponent component we created, id will be 0
		assert(tc->getId() == 0);

		// Change data
		tc->data = 5;

		// You can query like
		TestComponent* tc2 = e->getComponent<TestComponent>();
		assert(tc->getId() == tc2->getId());
		assert(tc->data == tc2->data);

		m->printComponentsInfo();
	}

	// clear manager
	m->clear();

	// 10. Add mutliple same components
	{
		// New entity
		ECS::Entity* e = m->createEntity();

		for (int i = 0; i < 5; i++)
		{
			e->addComponent<TestComponent>();
		}

		// Get allcomponents
		std::vector<TestComponent*> components = e->getComponents<TestComponent>();

		for (int i = 0; i < 5; i++)
		{
			delete components.at(i);
		}

		m->printComponentsInfo();
	}

	// Final. Delete manager
	{
		// Don't forget to delete instance. 
		// But don't worry! It will be deleted itself even if you forget ;)
		ECS::Manager::deleteInstance();
	}


	system("pause");

	return -1;
}
