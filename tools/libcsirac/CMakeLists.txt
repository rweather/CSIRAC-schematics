
add_library(csirac-core STATIC
    csirac.h
    csirac_bootstrap.c
    csirac_drum.c
    csirac_media.c
    csirac_run.c
    csirac_state.c
)
install(TARGETS csirac-core DESTINATION lib)
install(FILES csirac.h DESTINATION include/libcsirac)
