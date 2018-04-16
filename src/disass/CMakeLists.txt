add_executable(disass main.c disass.c ../../include/nessan-gtr/parser.h ../../include/nessan-gtr/ines.h)
target_link_libraries(disass parser)