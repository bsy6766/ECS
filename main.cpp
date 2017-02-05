#include <gtest\gtest.h>
#include "src\ECS.h"

class TestC1 : public ECS::Component
{
public:
	TestC1();
	~TestC1() = default;

	int data;
};

TestC1::TestC1() : ECS::Component(), data(0) {}

class TestC2 : public ECS::Component
{
public:
	TestC2();
	~TestC2();

	int data;
};

TestC2::TestC2() : ECS::Component(), data(0) {}

TestC2::~TestC2() {}

class TestSystem1 : public ECS::System
{
public:
	TestSystem1();
	~TestSystem1() = default;

	void update(const float delta, std::vector<ECS::Entity*>& entities) override {};
};

TestSystem1::TestSystem1() : System(0) {}

class TestSystem2 : public ECS::System
{
public:
	TestSystem2();
	~TestSystem2() = default;

	void update(const float delta, std::vector<ECS::Entity*>& entities) override {};
};

TestSystem2::TestSystem2() : System(1) {}

class TestSystem3 : public ECS::System
{
public:
	TestSystem3();
	~TestSystem3() = default;

	void update(const float delta, std::vector<ECS::Entity*>& entities) override {};
};

TestSystem3::TestSystem3() : System(0) {}


class HealthComponent : public ECS::Component
{
public:
	HealthComponent();
	~HealthComponent() = default;

	int health;
};

HealthComponent::HealthComponent() : ECS::Component(), health(10) {}

class HealthSystem : public ECS::System
{
public:
	HealthSystem();
	~HealthSystem() = default;

	void update(const float delta, std::vector<ECS::Entity*>& entities) override;
};

HealthSystem::HealthSystem() : ECS::System(0)
{}

void HealthSystem::update(const float delta, std::vector<ECS::Entity*>& entities)
{
	for (auto& entity : entities)
	{
		auto healthComp = entity->getComponent<HealthComponent>();
		healthComp->health++;
	}
}

class PositionComponent : public ECS::Component
{
public:
	PositionComponent();
	~PositionComponent() = default;

	float x;
	float y;
};

PositionComponent::PositionComponent() : ECS::Component(), x(0), y(0) {}

class MovementSystem : public ECS::System
{
public:
	MovementSystem();
	~MovementSystem() = default;

	void update(const float delta, std::vector<ECS::Entity*>& entities) override;
};

MovementSystem::MovementSystem() : ECS::System(1)
{}

void MovementSystem::update(const float delta, std::vector<ECS::Entity*>& entities)
{
	for (auto& entity : entities)
	{
		auto posComp = entity->getComponent<PositionComponent>();
		posComp->x += 1.0f;
	}
}

ECS::Manager* m = nullptr;

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	RUN_ALL_TESTS();

	ECS::Manager::deleteInstance();

	system("pause");
	return 0;
}

TEST(CREATION_TEST, MANAGER)
{
	m = ECS::Manager::getInstance();
	ASSERT_NE(m, nullptr);
	ASSERT_TRUE(ECS::Manager::isValid());
}

TEST(CREATION_TEST, DEFAULT_ENTITY_POOL)
{
	m->clear();

	ASSERT_TRUE(m->hasEntityPoolName(ECS::DEFAULT_ENTITY_POOL_NAME));
}

TEST(CREATION_TEST, ADDITIONAL_ENTITY_POOL)
{
	m->clear();

	ASSERT_TRUE(m->createEntityPool("TEST", 2));
	ASSERT_FALSE(m->createEntityPool("TEST2", 0));
}

TEST(CREATION_TEST, SINGLE_ENTITY)
{
	ECS::Entity* e = m->createEntity();
	ASSERT_NE(e, nullptr);
}

TEST(CREATION_TEST, MULTIPLE_ENTITIES)
{
	m->clear();

	for (int i = 0; i < 20; i++)
	{
		ECS::Entity* e = m->createEntity();
		ASSERT_NE(e, nullptr);
	}

	for (int i = 0; i < 20; i++)
	{
		ECS::Entity* e = m->getEntityById(i);
		ASSERT_NE(e, nullptr);
	}
}

TEST(CREATION_TEST, SINGLE_COMPONENT)
{
	m->clear();

	TestC1* testComponent = m->createComponent<TestC1>();
	ASSERT_NE(testComponent, nullptr);
}

