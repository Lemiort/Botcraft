project(5_ResidentSleeperExample)

set(SRC_FILES
${PROJECT_SOURCE_DIR}/src/ResidentSleeperBot.cpp
${PROJECT_SOURCE_DIR}/src/main.cpp
)
set(HDR_FILES 
${PROJECT_SOURCE_DIR}/include/ResidentSleeperBot.hpp
)


add_executable(5_ResidentSleeperExample ${HDR_FILES} ${SRC_FILES})
target_include_directories(5_ResidentSleeperExample PUBLIC include)
target_link_libraries(5_ResidentSleeperExample botcraft)

set_property(TARGET 5_ResidentSleeperExample PROPERTY CXX_STANDARD 14)
set_target_properties(5_ResidentSleeperExample PROPERTIES FOLDER Examples)
set_target_properties(5_ResidentSleeperExample PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(5_ResidentSleeperExample PROPERTIES RELWITHDEBINFO_POSTFIX "_rd")

install(TARGETS 5_ResidentSleeperExample RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})