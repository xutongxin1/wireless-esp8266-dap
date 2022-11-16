#include <string.h>
#include <stdint.h>
#include <sys/param.h>

#include "main/wifi_configuration.h"
#include "main/usbip_server.h"
#include "main/DAP_handle.h"

#include "components/elaphureLink/elaphureLink_protocol.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <errno.h>

#include "cJSON.h"
#include "tcp_server1.h"
#include "Handle.h"

static int Flag1=0;//是否收到COM心跳包
const char *Flag2="OK\r\n";//心跳包发送
static int Flag3=0;//是否发送OK心跳包
static int Command_Flag=0;//指令模式

static const char *TAG = "example";

extern TaskHandle_t kDAPTaskHandle1;
extern int kRestartDAPHandle1;

uint8_t kState1 = ACCEPTING;
int kSock1 = -1;
int written =0;

void tcp_server_task1(void *pvParameters)
{

    char addr_str[128];
    int addr_family;
    int ip_protocol;

    int on = 1;
    while (1)
    {

#ifdef CONFIG_EXAMPLE_IPV4
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        struct sockaddr_in6 destAddr;
        bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
        destAddr.sin6_family = AF_INET6;
        destAddr.sin6_port = htons(PORT);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0)
        {
            os_printf("Unable to create socket: errno %d\r\n", errno);
            break;
        }
        os_printf("Socket created\r\n");

        setsockopt(listen_sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on));
        setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on));

        int err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0)
        {
            os_printf("Socket unable to bind: errno %d\r\n", errno);
            break;
        }
        os_printf("Socket binded\r\n");

        err = listen(listen_sock, 1);
        if (err != 0)
        {
            os_printf("Error occured during listen: errno %d\r\n", errno);
            break;
        }
        os_printf("Socket listening\r\n");

#ifdef CONFIG_EXAMPLE_IPV6
        struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6
#else
        struct sockaddr_in sourceAddr;
#endif
        uint32_t addrLen = sizeof(sourceAddr);
        while (1)
        {

            kSock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
            if (kSock < 0)
            {
                os_printf("Unable to accept connection: errno %d\r\n", errno);
                break;
            }
            setsockopt(kSock, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on));
            setsockopt(kSock, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on));
            os_printf("Socket accepted\r\n");

            while (1)
            {
                char tcp_rx_buffer[1500]={0};
                int len = recv(kSock, tcp_rx_buffer, sizeof(tcp_rx_buffer), 0);
                // Error occured during receiving
                if (len < 0)
                {
                    os_printf("recv failed: errno %d\r\n", errno);
                    break;
                }
                // Connection closed
                else if (len == 0)
                {
                    os_printf("Connection closed\r\n");
                    break;
                }
                // Data received
                else
                {
                    // #ifdef CONFIG_EXAMPLE_IPV6
                    //                     // Get the sender's ip address as string
                    //                     if (sourceAddr.sin6_family == PF_INET)
                    //                     {
                    //                         inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                    //                     }
                    //                     else if (sourceAddr.sin6_family == PF_INET6)
                    //                     {
                    //                         inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                    //                     }
                    // #else
                    //                     inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                    // #endif

                    switch (kState1)
                    {
                    case ACCEPTING:
                        kState = ATTACHING;

                    case ATTACHING:
                        printf("the data is %s\n", tcp_rx_buffer);
                        Heart_beat(len,tcp_rx_buffer);//当COM心跳成功发送，则置Falg为1
                        printf("Flag1=%d,Flag3=%d\n",Flag1,Flag3);
                        if(Flag3==1)
                        {
                            printf("\nCommand analysis!!!\n");
                            printf("the data is %s\n", tcp_rx_buffer);
                            printf("Flag1=%d,Flag3=%d\n",Flag1,Flag3);
                            Command_analysis(len,tcp_rx_buffer);

                        }
                        if(Flag1==1)//确认接收到COM心跳
                        {
                            written = send(kSock, Flag2, strlen(Flag2)-1, 0);
                            Flag3=1;//确认好心跳OK发送完整
                            Flag1=0;
                            printf("Flag1=%d,Flag3=%d\n",Flag1,Flag3);
                        }

                        //attach(tcp_rx_buffer, len);
                       break;

                    
                    default:
                        os_printf("unkonw kstate!\r\n");
                    }
                }
            }
            // kState = ACCEPTING;
            if (kSock != -1)
            {
                os_printf("Shutting down socket and restarting...\r\n");
                //shutdown(kSock, 0);
                close(kSock);
                if (kState == EMULATING || kState == EL_DATA_PHASE)
                    kState = ACCEPTING;

                // Restart DAP Handle
                el_process_buffer_free();

                kRestartDAPHandle1 = RESET_HANDLE;
                if (kDAPTaskHandle1)
                    xTaskNotifyGive(kDAPTaskHandle1);

                //shutdown(listen_sock, 0);
                //close(listen_sock);
                //vTaskDelay(5);
            }
        }
    }
    vTaskDelete(NULL);
}

void Heart_beat(unsigned int len,char *rx_buffer){
    const char *p1="COM\r\n";
    if(rx_buffer!=NULL){
        if(strstr(rx_buffer,p1))
        {Flag1=1;}
    }
} 

void Command_analysis(unsigned int len,void *rx_buffer)
{
    char *strattach=NULL;
    int *strcommand=NULL;
    char str_attach;
    int str_command;
    //首先整体判断是否为一个json格式的数据
    cJSON *pJsonRoot = cJSON_Parse(rx_buffer);
    cJSON *pcommand = cJSON_GetObjectItem(pJsonRoot, "command");      // 解析command字段内容
    cJSON *pattach = cJSON_GetObjectItem(pJsonRoot, "attach");      // 解析attach字段内容
    printf("\nIn analysis\n");
    //是否为json格式数据
    if (pJsonRoot !=NULL)
    {
        printf("\nlife1\n");
        //是否指令为空
        if(pcommand!=NULL)
        {
            printf("\nlife2\n");
            //是否附加信息为空
            if(pattach!=NULL)
            {
                printf("\nlife3\n");
                strcommand=pcommand->valueint;
                strattach=pattach->valuestring;
                str_attach=(*strattach);
                str_command=(*strcommand);
                //printf("\nthe data is %d",str_command);
                printf("\nthe data is %c",str_attach);
                attach_status(str_attach);
                cJSON_Delete(pJsonRoot);
                printf("\nfree\n");
                Flag3=0;
            }
        }
    }
    
    
}


void attach_status(char str_attach)
{
    int attach=(int)str_attach;
    attach=attach-'0';
    switch (attach)
    {
    case DAP:
        DAP_Handle();
        break;
    case UART:
        UART_Handle();
        break;
    case ADC:
        ADC_Handle();
        break;
    case DAC:
        DAC_Handle();
        break;
    case PWM_Collect:
        PWM_Collect_Handle();
        break;            
    case PWM_Simulation:
        PWM_Simulation_Handle();
        break;  
    case I2C:
        I2C_Handle();
        break;  
    case SPI:
        SPI_Handle();
        break;
    case CAN:
        CAN_Handle();
        break;        
    default:
        break;
    }
}

