add_executable(csirac-assembler
    main.c
    code.c
    code.h
    parse.c
    parse.h
    token.c
    token.h
)
target_link_libraries(csirac-assembler PUBLIC csirac-core)
install(TARGETS csirac-assembler DESTINATION bin)
