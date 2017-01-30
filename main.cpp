#include <memory>
#include <iostream>
#include "src\ECS.h"
#include <typeinfo>
#include <typeindex>
#include <limits>

class Temp;

class Deleter
{
	friend std::unique_ptr<Temp, Deleter>;
private:
	void operator()(Temp* t) 
	{ 
		std::cout << "!" << std::endl;
		delete t; 
	}
};

class Temp 
{
public:
	Temp() :data(0) {}
	int data;
private:
	friend Deleter;
	~Temp() = default;
};

class Test
{
public:
	int a;
};

int main()
{
	/*
	{
		std::unique_ptr<Temp, Deleter> temp = std::unique_ptr<Temp, Deleter>(new Temp(), Deleter());

		Temp* t = temp.get();

		t = nullptr;

		temp->data = 2;
	}

	Test t;
	const std::type_info& info = typeid(Test);
	std::cout << "type index = " << std::type_index(info).name() << std::endl;
	std::cout << "name = " << typeid(Test).name() << std::endl;

	*/

	//ECS::Manager* m = ECS::Manager::getInstance();

	unsigned long l = -1;
	std::cout << l << std::endl;

	system("pause");

	return -1;
}