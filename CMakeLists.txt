cmake_minimum_required(VERSION 3.10)
project(project2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(print_info MESSAGE_TEXT)
    message(STATUS "${MESSAGE_TEXT}")
endfunction()

print_info("\Советы по использованию")
print_info("Много типов при генерации ломают программу, потому что не хватает памяти,\n поэтому желательно использовать не больше трех типов\n или расширить стек.")



if(DEFINED TYPES)
    execute_process(
        COMMAND python3 generator.py "${TYPES}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    add_executable(project2 main.cpp)
    target_include_directories(project2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    print_info("Генерация завершена успешно")
else()
    print_info("Ошибка: TYPES не определен! используйте -DTYPES=<value>.")
    message(FATAL_ERROR "TYPES variable is not defined. Use -DTYPES=<value>.")
endif()
