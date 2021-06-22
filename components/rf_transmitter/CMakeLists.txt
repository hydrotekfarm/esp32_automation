idf_component_register(
	SRCS "rf_transmitter.c" "rf_libs/hc12_lib.c"
	INCLUDE_DIRS "." "rf_libs" 	
	REQUIRES log boot
)

