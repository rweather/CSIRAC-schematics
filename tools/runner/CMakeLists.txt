add_executable(csirac
    main.c
)
target_link_libraries(csirac PUBLIC csirac-core)
install(TARGETS csirac DESTINATION bin)
