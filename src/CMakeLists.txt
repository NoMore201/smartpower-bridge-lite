add_executable(
        smartpower
        main.cpp
        hid_pnp.cpp
)

target_link_libraries(
	smartpower
	pthread
	${LIBHIDAPI}
)
