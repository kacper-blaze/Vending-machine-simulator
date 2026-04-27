cmake_minimum_required(VERSION 4.1)
project(vendingMachineSimulator C)

set(CMAKE_C_STANDARD 99)

add_executable(vendingMachineSimulator main.c
        vending_machine.h
        vending_machine.c)
