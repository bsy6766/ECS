#include <gtest\gtest.h>
#include "src\ECS.h"

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	RUN_ALL_TESTS();
	system("pause");
	return 0;
}

class TestC1 : public ECS::Component
{
public:
	TestC1();
	~TestC1() = default;

	int data;

	void update(const float delta) override {};
};

TestC1::TestC1() : ECS::Component(), data(0) {}

class TestC2 : public ECS::Component
{
public:
	TestC2();
	~TestC2();

	int data;

	void update(const float delta) override {};
};

TestC2::TestC2() : ECS::Component(), data(0) {}

TestC2::~TestC2() {}

ECS::Manager* m = nullptr;

TEST(CREATION_TEST, MANAGER)
{
	m = ECS::Manager::getInstance();
	ASSERT_NE(m, nullptr);
	ASSERT_TRUE(ECS::Manager::isValid());

	m->clear();
}

TEST(CREATION_TEST, DEFAULT_ENTITY_POOL)
{
	ECS::EntityPool* entityPool = m->getEntityPool();
	ASSERT_EQ(entityPool->getName(), ECS::DEFAULT_ENTITY_POOL_NAME);
	ASSERT_EQ(entityPool->getPoolSize(), ECS::DEFAULT_ENTITY_POOL_SIZE);
	ASSERT_EQ(entityPool->countAliveEntity(), 0);

	m->clear();
}

TEST(CREATION_TEST, SINGLE_ENTITY)
{
	ECS::Entity* e = m->createEntity();
	ASSERT_NE(e, nullptr);
	ASSERT_EQ(e->getId(), 0);

	m->clear();
}

TEST(CREATION_TEST, MULTIPLE_ENTITIES)
{
	for (int i = 0; i < 20; i++)
	{
		ECS::Entity* e = m->createEntity();
		ASSERT_NE(e, nullptr);
		ASSERT_EQ(e->getId(), i);
	}

	m->clear();
}

TEST(CREATION_TEST, ENTITY_POOL)
{
	const std::string testPoolName = "TEST0";
	ECS::EntityPool* newPool = m->createEntityPool(testPoolName, 200);
	ASSERT_EQ(newPool->getPoolSize(), 256);
	ASSERT_EQ(newPool->getName(), testPoolName);
}

TEST(CREATION_TEST, MULTIPLE_ENTITY_POOLS)
{
	for (int i = 1; i < 4; i++)
	{
		const std::string testPoolName = "TEST" + std::to_string(i);
		ECS::EntityPool* newPool = m->createEntityPool(testPoolName, 200);
		ASSERT_EQ(newPool->getPoolSize(), 256);
		ASSERT_EQ(newPool->getName(), testPoolName);
	}

	for (int i = 0; i < 4; i++)
	{
		const std::string testPoolName = "TEST" + std::to_string(i);
		ECS::EntityPool* testPool = m->getEntityPool(testPoolName);
		ASSERT_TRUE(testPool != nullptr);
		ASSERT_EQ(testPool->getPoolSize(), 256);
		ASSERT_EQ(testPool->getName(), testPoolName);
	}
}

TEST(CREATION_TEST, SINGLE_COMPONENT)
{
	TestC1* testComponent = m->createComponent<TestC1>();
	ASSERT_EQ(testComponent->getUniqueId(), 0);
	ASSERT_EQ(testComponent->getId(), ECS::INVALID_C_ID);
	ASSERT_EQ(testComponent->getOwnerId(), ECS::INVALID_E_ID);

	m->clear();
}

TEST(CREATION_TEST, MULTIPLE_COMPONENTS)
{
	for (int i = 0; i < 20; i++)
	{
		TestC1* testComponent = m->createComponent<TestC1>();
		ASSERT_EQ(testComponent->getUniqueId(), 0);
		ASSERT_EQ(testComponent->getId(), ECS::INVALID_C_ID);
		ASSERT_EQ(testComponent->getOwnerId(), ECS::INVALID_E_ID);
	}

	m->clear();
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
	m->update(0);

	ASSERT_EQ(e->getId(), ECS::INVALID_E_ID);
	ASSERT_EQ(e->isAlive(), false);
}

TEST(DELETION_TEST, ENTITY_POOL)
{
	ECS::EntityPool* testPool1 = m->createEntityPool("TEST1", 128);

	bool success = m->deleteEntityPool("TEST1");
	ASSERT_TRUE(success);

	testPool1 = m->getEntityPool("TEST1");
	ASSERT_EQ(testPool1, nullptr);
}

TEST(DELETION_TEST, COMPONENT)
{
	m->clear();
	auto newComp = m->createComponent<TestC1>();
	ASSERT_EQ(newComp->getUniqueId(), 0);
	m->deleteComponent<TestC1>(newComp);
	ASSERT_EQ(newComp, nullptr);
}



