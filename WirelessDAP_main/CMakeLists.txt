#set(COMPONENT_ADD_INCLUDEDIRS "${PROJECT_PATH}/components/wireless-esp8266-dap")
#set(COMPONENT_SRCS
#    main.c timer.c tcp_server.c usbip_server.c DAP_handle.c
#    kcp_server.c tcp_netconn.c uart_bridge.c wifi_handle.c)
#
#register_component()
FILE(GLOB_RECURSE main_source ./*.*)

idf_component_register(SRCS ${main_source}
        INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/components/wireless-esp8266-dap"
        PRIV_REQUIRES driver nvs_flash esp_wifi esp_ringbuf lwip soc mdns DAP corsacOTA elaphureLink kcp USBIP
        )