TEST(CREATION_TEST, MULTIPLE_COMPONENTS)
{
	m->clear();

	for (int i = 0; i < 20; i++)
	{
		TestC1* t = m->createComponent<TestC1>();
		ASSERT_NE(t, nullptr);
	}
}

TEST(CREATION_TEST, MULTIPLE_DIFFERENT_COMPONENTS)
{
	m->clear();

	for (int i = 0; i < 20; i++)
	{
		TestC1* t = m->createComponent<TestC1>();
		ASSERT_NE(t, nullptr);
	}

	for (int i = 0; i < 20; i++)
	{
		TestC2* t = m->createComponent<TestC2>();
		ASSERT_NE(t, nullptr);
	}
}

TEST(CREATION_TEST, SYSTEM)
{
	m->clear();
	TestSystem1* ts1 = m->createSystem<TestSystem1>();
	ASSERT_NE(ts1, nullptr);
}



TEST(DELETION_TEST, MANAGER)
{
	ECS::Manager::deleteInstance();
	ASSERT_FALSE(ECS::Manager::isValid());
}

TEST(DELETION_TEST, ENTITY)
{
	m = ECS::Manager::getInstance();
	ECS::Entity* e = m->createEntity();
	ASSERT_TRUE(e != nullptr);

	e->kill();

	ASSERT_EQ(e->getId(), ECS::INVALID_E_ID);
	ASSERT_EQ(e->isAlive(), false);
}

TEST(DELETION_TEST, COMPONENT)
{
	m->clear();
	auto newComp = m->createComponent<TestC1>();
	ASSERT_EQ(newComp->getUniqueId(), 0);
	m->deleteComponent<TestC1>(newComp);
	ASSERT_EQ(newComp, nullptr);
}

TEST(DELETION_TEST, SYSTEM)
{
	m->clear();
	TestSystem1* ts1 = m->createSystem<TestSystem1>();

	m->deleteSystem<TestSystem1>(ts1);
	ASSERT_EQ(ts1, nullptr);
}



TEST(FUNCTION_TEST, MANAGER_CLEAR)
{
	m->clear();
	m = ECS::Manager::getInstance();
	ECS::Entity* e = m->createEntity();
	ASSERT_NE(e, nullptr);
	const ECS::E_ID eId = e->getId();
	ASSERT_EQ(eId, 0);

	m->createEntityPool("TEST", 2);

	auto system = m->createSystem<TestSystem1>();

	m->clear();

	e = m->getEntityById(eId);
	ASSERT_EQ(e, nullptr);

	ASSERT_FALSE(m->hasEntityPoolName("TEST"));
	ASSERT_FALSE(m->hasSystem<TestSystem1>());
}

TEST(FUNCTION_TEST, MANAGER_IS_VALID)
{
	m->clear();
	m = ECS::Manager::getInstance();
	ASSERT_TRUE(m->isValid());
	ECS::Manager::deleteInstance();
	ASSERT_FALSE(m->isValid());
	m = ECS::Manager::getInstance();
	ASSERT_TRUE(m->isValid());
}

TEST(FUNCTION_TEST, MANAGER_CREATE_ENTITY_POOL_NAME_TEST)
{
	m->clear();
	bool success = m->createEntityPool(ECS::DEFAULT_ENTITY_POOL_NAME);
	ASSERT_FALSE(success);
	success = m->createEntityPool("");
	ASSERT_FALSE(success);
	const std::string validName = " !%ASD./GA#? 32 _-";
	success = m->createEntityPool(validName, 2);
	ASSERT_TRUE(success);
}

TEST(FUNCTION_TEST, MANAGER_CREATE_ENTITY_POOL_SIZE_TEST)
{
	m->clear();
	ASSERT_EQ(m->getEntityPoolSize(), ECS::DEFAULT_ENTITY_POOL_SIZE);
	bool success = m->createEntityPool("SMALL", 2);
	ASSERT_TRUE(success);
	ASSERT_EQ(m->getEntityPoolSize("SMALL"), 2);
	success = m->createEntityPool("LARGE", 4096);
	ASSERT_TRUE(success);
	ASSERT_EQ(m->getEntityPoolSize("LARGE"), 4096);
	success = m->createEntityPool("ROUND_POW_2", 6);
	ASSERT_TRUE(success);
	ASSERT_EQ(m->getEntityPoolSize("ROUND_POW_2"), 8);
}

