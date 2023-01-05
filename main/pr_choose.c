#include "pr_choose.h"
#include "lwip/sys.h"
#include "cJSON.h"

int pr_choose(unsigned char *buffer, unsigned int length)
{
    char rx_buffer[1500];
    for(int i=0; i<length; i++)
    {
        rx_buffer[i] = buffer[i];
    }
    
    if(rx_buffer[0]=='C'&&rx_buffer[1]=='O'&&rx_buffer[2]=='M'&&rx_buffer[3]=='\r'&&rx_buffer[4]=='\n')
    {
        return 1;
    } 
    // else if(rx_buffer[0]=='"'&&rx_buffer[1]=='C'&&rx_buffer[2]=='o'&&rx_buffer[3]=='m'&&rx_buffer[4]=='m'&&rx_buffer[5]=='a'&&rx_buffer[6]=='n'&&rx_buffer[7]=='d'&&rx_buffer[8]=='"')
    // {
    //     return 2;
    // } 
    else{
        return -1;
    }
    
}

int base_choose(const char *buffer, unsigned int length)
{
    cJSON *pcommandItem = NULL;
    cJSON *pcommandObj = NULL;
    char *rx_buffer;
    pcommandObj=cJSON_Parse(buffer);
    cJSON *pArrycommand = cJSON_GetObjectItem(pcommandObj, "command");
    if(!pArrycommand) return;
    else
    {   
    rx_buffer=pArrycommand->valuestring;
    }
    cJSON_Delete(pcommandObj);
    if(rx_buffer[0]==1)
    return 1;
    else
    return 0;    
}

int p_model_choose(const char *buffer, unsigned int length)
{
    cJSON *pattachItem = NULL;
    cJSON *pattachObj = NULL;
    char rx_buffer[500];

    pattachObj=cJSON_Parse(buffer);
    cJSON *pArryattach= cJSON_GetObjectItem(pattachObj, "attach");
    if(!pArryattach)
    { return;}
    else {
        int arryLength = cJSON_GetArraySize(pArryattach);
 
        for( int i = 0; i < arryLength; i++)
        {
            pattachItem = cJSON_GetObjectItem(pArryattach,i);
            if(NULL != pattachItem)
            {
            rx_buffer[i]=pattachItem->valuestring;
            }
        }
    }
    cJSON_Delete(cJSON_Parse(rx_buffer));
     for(int k=0; k<length; k++)
    {
        rx_buffer[k] = buffer[k];
    }
    if(rx_buffer[0]=='1'&&rx_buffer[1]=='0')
    {
        switch (rx_buffer[2])
        {
        case 1://101,DAP
            //DAP
            return 1;
            break;
        case 2://102,UART
            //UART
            return 2;
            break;
        case 3://103,ADC
            //adc
            return 3;
            break;
        case 4://104,DAC

            return 4;
            break;
        case 5://105,PWM_rx

            return 5;
            break;
        case 6://106,PWM_tx
            
            return 6;
            break;
        case 7://107,I2C
        
            return 7;
            break;
        case 8://108,SPI
            
            return 8;
            break;
        case 9://109,CAN
            
            return 9;
            break;
        default:
            return -1;
            break;
        }
    }
    else 
    return -2;
}   

