add_executable(test_types test_types.cpp)
target_link_libraries(test_types PRIVATE input_manager)
add_test(NAME test_types COMMAND test_types)

add_executable(test_manager test_manager.cpp)
target_link_libraries(test_manager PRIVATE input_manager)
add_test(NAME test_manager COMMAND test_manager)