TEST(FUNCTION_TEST, MANAGER_CREATE_ENTITY)
{
	m->clear();

	ECS::Entity* e1 = m->createEntity();
	ASSERT_EQ(e1->getId(), 0);
	ASSERT_EQ(e1->hasComponent<TestC1>(), false);
	ASSERT_EQ(e1->getEntityPoolName(), ECS::DEFAULT_ENTITY_POOL_NAME);

	bool success = m->createEntityPool("NEW", 4);

	ECS::Entity* e2 = m->createEntity("NEW");
	ASSERT_EQ(e2->getId(), 0);
	ASSERT_EQ(e2->hasComponent<TestC1>(), false);
	ASSERT_EQ(e2->getEntityPoolName(), "NEW");

	ECS::Entity* e3 = m->createEntity("");
	ASSERT_EQ(e3, nullptr);
}

TEST(FUNCTION_TEST, MANAGER_CREATE_ENTITY_ON_FULL_ENTITY_POOL)
{
	m->clear();

	bool success = m->createEntityPool("TWO", 2);
	ECS::Entity* e1 = m->createEntity("TWO");
	ECS::Entity* e2 = m->createEntity("TWO");
	ECS::Entity* e3 = m->createEntity("TWO");
	ASSERT_EQ(e3, nullptr);
}

TEST(FUNCTION_TEST, MANAGER_GET_ENTITY_BY_ID)
{
	m->clear();

	std::vector<ECS::E_ID> eIds;

	for (int i = 0; i < 5; i++)
	{
		ECS::Entity* e = m->createEntity();
		eIds.push_back(e->getId());
	}

	for (auto eId : eIds)
	{
		ECS::Entity* e = m->getEntityById(eId);
		ASSERT_EQ(e->getId(), eId);
	}

	ECS::Entity* e = m->getEntityById(20);
	ASSERT_EQ(e, nullptr);
}

TEST(FUNCTION_TEST, MANAGER_GET_ENTITY_BY_ID_NONE_DEFAULT_ENTITY_POOL)
{
	m->clear();
	m->createEntityPool("TEST", 2);
	auto e = m->createEntity("TEST");

	auto eCheck = m->getEntityById(e->getId());
	ASSERT_EQ(e, eCheck);
}

TEST(FUNCTION_TEST, MANAGER_GET_ALL_ENTITIES_IN_POOL)
{
	m->clear();

	for (int i = 0; i < 20; i++)
	{
		m->createEntity();
	}

	std::vector<ECS::Entity*> entities;
	m->getAllEntitiesInPool(entities);

	ASSERT_EQ(entities.size(), 20);

	for (int i = 0; i < 20; i++)
	{
		ASSERT_EQ(entities.at(i)->getId(), i);
	}
}

TEST(FUNCTION_TEST, MANAGER_GET_ALL_ENTITIES_FOR_SYSTEM)
{
	m->clear();

	m->createEntityPool("FIRST", 16);
	m->createEntityPool("SECOND", 16);

	for (int i = 0; i < 10; i++)
	{
		m->createEntity("FIRST");
		m->createEntity("SECOND");
	}

	auto s = m->createSystem<TestSystem1>();
	s->disbaleDefafultEntityPool();
	s->addEntityPoolName("FIRST");
	s->addEntityPoolName("SECOND");

	std::vector<ECS::Entity*> entities;
	m->getAllEntitiesForSystem<TestSystem1>(entities);

	ASSERT_EQ(entities.size(), 20);
}

TEST(FUNCTION_TEST, MANAGER_MOVE_ENTITY_TO_ENTITY_POOL)
{
	m->clear();

	auto e1 = m->createEntity();
	e1->addComponent<TestC1>();

	m->createEntityPool("TEST", 2);

	bool success = m->moveEntityToEntityPool(e1, "TEST");
	ASSERT_TRUE(success);
	ASSERT_NE(e1, nullptr);
	ASSERT_EQ(e1->getId(), 0);
	ASSERT_EQ(e1->getEntityPoolName(), "TEST");
	auto c1 = e1->getComponent<TestC1>();
	ASSERT_NE(c1, nullptr);
}

