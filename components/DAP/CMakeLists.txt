#set(COMPONENT_ADD_INCLUDEDIRS "config include $ENV{IDF_PATH}/components/esp_ringbuf/include/ $ENV{IDF_PATH}/components/")
#set(COMPONENT_SRCS "./source/DAP.c ./source/DAP_vendor.c ./source/JTAG_DP.c ./source/SW_DP.c ./source/SWO.c  ./source/spi_op.c ./source/spi_switch.c ./source/dap_utility.c")
#
#
#
#register_component()
FILE(GLOB_RECURSE dap_source ./source/*.*)

idf_component_register(SRCS ${dap_source}
        INCLUDE_DIRS "./config" "./include" "${IDF_PATH}/components/esp_ringbuf/include/" "${IDF_PATH}/components/"
        REQUIRES esp_ringbuf
        )