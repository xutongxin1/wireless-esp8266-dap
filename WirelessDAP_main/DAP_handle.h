#ifndef __DAP_HANDLE_H__
#define __DAP_HANDLE_H__

#include "components/USBIP/usbip_defs.h"

//enum reset_handle_t {
//    NO_SIGNAL = 0,
//    RESET_HANDLE = 1,
//    DELETE_HANDLE = 2,
//};
// 重复定义了，与tcp_server那里的重复了

void handle_dap_data_request(usbip_stage2_header *header, uint32_t length);
void handle_dap_data_response(usbip_stage2_header *header);
void handle_swo_trace_response(usbip_stage2_header *header);
void handle_dap_unlink();
void DAP_Setup(void);
void DAP_Thread(void *argument);
void SWO_Thread();

int fast_reply(uint8_t *buf, uint32_t length);

#endif