TEST(FUNCTION_TEST, MANAGER_MOVE_ENTITY_TO_FULL_ENTITY_POOL)
{
	m->clear();

	auto e1 = m->createEntity();

	m->createEntityPool("TEST", 2);
	auto e2 = m->createEntity("TEST");
	auto e3 = m->createEntity("TEST");

	bool success = m->moveEntityToEntityPool(e1, "TEST");
	ASSERT_FALSE(success);

	ASSERT_EQ(e1->getEntityPoolName(), ECS::DEFAULT_ENTITY_POOL_NAME);
}

TEST(FUNCTION_TEST, MANAGER_RESIZE_ENTITY_POOL)
{
	m->clear();
	m->createEntityPool("TEST", 2);

	auto e1 = m->createEntity("TEST");
	auto e2 = m->createEntity("TEST");
	auto bad = m->createEntity("TEST");
	ASSERT_EQ(bad, nullptr);

	bool success = m->resizeEntityPool("TEST", 4);
	ASSERT_TRUE(success);
	ASSERT_EQ(m->getEntityPoolSize("TEST"), 4);

	auto d1 = m->createEntity("TEST");
	ASSERT_NE(d1, nullptr);
	auto d2 = m->createEntity("TEST");
	ASSERT_NE(d2, nullptr);

	success = m->resizeEntityPool("TEST", 4);
	std::vector<ECS::Entity*> entities;
	m->getAllEntitiesInPool(entities, "TEST");
	ASSERT_EQ(entities.size(), 4);
	ASSERT_EQ(entities.at(0)->getId(), e1->getId());
	ASSERT_EQ(entities.at(1)->getId(), e2->getId());
	ASSERT_EQ(entities.at(2)->getId(), d1->getId());
	ASSERT_EQ(entities.at(3)->getId(), d2->getId());

	m->resizeEntityPool("TEST", 2);
	entities.clear();
	m->getAllEntitiesInPool(entities, "TEST");
	ASSERT_EQ(entities.size(), 2);
	ASSERT_EQ(entities.at(0)->getId(), e1->getId());
	ASSERT_EQ(entities.at(1)->getId(), e2->getId());
	ASSERT_NE(entities.at(0)->getId(), d1->getId());
	ASSERT_NE(entities.at(1)->getId(), d2->getId());
}

TEST(FUNCTION_TEST, MANAGER_DELETE_ENTITY)
{
	m->clear();

	auto e = m->createEntity();
	e->addComponent<TestC2>();

	e->kill();

	ASSERT_EQ(m->getComponentCount<TestC2>(), 0);
}

TEST(FUNCTION_TEST, MANAGER_DELETE_ENTITY_ON_NONE_DEFAULT_ENTITY_POOL)
{
	m->clear();

	m->createEntityPool("TEST", 2);

	auto e = m->createEntity("TEST");
	e->addComponent<TestC2>();

	e->kill();

	ASSERT_EQ(e->getId(), ECS::INVALID_E_ID);
	ASSERT_EQ(e->isAlive(), false);
}

TEST(FUNCTION_TEST, ADD_COMPONENT)
{
	m->clear();
	ECS::Entity* e = m->createEntity();
	bool success = e->addComponent<TestC1>();
	ASSERT_TRUE(success);

	bool fail = e->addComponent<TestC1>(nullptr);
	ASSERT_FALSE(fail);
}

TEST(FUNCTION_TEST, HAS_COMPONENT)
{
	m->clear();

	ECS::Entity* e = m->createEntity();
	ASSERT_EQ(e->hasComponent<TestC1>(), false);

	TestC2* tc = m->createComponent<TestC2>();
	bool success = e->addComponent<TestC2>(tc);
	ASSERT_TRUE(success);
	ASSERT_EQ(e->hasComponent<TestC1>(), false);
	ASSERT_EQ(e->hasComponent<TestC2>(), true);

	bool hasComponent = e->hasComponent<TestC2>(tc);
	ASSERT_TRUE(hasComponent);
}

