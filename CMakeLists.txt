FILE(GLOB_RECURSE main_source ./main/*.*)

idf_component_register(SRCS ${main_source} INCLUDE_DIRS "./main" ".")