TEST(FUNCTION_TEST, MANAGER_CLEAR)
{
	m = ECS::Manager::getInstance();
	ECS::Entity* e = m->createEntity();
	ASSERT_NE(e, nullptr);
	const ECS::E_ID eId = e->getId();
	ASSERT_EQ(eId, 0);

	m->clear();
	e = m->getEntityById(eId);
	ASSERT_EQ(e, nullptr);

	ECS::EntityPool* defaultPool = m->getEntityPool();
	ASSERT_TRUE(defaultPool != nullptr);
	ASSERT_EQ(defaultPool->getName(), ECS::DEFAULT_ENTITY_POOL_NAME);
	ASSERT_EQ(defaultPool->getPoolSize(), ECS::DEFAULT_ENTITY_POOL_SIZE);

	m->clear();
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
	ECS::EntityPool* defaultPool = m->createEntityPool(ECS::DEFAULT_ENTITY_POOL_NAME);
	ASSERT_EQ(defaultPool, nullptr);
	ECS::EntityPool* emptyNamePool = m->createEntityPool("");
	ASSERT_EQ(emptyNamePool, nullptr);
	const std::string validName = " !%ASD./GA#? 32 _-";
	ECS::EntityPool* validPool = m->createEntityPool(validName, 2);
	ASSERT_NE(validPool, nullptr);
}

TEST(FUNCTION_TEST, MANAGER_CREATE_ENTITY_POOL_SIZE_TEST)
{
	m->clear();
	ECS::EntityPool* defaultPool = m->getEntityPool(ECS::DEFAULT_ENTITY_POOL_NAME);
	ASSERT_EQ(defaultPool->getPoolSize(), ECS::DEFAULT_ENTITY_POOL_SIZE);
	ECS::EntityPool* smallPool = m->createEntityPool("SMALL", 2);
	ASSERT_EQ(smallPool->getPoolSize(), 2);
	ECS::EntityPool* largePool = m->createEntityPool("LARGE", 4096);
	ASSERT_EQ(largePool->getPoolSize(), 4096);
	ECS::EntityPool* sizeRoundUpPool = m->createEntityPool("SIZE_ROUND_UP_POOL", 6);
	ASSERT_EQ(sizeRoundUpPool->getPoolSize(), 8);
}

TEST(FUNCTION_TEST, MANAGER_DETACH_ENTITY_POOL)
{
	m->clear();

	ECS::EntityPool* defaultPool = m->detachEntityPool(ECS::DEFAULT_ENTITY_POOL_NAME);
	ASSERT_TRUE(defaultPool == nullptr);

	const std::string testPoolName = "TEST";
	m->createEntityPool(testPoolName, 2);

	ECS::EntityPool* testPool = m->detachEntityPool(testPoolName);
	ASSERT_TRUE(testPool != nullptr);
	ASSERT_EQ(testPool->getName(), testPoolName);

	ECS::EntityPool* testPoolAgain = m->getEntityPool(testPoolName);
	ASSERT_EQ(testPoolAgain, nullptr);

	ECS::EntityPool* emptyName = m->detachEntityPool("");
	ASSERT_EQ(emptyName, nullptr);
}

TEST(FUNCTION_TEST, MANAGER_GET_ENTITY_POOL)
{
	m->clear();

	ECS::EntityPool* defaultPool = m->getEntityPool(ECS::DEFAULT_ENTITY_POOL_NAME);
	ASSERT_NE(defaultPool, nullptr);

	ECS::EntityPool* emptyNamePool = m->getEntityPool("");
	ASSERT_EQ(emptyNamePool, nullptr);

	ECS::EntityPool* wrongNamePool = m->getEntityPool("WRONG_NAME!");
	ASSERT_EQ(wrongNamePool, nullptr);
}

TEST(FUNCTION_TEST, MANAGER_CREATE_ENTITY)
{
	m->clear();

	ECS::Entity* e1 = m->createEntity();
	ASSERT_EQ(e1->getId(), 0);
	ASSERT_EQ(e1->hasComponent<TestC1>(), false);
	ASSERT_EQ(e1->getEntityPoolName(), ECS::DEFAULT_ENTITY_POOL_NAME);

	ECS::EntityPool* pool = m->createEntityPool("NEW", 4);

	ECS::Entity* e2 = m->createEntity("NEW");
	ASSERT_EQ(e2->getId(), 1);
	ASSERT_EQ(e2->hasComponent<TestC1>(), false);
	ASSERT_EQ(e2->getEntityPoolName(), "NEW");

	ECS::Entity* e3 = m->createEntity("");
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