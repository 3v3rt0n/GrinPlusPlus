set(TARGET_NAME ed25519-donna)

file(GLOB SRC
    "ed25519_tor.c"
)
string( REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
string( REPLACE "/DNDEBUG" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")

# ed25519-donna Library
add_library(${TARGET_NAME} STATIC ${SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE ED25519_CUSTOMRANDOM ED25519_CUSTOMHASH)
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tor ${CMAKE_CURRENT_SOURCE_DIR}/tor/ext)