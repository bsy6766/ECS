# ECS
Simple ECS implementation with C++

## Manager
Manager class is the class that manages entire ECS. <br>
There is only one manager class (singleton) and it can create Entity, EntityPool, Component and System.

## Entity
Entity is just a pack of numbers.<br>
Entity itself is represented with id, an unsigned integer number.<br>
Entity does not hold any Component, but it knows where Components exist by storing Component id. <br>

## EntityPool
EntityPool is a pool where entities are stored. <br>

## Component
Component is an object that holds data.<br>

## System
System is an object that has logic.<br> 

## Examples
There are some unittests in main.cpp to see how it works. Also header is fully commneted.
