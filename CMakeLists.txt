cmake_minimum_required(VERSION 3.10)
project(vendingMachineSimulator C)

set(CMAKE_C_STANDARD 99)

# Console version (always builds)
add_executable(vendingMachineSimulator main.c
        vending_machine.h
        vending_machine.c
        logger.c
        maintenance.c)
target_link_libraries(vendingMachineSimulator pthread)

# Try to find GTK+3 for GUI version
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(GTK3 gtk+-3.0)
    
    if(GTK3_FOUND)
        # GUI version (only builds if GTK+3 is available)
        add_executable(vendingMachineSimulatorGUI gui_main.c
                vending_machine.h
                vending_machine.c
                logger.c
                maintenance.c)
        
        target_include_directories(vendingMachineSimulatorGUI PRIVATE ${GTK3_INCLUDE_DIRS})
        target_link_libraries(vendingMachineSimulatorGUI ${GTK3_LIBRARIES})
        target_compile_options(vendingMachineSimulatorGUI PRIVATE ${GTK3_CFLAGS_OTHER})
        target_link_libraries(vendingMachineSimulatorGUI pthread)
        
        message(STATUS "GTK+3 found - GUI version will be built")
    else()
        message(WARNING "GTK+3 development libraries not found. GUI version will not be built.")
        message(WARNING "To install GTK+3 dev libraries, run: sudo apt install libgtk-3-dev")
    endif()
else()
    message(WARNING "PkgConfig not found. GUI version will not be built.")
endif()
