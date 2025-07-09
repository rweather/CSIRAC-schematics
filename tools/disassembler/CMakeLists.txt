add_executable(csirac-disassembler
    main.c
)
target_link_libraries(csirac-disassembler PUBLIC csirac-core)
install(TARGETS csirac-disassembler DESTINATION bin)
