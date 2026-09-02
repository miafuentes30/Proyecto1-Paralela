if(NOT DEFINED EXECUTABLE OR NOT DEFINED OUTPUT_CSV)
    message(FATAL_ERROR "Faltan EXECUTABLE u OUTPUT_CSV")
endif()

if(DEFINED TOOLCHAIN_BIN)
    set(ENV{PATH} "${TOOLCHAIN_BIN};$ENV{PATH}")
endif()

file(REMOVE "${OUTPUT_CSV}")
execute_process(
    COMMAND "${EXECUTABLE}" 20 benchmark 2 20260828 2 10 "${OUTPUT_CSV}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Benchmark fallo (${result}):\n${output}\n${error}")
endif()
if(NOT output MATCHES "BENCHMARK_OK")
    message(FATAL_ERROR "El benchmark no produjo BENCHMARK_OK:\n${output}")
endif()
if(NOT EXISTS "${OUTPUT_CSV}")
    message(FATAL_ERROR "El benchmark no genero el CSV")
endif()

file(READ "${OUTPUT_CSV}" csv)
string(REGEX MATCHALL "medicion," measurements "${csv}")
list(LENGTH measurements measurement_count)
string(REGEX MATCHALL "promedio," averages "${csv}")
list(LENGTH averages average_count)
if(NOT measurement_count EQUAL 10 OR NOT average_count EQUAL 1)
    message(FATAL_ERROR "Conteos CSV invalidos: mediciones=${measurement_count}, promedios=${average_count}")
endif()

message(STATUS "Benchmark sin ventana y CSV comprobados")
