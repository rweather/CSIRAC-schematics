
function(build_csirac_program name options)
    add_custom_command(
        OUTPUT ${name}.cvt 
        COMMAND ${CSIRAC_ASSEMBLER} ${CMAKE_CNA_FLAGS} ${options} -o ${name}.cvt ${CMAKE_CURRENT_LIST_DIR}/${name}.cna
        COMMAND ${CMAKE_COMMAND} -E copy ${name}.cvt ${CMAKE_CURRENT_LIST_DIR}/${name}.cvt
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${name}.cna ${CSIRAC_ASSEMBLER}
    )
    add_custom_target(${name} ALL DEPENDS ${name}.cvt)
endfunction()

function(build_csirac_library name options)
    add_custom_command(
        OUTPUT ${name}.cvt 
        COMMAND ${CSIRAC_ASSEMBLER} ${CMAKE_CNA_FLAGS} ${options} -o ${name}.cvt ${CMAKE_CURRENT_LIST_DIR}/${name}.cna
        COMMAND ${CMAKE_COMMAND} -E copy ${name}.cvt ${CMAKE_CURRENT_LIST_DIR}/${name}.cvt
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${name}.cna ${CSIRAC_ASSEMBLER}
    )
    add_custom_target(${name} ALL DEPENDS ${name}.cvt)
    install(FILES ${name}.cvt ${CMAKE_CURRENT_LIST_DIR}/${name}.cna DESTINATION share/csirac/library/${name})
endfunction()

