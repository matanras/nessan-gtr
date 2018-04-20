add_executable(emu
	main.c
	emu.c
	emu.h
	${CMAKE_CURRENT_SOURCE_DIR}/cpu/cpu.c
	${CMAKE_CURRENT_SOURCE_DIR}/cpu/cpu.h
	${CMAKE_CURRENT_SOURCE_DIR}/cpu/cpudefs.h
	${CMAKE_CURRENT_SOURCE_DIR}/cpu/cpu_internal.h
	${CMAKE_CURRENT_SOURCE_DIR}/cpu/mmu.h
	${CMAKE_CURRENT_SOURCE_DIR}/cpu/mmu.c
	${CMAKE_CURRENT_SOURCE_DIR}/ram/ram.h
	${CMAKE_CURRENT_SOURCE_DIR}/ram/ram.c
	${CMAKE_CURRENT_SOURCE_DIR}/ram/ram_internal.h
	../../include/nessan-gtr/parser.h
	../../include/nessan-gtr/ines.h
	../../include/optparse.h)
target_link_libraries(emu parser)