TEST(FUNCTION_TEST, GET_COMPONENT)
{
	m->clear();

	ECS::Entity* e = m->createEntity();
	bool success = e->addComponent<TestC1>();
	ASSERT_TRUE(success);

	TestC1* tc1 = e->getComponent<TestC1>();
	ASSERT_NE(tc1, nullptr);
	TestC2* tc2 = e->getComponent<TestC2>();
	ASSERT_EQ(tc2, nullptr);

	for (int i = 0; i < 5; i++)
	{
		e->addComponent<TestC1>();
	}

	TestC1* firstTC1Comp = e->getComponent<TestC1>();

	std::vector<TestC1*> components = e->getComponents<TestC1>();

	ASSERT_EQ(components.front()->getId(), firstTC1Comp->getId());
	ASSERT_EQ(components.front()->getUniqueId(), firstTC1Comp->getUniqueId());

	for (auto component : components)
	{
		ASSERT_NE(component, nullptr);
		ASSERT_EQ(component->getUniqueId(), tc1->getUniqueId());
		ASSERT_EQ(component->getOwnerId(), tc1->getOwnerId());
	}

	std::vector<TestC2*> badComponents = e->getComponents<TestC2>();
	ASSERT_TRUE(badComponents.empty());
}

TEST(FUNCTION_TEST, REMOVE_COMPONENT)
{
	m->clear();

	ECS::Entity* e = m->createEntity();
	TestC1* tc1 = m->createComponent<TestC1>();
	bool success = e->addComponent<TestC1>(tc1);
	ASSERT_TRUE(success);

	success = e->removeComponent<TestC1>(tc1->getId());
	ASSERT_TRUE(success);
	success = e->removeComponent<TestC1>(tc1->getId());
	ASSERT_FALSE(success);

	TestC1* tc11 = e->getComponent<TestC1>();
	ASSERT_EQ(tc11, nullptr);

	for (int i = 0; i < 5; i++)
	{
		e->addComponent<TestC1>();
	}

	std::vector<TestC1*> comps1 = e->getComponents<TestC1>();
	ASSERT_EQ(comps1.size(), 5);

	TestC1* tc111 = e->getComponent<TestC1>();
	ASSERT_NE(tc111, nullptr);

	success = e->removeComponent<TestC1>(tc111);
	ASSERT_TRUE(success);

	success = e->removeComponents<TestC1>();
	ASSERT_TRUE(success);

	std::vector<TestC1*> comps3 = e->getComponents<TestC1>();
	ASSERT_EQ(comps3.size(), 0);

	success = e->removeComponents<TestC1>();
	ASSERT_FALSE(success);

}

TEST(FUNCTION_TEST, SIGNATURE)
{
	m->clear();

	ECS::Entity* e = m->createEntity();

	ASSERT_EQ(e->getSignature(), 0);

	TestC1* tc1 = m->createComponent<TestC1>();
	e->addComponent<TestC1>(tc1);

	ASSERT_TRUE(e->getSignature()[tc1->getUniqueId()] == true);
}

TEST(FUNCTION_TEST, SYSTEM_DUPLICATION)
{
	m->clear();

	auto system = m->createSystem<TestSystem1>();
	auto badSystem = m->createSystem<TestSystem1>();
	ASSERT_EQ(badSystem, nullptr);
}

TEST(FUNCTION_TEST, HAS_SYSTEM)
{
	m->clear();

	ASSERT_FALSE(m->hasSystem<TestSystem1>());

	auto system = m->createSystem<TestSystem1>();

	ASSERT_TRUE(m->hasSystem<TestSystem1>());
	ASSERT_FALSE(m->hasSystem<TestSystem2>());
	ASSERT_TRUE(m->hasSystem<TestSystem1>(system));

	m->deleteSystem<TestSystem1>(system);

	ASSERT_FALSE(m->hasSystem<TestSystem1>());
	ASSERT_FALSE(m->hasSystem<TestSystem1>(system));
}

TEST(FUNCTION_TEST, GET_SYSTEM)
{
	m->clear();

	auto s1 = m->createSystem<TestSystem1>();
	auto s2 = m->createSystem<TestSystem2>();

	auto check = m->getSystem<TestSystem1>();
	ASSERT_EQ(s1->getId(), check->getId());
	ASSERT_NE(s2->getId(), check->getId());
}

