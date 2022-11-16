#ifndef __DAP_INIT_H__
#define __DAP_INIT_H__

extern void DAP_Setup(void);
extern void DAP_Thread(void *argument);
extern void SWO_Thread();



static const char *MDNS_TAG = "server_common";

void mdns_setup(void);

void Dap_Init(void);

#endif