TEST(FUNCTION_TEST, SYSTEM_COMPONENT_TYPE)
{
	m->clear();

	auto s = m->createSystem<TestSystem1>();
	
	ECS::Signature signature = s->getSignature();
	ASSERT_EQ(signature.to_ulong(), 0);

	bool success = s->addComponentType<TestC1>();
	ASSERT_FALSE(success);
	signature = s->getSignature();
	ASSERT_EQ(signature.to_ulong(), 0);

	auto e = m->createEntity();
	e->addComponent<TestC1>();
	e->addComponent<TestC2>();
	
	success = s->addComponentType<TestC1>();
	ASSERT_TRUE(success);
	success = s->addComponentType<TestC1>();
	ASSERT_TRUE(success);
	signature = s->getSignature();
	ASSERT_EQ(signature.to_ulong(), 1);
	success = s->addComponentType<TestC2>();
	ASSERT_TRUE(success);
	signature = s->getSignature();
	ASSERT_EQ(signature.to_ulong(), 3);

	success = s->removeComponentType<TestC1>();
	ASSERT_TRUE(success);
	signature = s->getSignature();
	ASSERT_EQ(signature.to_ulong(), 2);
	success = s->removeComponentType<TestC1>();
	ASSERT_TRUE(success);
}

TEST(FUNCTION_TEST, SYSTEM_PRIORITY)
{
	m->clear();

	auto s1 = m->createSystem<TestSystem1>();	// priority = 0
	auto s2 = m->createSystem<TestSystem2>();	// priority = 1

	ASSERT_EQ(s1->getPriority(), 0);
	ASSERT_EQ(s2->getPriority(), 1);
}

TEST(FUNCTION_TEST, SYSTEM_SAME_PRIORITY)
{
	m->clear();
	auto s1 = m->createSystem<TestSystem1>();	// priority = 0
	auto s3 = m->createSystem<TestSystem3>();	// priority = 0

	ASSERT_EQ(s1->getPriority(), 0);
	ASSERT_EQ(s3, nullptr);
}

TEST(FUNCTION_TEST, SYSTEM_UPDATE_ORDER)
{
	m->clear();

	auto s1 = m->createSystem<TestSystem1>();	// priority = 0
	auto s2 = m->createSystem<TestSystem2>();	// priority = 1

	auto updateOrder = m->getSystemUpdateOrder();

	ASSERT_EQ(updateOrder.at(s1->getPriority()), s1->getId());
	ASSERT_EQ(updateOrder.at(s2->getPriority()), s2->getId());
}

TEST(FUNCTION_TEST, SYSTEM_ACTIVE_STATE)
{
	m->clear();

	auto e1 = m->createEntity();
	e1->addComponent<HealthComponent>();

	auto s1 = m->createSystem<HealthSystem>();
	s1->addComponentType<HealthComponent>();

	m->update(0);

	ASSERT_EQ(e1->getComponent<HealthComponent>()->health, 11);

	s1->deactivate();
	ASSERT_EQ(s1->isActive(), false);

	m->update(0);

	ASSERT_EQ(e1->getComponent<HealthComponent>()->health, 11);

	s1->activate();
	ASSERT_EQ(s1->isActive(), true);

	m->update(0);

	ASSERT_EQ(e1->getComponent<HealthComponent>()->health, 12);
}

TEST(FUNCTION_TEST, MANGER_UPDATE)
{
	m->clear();

	auto e1 = m->createEntity();
	auto e2 = m->createEntity();

	e1->addComponent<HealthComponent>();
	e2->addComponent<HealthComponent>();
	e2->addComponent<PositionComponent>();

	auto s1 = m->createSystem<HealthSystem>();
	auto s2 = m->createSystem<MovementSystem>();

	s1->addComponentType<HealthComponent>();
	s2->addComponentType<PositionComponent>();

	m->update(0);

	ASSERT_EQ(e1->getComponent<HealthComponent>()->health, 11);

	ASSERT_EQ(e2->getComponent<HealthComponent>()->health, 11);
	ASSERT_EQ(e2->getComponent<PositionComponent>()->x, 1);
	ASSERT_EQ(e2->getComponent<PositionComponent>()->y, 0);
}

TEST(END, DELETE_MANAGER)
{
	ECS::Manager::deleteInstance();
}