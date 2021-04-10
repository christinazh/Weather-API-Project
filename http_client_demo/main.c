//*****************************************************************************
// Copyright (C) 2014 Texas Instruments Incorporated
//
// All rights reserved. Property of Texas Instruments Incorporated.
// Restricted rights to use, duplicate or disclose this code are
// granted through contract.
// The program may not be used without the written permission of
// Texas Instruments Incorporated or against the terms and conditions
// stipulated in the agreement under which this program has been supplied,
// and under no circumstances can it be used with non-TI connectivity device.
//
//*****************************************************************************


//*****************************************************************************
//
// Application Name     - HTTP Client Demo
// Application Overview - This sample application demonstrates how to use
//                          HTTP Client (In Minimum mode) API for HTTP based
//                          application development.
//                          This application explain user to how to:
//                          1. Connect to an access point
//                          2. Connect to a HTTP Server with and without proxy
//                          3. Do POST, GET, PUT and DELETE
//                          4. Parse JSON data using “Jasmine JSON Parser”
// Note: To use HTTP Client in minimum mode, user need to compile library (webclient)
// 			with HTTPCli_LIBTYPE_MIN option.
//
// 			HTTP Client (minimal) library supports synchronous mode, redirection
// 			handling, chunked transfer encoding support, proxy support and TLS
// 			support (for SimpleLink Only. TLS on other platforms are disabled)
//
// 			HTTP Client (Full) library supports all the features of the minimal
// 			library + asynchronous mode and content handling support +
// 			TLS support (all platforms). To use HTTP Client in full mode user need
//			to compile library (webclient) with HTTPCli_LIBTYPE_MIN option. For full
//			mode RTOS is needed.
//
//*****************************************************************************


#include <string.h>


// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include <stdint.h>
#include "stdio.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"

//Common interface includes
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "prcm.h"
#include "utils.h"
#include "pin_mux_config.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"
#include "i2c_if.h"

#include "Adafruit_SSD1351.h"
#include "Adafruit_GFX.h"
#include "glcdfont.h"
#include "spi.h"
#include <http/client/httpcli.h>

#include "pin.h"
#include "gpio.h"
#include "prcm.h"

// JSON Parser
#include "jsmn.h"

#define MASTER_MODE      1

#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100
#define I2C_MASTER_MODE_FST     1


#define MAX_URI_SIZE 128
#define URI_SIZE MAX_URI_SIZE + 1

#define POSTHEADER "POST /things/CC3200_Thing/shadow HTTP/1.1\n\r"
#define GETHEADER "GET /things/CC3200_Thing/shadow HTTP/1.1\n\r"

#define HOSTHEADER "Host: a38zrmdxq4r7zx-ats.iot.us-west-2.amazonaws.com\r\n"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"
#define DATA0 "{\"state\": {\n\r\"desired\" : {\n\r\"default\" : \"It's nice and clear outside!!\"\n\r}}}\n\r\n\r"
#define DATA1 "{\"state\":{\n\r\"desired\" : {\n\r\"default\" : \"It's currently cloudy outside!!\"\n\r}}}\n\r\n\r"
#define DATA2 "{\"state\":{\n\r\"desired\" : {\n\r\"default\" : \"It's currently raining outside!!Don't forget an umbrella!\"\n\r}}}\n\r\n\r"

#define APPLICATION_NAME        "SSL"
#define APPLICATION_VERSION     "1.1.1.EEC.Spring2020"
#define SERVER_NAME                "a38zrmdxq4r7zx-ats.iot.us-west-2.amazonaws.com"
#define GOOGLE_DST_PORT             8443

#define SL_SSL_CA_CERT "/cert/rootCA.der"
#define SL_SSL_PRIVATE "/cert/private.der"
#define SL_SSL_CLIENT  "/cert/client.der"

//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                12    /* Current Date */
#define MONTH               3    /* Month 1-12 */
#define YEAR                2021  /* Current year */
#define HOUR                15    /* Time - hours */
#define MINUTE              06    /* Time - minutes */
#define SECOND              0     /* Time - seconds */

#define APPLICATION_VERSION "1.4.0"
#define APP_NAME            "HTTP Client"

#define POST_REQUEST_URI 	"/data/2.5/weather?q=Davis&units=imperial&APPID=9258a3db688461c8373afc108f08181f"
#define POST_DATA           "{\n\"name\":\"xyz\",\n\"address\":\n{\n\"plot#\":12,\n\"street\":\"abc\",\n\"city\":\"ijk\"\n},\n\"age\":30\n}"

#define DELETE_REQUEST_URI 	"/delete"


#define PUT_REQUEST_URI 	"/put"
#define PUT_DATA            "PUT request."

#define GET_REQUEST_URI1 	"/data/2.5/weather?q=Davis&units=imperial&APPID=9258a3db688461c8373afc108f08181f"
#define GET_REQUEST_URI2    "/data/2.5/weather?q=Sacramento&units=imperial&APPID=9258a3db688461c8373afc108f08181f"
#define GET_REQUEST_URI3    "/data/2.5/weather?q=tokyo,jp&units=imperial&appid=9258a3db688461c8373afc108f08181f"
#define GET_REQUEST_URI4    "/data/2.5/weather?q=San%20Jose&units=imperial&appid=9258a3db688461c8373afc108f08181f"

bool sacramento = false;
bool tokyo;
bool davis;
bool sanjose;

#define HOST_NAME       	"api.openweathermap.org" //"<host name>"
#define HOST_PORT           80

#define PROXY_IP       	    <proxy_ip>
#define PROXY_PORT          <proxy_port>

#define READ_SIZE           1450
#define MAX_BUFF_SIZE       1460

char acRecvbuff[1460];   // Buffer to receive data

// Application specific status/error codes
typedef enum{
 /* Choosing this number to avoid overlap with host-driver's error codes */
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,       
    DEVICE_START_FAILED = DEVICE_NOT_IN_STATION_MODE - 1,
    INVALID_HEX_STRING = DEVICE_START_FAILED - 1,
    TCP_RECV_ERROR = INVALID_HEX_STRING - 1,
    TCP_SEND_ERROR = TCP_RECV_ERROR - 1,
    FILE_NOT_FOUND_ERROR = TCP_SEND_ERROR - 1,
    INVALID_SERVER_RESPONSE = FILE_NOT_FOUND_ERROR - 1,
    FORMAT_NOT_SUPPORTED = INVALID_SERVER_RESPONSE - 1,
    FILE_OPEN_FAILED = FORMAT_NOT_SUPPORTED - 1,
    FILE_WRITE_ERROR = FILE_OPEN_FAILED - 1,
    INVALID_FILE = FILE_WRITE_ERROR - 1,
    SERVER_CONNECTION_FAILED = INVALID_FILE - 1,
    GET_HOST_IP_FAILED = SERVER_CONNECTION_FAILED  - 1,
    
    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

typedef struct
{
   /* time */
   unsigned long tm_sec;
   unsigned long tm_min;
   unsigned long tm_hour;
   /* date */
   unsigned long tm_day;
   unsigned long tm_mon;
   unsigned long tm_year;
   unsigned long tm_week_day; //not required
   unsigned long tm_year_day; //not required
   unsigned long reserved[3];
}SlDateTime;
//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulDestinationIP; // IP address of destination server
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
unsigned char g_buff[MAX_BUFF_SIZE+1];
long bytesReceived = 0; // variable to store the file size

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

static int HandleXMLData(char *acRecvbuff, char ** data_table);

static long WlanConnect();
static int set_time();
static int tls_connect();
static int connectToAccessPoint();
static int http_post(int iTLSSockID, int state);
SlDateTime g_time;
signed char    *g_Host = SERVER_NAME;

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************

static int set_time() {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}
int connectToAccessPoint() {
    long lRetVal = -1;
    //GPIO_IF_LedConfigure(LED1|LED3);

   // GPIO_IF_LedOff(MCU_RED_LED_GPIO);
   // GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);

    lRetVal = InitializeAppVariables();
    ASSERT_ON_ERROR(lRetVal);

    //
    // Following function configure the device to default state by cleaning
    // the persistent settings stored in NVMEM (viz. connection profiles &
    // policies, power policy etc)
    //
    // Applications may choose to skip this step if the developer is sure
    // that the device is in its default state at start of applicaton
    //
    // Note that all profiles and persistent settings that were done on the
    // device will be lost
    //
    lRetVal = ConfigureSimpleLinkToDefaultState();
    if(lRetVal < 0) {
      if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
          UART_PRINT("Failed to configure the device in its default state \n\r");

      return lRetVal;
    }

    UART_PRINT("Device is configured in default state \n\r");

    CLR_STATUS_BIT_ALL(g_ulStatus);

    ///
    // Assumption is that the device is configured in station mode already
    // and it is in its default state
    //
    lRetVal = sl_Start(0, 0, 0);
    if (lRetVal < 0 || ROLE_STA != lRetVal) {
        UART_PRINT("Failed to start the device \n\r");
        return lRetVal;
    }

    UART_PRINT("Device started as STATION \n\r");

    //
    //Connecting to WLAN AP
    //
    lRetVal = WlanConnect();
    if(lRetVal < 0) {
        UART_PRINT("Failed to establish connection w/ an AP \n\r");
        //GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    UART_PRINT("Connection established w/ AP and IP is aquired \n\r");
    return 0;
}
long printErrConvenience(char * msg, long retVal) {
    UART_PRINT(msg);
    //GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    return retVal;
}
static int tls_connect() {
    SlSockAddrIn_t    Addr;
    int    iAddrSize;
    unsigned char    ucMethod = SL_SO_SEC_METHOD_TLSV1_2;
    unsigned int uiIP,uiCipher = SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA;
    long lRetVal = -1;
    int iSockID;

    lRetVal = sl_NetAppDnsGetHostByName(g_Host, strlen((const char *)g_Host),
                                    (unsigned long*)&uiIP, SL_AF_INET);

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't retrieve the host name \n\r", lRetVal);
    }

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(GOOGLE_DST_PORT);
    Addr.sin_addr.s_addr = sl_Htonl(uiIP);
    iAddrSize = sizeof(SlSockAddrIn_t);
    //
    // opens a secure socket
    //
    iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
    if( iSockID < 0 ) {
        return printErrConvenience("Device unable to create secure socket \n\r", lRetVal);
    }

    //
    // configure the socket as TLS1.2
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_SECMETHOD, &ucMethod,\
                               sizeof(ucMethod));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }
    //
    //configure the socket as ECDHE RSA WITH AES256 CBC SHA
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &uiCipher,\
                           sizeof(uiCipher));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    //
    //configure the socket with CA certificate - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
                           SL_SO_SECURE_FILES_CA_FILE_NAME, \
                           SL_SSL_CA_CERT, \
                           strlen(SL_SSL_CA_CERT));

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    //configure the socket with Client Certificate - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
                SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME, \
                                    SL_SSL_CLIENT, \
                           strlen(SL_SSL_CLIENT));

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    //configure the socket with Private Key - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
            SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME, \
            SL_SSL_PRIVATE, \
                           strlen(SL_SSL_PRIVATE));

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }


    /* connect to the peer device - Google server */
    lRetVal = sl_Connect(iSockID, ( SlSockAddr_t *)&Addr, iAddrSize);

    if(lRetVal < 0) {
        UART_PRINT("Device couldn't connect to server:");
        UART_PRINT(SERVER_NAME);
        UART_PRINT("\n\r");
        return printErrConvenience("Device couldn't connect to server \n\r", lRetVal);
    }
    else {
        UART_PRINT("Device has connected to the website:");
        UART_PRINT(SERVER_NAME);
        UART_PRINT("\n\r");
    }

    //GPIO_IF_LedOff(MCU_RED_LED_GPIO);
   // GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    return iSockID;
}

//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'-Applications
            // can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s ,"
                            " BSSID: %x:%x:%x:%x:%x:%x\n\r",
                      g_ucConnectionSSID,g_ucConnectionBSSID[0],
                      g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                      g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                      g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION
            if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s,"
                "BSSID: %x:%x:%x:%x:%x:%x on application's request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s,"
                "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            //Gateway IP address
            g_ulGatewayIP = pEventData->gateway;

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
            "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
        }
        break;

        default:
        {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}


//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
    // Unused in this application
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    //
    // This application doesn't work w/ socket - Events are not expected
    //
       switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
        	switch( pSock->socketAsyncEvent.SockTxFailData.status )
            {
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                    "failed to transmit all queued packets\n\n",
                           pSock->socketAsyncEvent.SockAsyncData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED : socket %d , reason"
                        "(%d) \n\n",
                        pSock->socketAsyncEvent.SockAsyncData.sd,
                        pSock->socketAsyncEvent.SockTxFailData.status);
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
    }
}


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************



//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param    None
//!
//! \return None
//!
//*****************************************************************************
static void InitializeAppVariables()
{
    g_ulStatus = 0;
    g_ulGatewayIP = 0;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
}


//*****************************************************************************
//! \brief This function puts the device in its default state. It:
//!           - Set the mode to STATION
//!           - Configures connection policy to Auto and AutoSmartConfig
//!           - Deletes all the stored profiles
//!           - Enables DHCP
//!           - Disables Scan policy
//!           - Sets Tx power to maximum
//!           - Sets power policy to normal
//!           - Unregister mDNS services
//!           - Remove all filters
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//*****************************************************************************
static long ConfigureSimpleLinkToDefaultState()
{
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode 
    if (ROLE_STA != lMode)
    {
        if (ROLE_AP == lMode)
        {
            // If the device is in AP mode, we need to wait for this event 
            // before doing anything 
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
            }
        }

        // Switch to STA role and restart 
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again 
        if (ROLE_STA != lRetVal)
        {
            // We don't want to proceed if the device is not coming up in STA-mode 
            return DEVICE_NOT_IN_STATION_MODE;
        }
    }
    
    // Get the device's version-information
    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt, 
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);
    
    UART_PRINT("Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT("Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
    ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
    ver.ChipFwAndPhyVersion.FwVersion[0],ver.ChipFwAndPhyVersion.FwVersion[1],
    ver.ChipFwAndPhyVersion.FwVersion[2],ver.ChipFwAndPhyVersion.FwVersion[3],
    ver.ChipFwAndPhyVersion.PhyVersion[0],ver.ChipFwAndPhyVersion.PhyVersion[1],
    ver.ChipFwAndPhyVersion.PhyVersion[2],ver.ChipFwAndPhyVersion.PhyVersion[3]);

    // Set connection policy to Auto + SmartConfig 
    //      (Device's default connection policy)
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, 
                                SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove all profiles
    lRetVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(lRetVal);

    //
    // Device in station-mode. Disconnect previous connection if any
    // The function returns 0 if 'Disconnected done', negative number if already
    // disconnected Wait for 'disconnection' event if 0 is returned, Ignore 
    // other return-codes
    //
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal)
    {
        // Wait
        while(IS_CONNECTED(g_ulStatus))
        {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
        }
    }

    // Enable DHCP client
    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Set Tx power level for station mode
    // Number between 0-15, as dB offset from max power - 0 will set max power
    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, 
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);

    // Set PM policy to normal
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Unregister mDNS services
    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove  all 64 filters (8*8)
    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);


    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    InitializeAppVariables();

    return SUCCESS;
}



//****************************************************************************
//
//! \brief Connecting to a WLAN Accesspoint
//!
//!  This function connects to the required AP (SSID_NAME) with Security
//!  parameters specified in te form of macros at the top of this file
//!
//! \param  Status value
//!
//! \return  None
//!
//! \warning    If the WLAN connection fails or we don't aquire an IP
//!            address, It will be stuck in this function forever.
//
//****************************************************************************
static long WlanConnect()
{
    SlSecParams_t secParams = {0};
    long lRetVal = 0;

    secParams.Key = (signed char *)SECURITY_KEY;
    secParams.KeyLen = strlen(SECURITY_KEY);
    secParams.Type = SECURITY_TYPE;

    lRetVal = sl_WlanConnect((signed char *)SSID_NAME,
                           strlen((const char *)SSID_NAME), 0, &secParams, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Wait for WLAN Event
    while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))
    {
        // wait till connects to an AP
        _SlNonOsMainLoopTask();
    }

    return SUCCESS;

}


//*****************************************************************************
//
//! \brief Flush response body.
//!
//! \param[in]  httpClient - Pointer to HTTP Client instance
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int FlushHTTPResponse(HTTPCli_Handle httpClient)
{
    const char *ids[2] = {
                            HTTPCli_FIELD_NAME_CONNECTION, /* App will get connection header value. all others will skip by lib */
                            NULL
                         };
    char buf[128];
    int id;
    int len = 1;
    bool moreFlag = 0;
    char ** prevRespFilelds = NULL;


    /* Store previosly store array if any */
    prevRespFilelds = HTTPCli_setResponseFields(httpClient, ids);

    /* Read response headers */
    while ((id = HTTPCli_getResponseField(httpClient, buf, sizeof(buf), &moreFlag))
            != HTTPCli_FIELD_ID_END)
    {

        if(id == 0)
        {
            if(!strncmp(buf, "close", sizeof("close")))
            {
                UART_PRINT("Connection terminated by server\n\r");
            }
        }

    }

    /* Restore previosuly store array if any */
    HTTPCli_setResponseFields(httpClient, (const char **)prevRespFilelds);

    while(1)
    {
        /* Read response data/body */
        /* Note:
                moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
                data is available Or in other words content length > length of buffer.
                The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
                Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
                for more information.
        */
        HTTPCli_readResponseBody(httpClient, buf, sizeof(buf) - 1, &moreFlag);
        ASSERT_ON_ERROR(len);

        if ((len - 2) >= 0 && buf[len - 2] == '\r' && buf [len - 1] == '\n'){
            break;
        }

        if(!moreFlag)
        {
            /* There no more data. break the loop. */
            break;
        }
    }
    return 0;
}


//*****************************************************************************
//
//! \brief Handler for parsing JSON data
//!
//! \param[in]  ptr - Pointer to http response body data
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int ParseJSONData(char *ptr)
{
	long lRetVal = 0;
    int noOfToken;
    jsmn_parser parser;
    jsmntok_t   *tokenList;


    /* Initialize JSON PArser */
    jsmn_init(&parser);

    /* Get number of JSON token in stream as we we dont know how many tokens need to pass */
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), NULL, 10);
    if(noOfToken <= 0)
    {
    	UART_PRINT("Failed to initialize JSON parser\n\r");
    	return -1;

    }

    /* Allocate memory to store token */
    tokenList = (jsmntok_t *) malloc(noOfToken*sizeof(jsmntok_t));
    if(tokenList == NULL)
    {
        UART_PRINT("Failed to allocate memory\n\r");
        return -1;
    }

    /* Initialize JSON Parser again */
    jsmn_init(&parser);
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), tokenList, noOfToken);
    if(noOfToken < 0)
    {
    	UART_PRINT("Failed to parse JSON tokens\n\r");
    	lRetVal = noOfToken;
    }
    else
    {
    	UART_PRINT("Successfully parsed %ld JSON tokens\n\r", noOfToken);
    }

    free(tokenList);

    return lRetVal;
}

/*!
    \brief This function read respose from server and dump on console

    \param[in]      httpClient - HTTP Client object

    \return         0 on success else -ve

    \note

    \warning
*/
static int readResponse(HTTPCli_Handle httpClient,char ** data_table)
{
	long lRetVal = 0;
	int bytesRead = 0;
	int id = 0;
	unsigned long len = 0;
	int json = 0;
	char *dataBuffer=NULL;
	bool moreFlags = 1;
	const char *ids[4] = {
	                        HTTPCli_FIELD_NAME_CONTENT_LENGTH,
			                HTTPCli_FIELD_NAME_CONNECTION,
			                HTTPCli_FIELD_NAME_CONTENT_TYPE,
			                NULL
	                     };

	/* Read HTTP POST request status code */
	lRetVal = HTTPCli_getResponseStatus(httpClient);
	if(lRetVal > 0)
	{
		switch(lRetVal)
		{
		case 200:
		{
			UART_PRINT("HTTP Status 200\n\r");
			/*
                 Set response header fields to filter response headers. All
                  other than set by this call we be skipped by library.
			 */
			HTTPCli_setResponseFields(httpClient, (const char **)ids);

			/* Read filter response header and take appropriate action. */
			/* Note:
                    1. id will be same as index of fileds in filter array setted
                    in previous HTTPCli_setResponseFields() call.

                    2. moreFlags will be set to 1 by HTTPCli_getResponseField(), if  field
                    value could not be completely read. A subsequent call to
                    HTTPCli_getResponseField() will read remaining field value and will
                    return HTTPCli_FIELD_ID_DUMMY. Please refer HTTP Client Libary API
                    documenation @ref HTTPCli_getResponseField for more information.
			 */
			while((id = HTTPCli_getResponseField(httpClient, (char *)g_buff, sizeof(g_buff), &moreFlags))
					!= HTTPCli_FIELD_ID_END)
			{

				switch(id)
				{
				case 0: /* HTTPCli_FIELD_NAME_CONTENT_LENGTH */
				{
					len = strtoul((char *)g_buff, NULL, 0);
				}
				break;
				case 1: /* HTTPCli_FIELD_NAME_CONNECTION */
				{
				}
				break;
				case 2: /* HTTPCli_FIELD_NAME_CONTENT_TYPE */
				{
					if(!strncmp((const char *)g_buff, "application/json",
							sizeof("application/json")))
					{
						json = 1;
					}
					else
					{
						/* Note:
                                Developers are advised to use appropriate
                                content handler. In this example all content
                                type other than json are treated as plain text.
						 */
						json = 0;
					}
					UART_PRINT(HTTPCli_FIELD_NAME_CONTENT_TYPE);
					UART_PRINT(" : ");
					UART_PRINT("application/json\n\r");
                    json = 1;

				}
				break;
				default:
				{
					UART_PRINT("Wrong filter id\n\r");
					lRetVal = -1;
					goto end;
				}
				}
			}
			bytesRead = 0;
			if(len > sizeof(g_buff))
			{
				dataBuffer = (char *) malloc(len);
				if(dataBuffer)
				{
					UART_PRINT("Failed to allocate memory\n\r");
					lRetVal = -1;
					goto end;
				}
			}
			else
			{
				dataBuffer = (char *)g_buff;
			}

			/* Read response data/body */
			/* Note:
                    moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
		            data is available Or in other words content length > length of buffer.
		            The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
		            Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
		            for more information

			 */
			bytesRead = HTTPCli_readResponseBody(httpClient, (char *)dataBuffer, len, &moreFlags);
			if(bytesRead < 0)
			{
				UART_PRINT("Failed to received response body\n\r");
				lRetVal = bytesRead;
				goto end;
			}
			else if( bytesRead < len || moreFlags)
			{
				UART_PRINT("Mismatch in content length and received data length\n\r");
				goto end;
			}
			dataBuffer[bytesRead] = '\0';

			if(json)
			{
				/* Parse JSON data */
				lRetVal = ParseJSONData(dataBuffer);
				//UART_PRINT(dataBuffer);
				HandleXMLData(dataBuffer,data_table);

				if(lRetVal < 0)
				{
					goto end;
				}
			}
			else
			{
				/* treating data as a plain text */
			}

		}
		break;

		case 404:
			UART_PRINT("File not found. \r\n");
			/* Handle response body as per requirement.
                  Note:
                    Developers are advised to take appopriate action for HTTP
                    return status code else flush the response body.
                    In this example we are flushing response body in default
                    case for all other than 200 HTTP Status code.
			 */
		default:
			/* Note:
              Need to flush received buffer explicitly as library will not do
              for next request.Apllication is responsible for reading all the
              data.
			 */
			FlushHTTPResponse(httpClient);
			break;
		}
	}
	else
	{
		UART_PRINT("Failed to receive data from server.\r\n");
		goto end;
	}

	lRetVal = 0;

end:
    if(len > sizeof(g_buff) && (dataBuffer != NULL))
	{
	    free(dataBuffer);
    }
    return lRetVal;
}

//*****************************************************************************
//
//! \brief HTTP POST Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int HTTPPostMethod(HTTPCli_Handle httpClient,char ** data_table)
{
    bool moreFlags = 1;
    bool lastFlag = 1;
    char tmpBuf[4];
    long lRetVal = 0;
    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {HTTPCli_FIELD_NAME_CONTENT_TYPE, "application/json"},
                                {NULL, NULL}
                            };
    

    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send POST method request. */
    /* Here we are setting moreFlags = 1 as there are some more header fields need to send
       other than setted in previous call HTTPCli_setRequestFields() at later stage.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
    */
    moreFlags = 1;
    //lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_POST, POST_REQUEST_URI, moreFlags); //POST_REQUEST_URI
    if(davis == true){
        lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_PUT, GET_REQUEST_URI1, moreFlags); //PUT_REQUEST_URI
    }else if(sacramento== true){
        lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_PUT, GET_REQUEST_URI2, moreFlags); //PUT_REQUEST_URI
    }else if(tokyo== true){
        lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_PUT, GET_REQUEST_URI3, moreFlags); //PUT_REQUEST_URI
    }else if(sanjose== true){
        lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_PUT, GET_REQUEST_URI4, moreFlags); //PUT_REQUEST_URI

    }
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP POST request header.\n\r");
        return lRetVal;
    }

    sprintf((char *)tmpBuf, "%d", (sizeof(POST_DATA)-1));

    /* Here we are setting lastFlag = 1 as it is last header field.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
    */
    lastFlag = 1;
    lRetVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (const char *)tmpBuf, lastFlag);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP POST request header.\n\r");
        return lRetVal;
    }


    /* Send POST data/body */
    lRetVal = HTTPCli_sendRequestBody(httpClient, POST_DATA, (sizeof(POST_DATA)-1));
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP POST request body.\n\r");
        return lRetVal;
    }


    lRetVal = readResponse(httpClient,data_table);

    return lRetVal;
}


//*****************************************************************************
//
//! \brief HTTP DELETE Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int HTTPDeleteMethod(HTTPCli_Handle httpClient,char **data_table)
{
  
    long lRetVal = 0;
    HTTPCli_Field fields[3] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {NULL, NULL}
                            };
    bool moreFlags;


    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send DELETE method request. */
    /* Here we are setting moreFlags = 0 as there are no more header fields need to send
       at later stage. Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest
       for more information.
    */
    moreFlags = 0;
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_DELETE, DELETE_REQUEST_URI, moreFlags);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP DELETE request header.\n\r");
        return lRetVal;
    }

    lRetVal = readResponse(httpClient,data_table);

    return lRetVal;
}

//*****************************************************************************
//
//! \brief HTTP PUT Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int HTTPPutMethod(HTTPCli_Handle httpClient,char ** data_table)
{
  
    long lRetVal = 0;
    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {HTTPCli_FIELD_NAME_CONTENT_TYPE, "text/html"},
                                {NULL, NULL}
                            };
    bool        moreFlags = 1;
    bool        lastFlag = 1;
    char        tmpBuf[4];
    
    
    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send PUT method request. */
    /* Here we are setting moreFlags = 1 as there are some more header fields need to send
       other than setted in previous call HTTPCli_setRequestFields() at later stage.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
    */
    moreFlags = 1;
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_PUT, PUT_REQUEST_URI, moreFlags); //PUT_REQUEST_URI

    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP PUT request header.\n\r");
        return lRetVal;
    }

    sprintf((char *)tmpBuf, "%d", (sizeof(PUT_DATA)-1));

    /* Here we are setting lastFlag = 1 as it is last header field.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
    */
    lastFlag = 1;
    lRetVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (char *)tmpBuf, lastFlag);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP PUT request header.\n\r");
        return lRetVal;
    }

    /* Send PUT data/body */
    lRetVal = HTTPCli_sendRequestBody(httpClient, PUT_DATA, (sizeof(PUT_DATA)-1));
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP PUT request body.\n\r");
        return lRetVal;
    }

    lRetVal = readResponse(httpClient,data_table);

    return lRetVal;
}

//*****************************************************************************
//
//! \brief HTTP GET Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int HTTPGetMethod(HTTPCli_Handle httpClient,char** data_table)
{
  
    long lRetVal = 0;
    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {HTTPCli_FIELD_NAME_CONTENT_LENGTH, "0"},
                                {NULL, NULL}
                            };
    bool        moreFlags;
    
    
    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send GET method request. */
    /* Here we are setting moreFlags = 0 as there are no more header fields need to send
       at later stage. Please refer HTTP Library API documentaion @ HTTPCli_sendRequest
       for more information.
    */
    moreFlags = 0;
    if(davis == true){
          lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, GET_REQUEST_URI1, moreFlags); //GET_REQUEST_URI
      }else if(sacramento== true){
          lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, GET_REQUEST_URI2, moreFlags); //GET_REQUEST_URI
      }else if(tokyo== true){
          lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, GET_REQUEST_URI3, moreFlags); //GET_REQUEST_URI
      }else if(sanjose== true){
          lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, GET_REQUEST_URI4, moreFlags); //GET_REQUEST_URI

      }
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP GET request.\n\r");
        return lRetVal;
    }


    lRetVal = readResponse(httpClient,data_table);

    return lRetVal;
}


//*****************************************************************************
//
//! Function to connect to AP
//!
//! \param  none
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************
static long ConnectToAP()
{
    long lRetVal = -1;
    
    //
    // Following function configure the device to default state by cleaning
    // the persistent settings stored in NVMEM (viz. connection profiles &
    // policies, power policy etc)
    //
    // Applications may choose to skip this step if the developer is sure
    // that the device is in its desired state at start of applicaton
    //
    // Note that all profiles and persistent settings that were done on the
    // device will be lost
    //
    lRetVal = ConfigureSimpleLinkToDefaultState();
    if(lRetVal < 0)
    {
        if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
        {
            UART_PRINT("Failed to configure the device in its default state, "
                            "Error-code: %d\n\r", DEVICE_NOT_IN_STATION_MODE);
        }

        return -1;
    }

    UART_PRINT("Device is configured in default state \n\r");

    //
    // Assumption is that the device is configured in station mode already
    // and it is in its default state
    //
    lRetVal = sl_Start(0, 0, 0);
    if (lRetVal < 0 || ROLE_STA != lRetVal)
    {
        ASSERT_ON_ERROR(DEVICE_START_FAILED);
    }

    UART_PRINT("Device started as STATION \n\r");

    // Connecting to WLAN AP - Set with static parameters defined at the top
    // After this call we will be connected and have IP address
    lRetVal = WlanConnect();

    UART_PRINT("Connected to the AP: %s\r\n", SSID_NAME);
    return 0;
}

//*****************************************************************************
//
//! Function to connect to HTTP server
//!
//! \param  httpClient - Pointer to HTTP Client instance
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************
static int ConnectToHTTPServer(HTTPCli_Handle httpClient)
{
    long lRetVal = -1;
    struct sockaddr_in addr;
  

#ifdef USE_PROXY
    struct sockaddr_in paddr;
    paddr.sin_family = AF_INET;
    paddr.sin_port = htons(PROXY_PORT);
    paddr.sin_addr.s_addr = sl_Htonl(PROXY_IP);
    HTTPCli_setProxy((struct sockaddr *)&paddr);
#endif
    
    /* Resolve HOST NAME/IP */
    lRetVal = sl_NetAppDnsGetHostByName((signed char *)HOST_NAME,
                                          strlen((const char *)HOST_NAME),
                                          &g_ulDestinationIP,SL_AF_INET);
    if(lRetVal < 0)
    {
        ASSERT_ON_ERROR(GET_HOST_IP_FAILED);
    }

    /* Set up the input parameters for HTTP Connection */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HOST_PORT);
    addr.sin_addr.s_addr = sl_Htonl(g_ulDestinationIP);

    /* Testing HTTPCli open call: handle, address params only */
    HTTPCli_construct(httpClient);
    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, 0, NULL);
    if (lRetVal < 0)
    {
        UART_PRINT("Connection to server failed. error(%d)\n\r", lRetVal);
        ASSERT_ON_ERROR(SERVER_CONNECTION_FAILED);
    }    
    else
    {
        UART_PRINT("Connection to server created successfully\r\n");
    }

    return 0;
}

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{
    UART_PRINT("\n\n\n\r");
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\t\t      CC3200 %s Application       \n\r", AppName);
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\n\n\n\r");
}


//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}
extern uint8_t sun1[];
extern uint8_t sun2[];
extern uint8_t sun1inv[];
extern uint8_t sun2inv[];
extern uint8_t eyebrow1[];
extern uint8_t eyebrow2[];
extern uint8_t cloud1[];
extern uint8_t cloud2[];
extern uint8_t ogcloud[];
extern uint8_t rain1[];
extern uint8_t rain2[];
extern uint8_t ograin[];
int flag = 1;
void program(    HTTPCli_Struct httpClient)
{
    char** data_table = malloc(10 * sizeof(char*));
    //long lRetVal = -1;
    long lRetVal1 = -1;
    int state = 0;

    unsigned char ucRegOffset; //offset char
    unsigned char aucRdDataBuf[256]; //create array
    unsigned char ucDevAddr = 0x18; //address is 0x18
    unsigned char ucRdLen = 6; //6 bytes, reads reg 0x2 to 0x7
    I2C_IF_Write(ucDevAddr,&ucRegOffset,4,0); //slave device address is 0x18
    I2C_IF_Read(ucDevAddr, &aucRdDataBuf[0], ucRdLen);
    UART_PRINT("\n\r");

    fillScreen(0x0000); //fill the OLED with black
    setCursor(5,0);
    Outstr ("Davis, CA");
    setCursor(5,10);
    Outstr ("Sacramento, CA");
    setCursor(5,20);
    Outstr ("Tokyo, Japan");
    setCursor(5,30);
    Outstr ("San Jose, CA");

    int x = 2;
    int y = 2;
    drawCircle(x, y, 1, 0xffff);
    while(1){
        delay(50);  //delay
        I2C_IF_Write(ucDevAddr,&ucRegOffset,4,0);
        I2C_IF_Read(ucDevAddr, &aucRdDataBuf[0], ucRdLen);
        drawCircle(x, y, 1, 0x0000); //remove the old circle
        if(aucRdDataBuf[3] > 35 && aucRdDataBuf[3] < 50){
            if (y>2){
              y -= 10;
            }else{
              y = 2;
            }
        }
        if(aucRdDataBuf[3] < 218 && aucRdDataBuf[3] > 200){
           if (y<30){//check if it in in the boundaries
              y += 10;//if in boundaries, move y value
           }

        }
        drawCircle(x, y, 1, 0xffff); //draw the new circle
/*
        if (GPIOPinRead(GPIOA2_BASE,0x40) && 0x40){ //check if SW[2] is pressed
            PinModeSet(PIN_04, PIN_MODE_0);
            drawCircle(x, y, 1, 0x0000); //remove the old circle
            if (y<30){
                y += 10;
            }else{
                y = 2;
            }
            drawCircle(x, y, 1, 0xffff); //draw the old circle
            Message("\t\t SW[2] \n\r"); //print message to terminal
        }*/
        if (GPIOPinRead(GPIOA1_BASE, 0x20) && 0x20){ //check if SW[3] is pressed
            Message("\t\t SW[3] \n\r"); //print message to terminal
            break;
        }
    }
    fillScreen(0x0000); //fill the OLED with black

    if(y==2){
        davis=true;
    }
    else if(y==12){
          sacramento=true;
    }
    else if(y==22){
          tokyo=true;
    }
    else if(y==32){
        sanjose = true;
    }
    UART_PRINT("HTTP Post Begin:\n\r");
    int lRetVal = HTTPPostMethod(&httpClient,data_table);
    if(lRetVal < 0)
    {
    	UART_PRINT("HTTP Post failed.\n\r");
    }
    UART_PRINT("HTTP Post End:\n\r");

    UART_PRINT("\n\r");
    UART_PRINT("HTTP Put Begin:\n\r");
    lRetVal = HTTPPutMethod(&httpClient,data_table);
    if(lRetVal < 0)
    {
    	UART_PRINT("HTTP Put failed.\n\r");
    }
    UART_PRINT("HTTP Put End:\n\r");

    UART_PRINT("\n\r");
    UART_PRINT("HTTP Get Begin:\n\r");

    lRetVal = HTTPGetMethod(&httpClient,data_table);

    if(lRetVal < 0)
    {
    	UART_PRINT("HTTP Post Get failed.\n\r");
    }
    UART_PRINT("HTTP Get End:\n\r");
    UART_PRINT("\n\r");

    //Connect the CC3200 to the local access point
      lRetVal1 = connectToAccessPoint();
      //Set time so that encryption can be used
      lRetVal1 = set_time();
      if(lRetVal1 < 0) {
          UART_PRINT("Unable to set time in the device");
          LOOP_FOREVER();
       }
          //Connect to the website with TLS encryption
          lRetVal1 = tls_connect();
          if(lRetVal1 < 0) {
          ERR_PRINT(lRetVal);
      }
    // Stop the CC3200 device
    http_get(lRetVal,state);
    setCursor(10,80);
    Outstr ("City: ");
    Outstr (data_table[0]);
    setCursor(10,95);
    Outstr ("Temperature: ");
    Outstr (data_table[1]);
    Outstr (" F");
    setCursor(10,110);
    Outstr ("Status: ");
    Outstr (data_table[2]);
    int sunny = 0;
    int cloudy = 0;
    //int rain = 0;
    if(strcmp(data_table[2], "Clear") == 0){
        state = 0;
        sunny = 1;
    }else if(strcmp(data_table[2], "Clouds") == 0){
        cloudy = 1;
        state = 1;
    }else if(strcmp(data_table[2], "Rain") == 0){
        rain = 1;
        state = 1;
    }


    http_post(lRetVal,state);

    if (sunny==1){
        drawBitmap(5, 5, sun1,80, 80, 0xffe0);
        while(1){
            drawBitmap(5, 5, sun1inv,80, 80, 0xffe0);
            drawBitmap(5, 5, eyebrow1,80, 80, 0xffe0);
            delay(30);
            drawBitmap(5, 5, eyebrow1,80, 80, 0x0000);
            drawBitmap(5, 5, sun1inv,80, 80, 0x0000);
            drawBitmap(5, 5, sun2inv,80, 80, 0xffe0);
            drawBitmap(5, 5, eyebrow2,80, 80, 0xffe0);
            delay(30);
            drawBitmap(5, 5, eyebrow2,80, 80, 0x0000);
            drawBitmap(5, 5, sun2inv,80, 80, 0x0000);
            if(GPIOPinRead(GPIOA2_BASE,0x40) && 0x40){
                        break;
            }
        }
    }
    if (cloudy==1){
           drawBitmap(5, 5, ogcloud,80, 80, 0xffff);
           while(1){
               drawBitmap(5, 5, cloud1,80, 80, 0xffff);
               delay(200);
               drawBitmap(5, 5, cloud1,80, 80, 0x0000);
               drawBitmap(5, 5, cloud2,80, 80, 0xffff);
               delay(200);
               drawBitmap(5, 5, cloud2,80, 80, 0x0000);
               if(GPIOPinRead(GPIOA2_BASE,0x40) && 0x40){
                                       break;
                           }
           }
       }
    davis=false;
    sacramento=false;
    tokyo=false;
    sanjose = false;

    //http_post(lRetVal);
    sl_Stop(SL_STOP_TIMEOUT);
    //LOOP_FOREVER();


}
static int HandleInput(char *acRecvbuff)
{
    char *pcIndxPtr;
    char *pcEndPtr;
    //
    // Get city name
    //
    DBG_PRINT(acRecvbuff);
    pcIndxPtr = strstr(acRecvbuff, "status");
    DBG_PRINT("\n\r****************************** \n\r\n\r");
    DBG_PRINT("Input: ");
    if( NULL != pcIndxPtr ){
        pcIndxPtr = pcIndxPtr + strlen("status") + 3;
        pcEndPtr = strstr(pcIndxPtr, "reported") -4;
        if( NULL != pcEndPtr ){
           *pcEndPtr = 0;
        }
        DBG_PRINT("%s\n\r",pcIndxPtr);
    }
    fillScreen(0x0000); //fill the OLED with black

    if(pcIndxPtr != "ON" ){
        UART_PRINT("HERE");
        drawBitmap(5, 5, sun1,80, 80, 0xffe0);
                while(1){
                    drawBitmap(5, 5, sun1inv,80, 80, 0xffe0);
                    drawBitmap(5, 5, eyebrow1,80, 80, 0xffe0);
                    delay(30);
                    drawBitmap(5, 5, eyebrow1,80, 80, 0x0000);
                    drawBitmap(5, 5, sun1inv,80, 80, 0x0000);
                    drawBitmap(5, 5, sun2inv,80, 80, 0xffe0);
                    drawBitmap(5, 5, eyebrow2,80, 80, 0xffe0);
                    delay(30);
                    drawBitmap(5, 5, eyebrow2,80, 80, 0x0000);
                    drawBitmap(5, 5, sun2inv,80, 80, 0x0000);
                    if(GPIOPinRead(GPIOA1_BASE, 0x20) && 0x20){
                        drawBitmap(5, 5, sun1,80, 80, 0x0000);
                        break;
                    }

                }
          drawBitmap(5, 5, ogcloud,80, 80, 0xffff);
                while(1){
                       drawBitmap(5, 5, cloud1,80, 80, 0xffff);
                       delay(200);
                       drawBitmap(5, 5, cloud1,80, 80, 0x0000);
                       drawBitmap(5, 5, cloud2,80, 80, 0xffff);
                       delay(200);
                       drawBitmap(5, 5, cloud2,80, 80, 0x0000);
                       if(GPIOPinRead(GPIOA1_BASE, 0x20) && 0x20){
                           drawBitmap(5, 5, ogcloud,80, 80, 0x0000);
                           break;
                       }
                   }
                drawBitmap(5, 5, ograin,80, 80, 0xffff);
                  while(1){
                        drawBitmap(5, 5, rain1,80, 80, 0x001F);
                        delay(200);
                        drawBitmap(5, 5, rain1,80, 80, 0x0000);
                        drawBitmap(5, 5, rain2,80, 80, 0x001F);
                        delay(200);
                        drawBitmap(5, 5, rain2,80, 80, 0x0000);
                        if(GPIOPinRead(GPIOA1_BASE, 0x20) && 0x20){
                            drawBitmap(5, 5, ograin,80, 80, 0x0000);
                            break;
                   }
               }
            }
    fillScreen(0x0000); //fill the OLED with black

    return SUCCESS;
}
static int HandleXMLData(char *acRecvbuff,char** data_table)
{
    char *pcIndxPtr;
    char *pcEndPtr;

    //
    // Get city name
    //
    pcIndxPtr = strstr(acRecvbuff, "name");
    DBG_PRINT("\n\r****************************** \n\r\n\r");
    DBG_PRINT("City: ");
    if( NULL != pcIndxPtr )
    {
        pcIndxPtr = pcIndxPtr + strlen("name=") + 2;
        pcEndPtr = strstr(pcIndxPtr, "cod") -3;
        if( NULL != pcEndPtr )
        {
           *pcEndPtr = 0;
        }
        DBG_PRINT("%s\n\r",pcIndxPtr);
        data_table[0]= pcIndxPtr;
    }

    //
    // Get temperature value
    //
    pcIndxPtr = strstr(acRecvbuff, "temp");
    DBG_PRINT("Temperature: ");
    if( NULL != pcIndxPtr )
    {
        pcIndxPtr = pcIndxPtr + strlen("temp") + 2;
        pcEndPtr = strstr(pcIndxPtr, "feels_like")-2;
        if( NULL != pcEndPtr )
        {
           *pcEndPtr = 0;
        }
        data_table[1]=pcIndxPtr;
        DBG_PRINT("%s\n\r",pcIndxPtr);
    }
    else
    {
        DBG_PRINT("N/A\n\r");
        return 0;
    }


    //
    // Get weather condition
    //
    pcIndxPtr = strstr(acRecvbuff, "weather");
    DBG_PRINT("Weather State: ");
    if( NULL != pcIndxPtr )
    {
        pcIndxPtr = pcIndxPtr + strlen("weather") + 21;
        pcEndPtr = strstr(pcIndxPtr, "description")-3;
        if( NULL != pcEndPtr )
        {
           *pcEndPtr = 0;
        }
        DBG_PRINT("%s\n\r",pcIndxPtr);
        data_table[2]=pcIndxPtr;

    }
    else
    {
        DBG_PRINT("N/A\n\r");
        return 0;
    }
    return SUCCESS;
}
static int http_post(int iTLSSockID, int state){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    char* pcBufHeadersINPUT;

    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int dataLength = strlen(DATA0);
    int dataLength1 = strlen(DATA1);


    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    if (state == 0){
        sprintf(cCLLength, "%d", dataLength);
   }else if(state == 1){
        sprintf(cCLLength, "%d", dataLength1);
   }
    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);
    if (state == 0){
        strcpy(pcBufHeaders, DATA0);
        pcBufHeaders += strlen(DATA0);
    }else if(state == 1){
        strcpy(pcBufHeaders, DATA1);
        pcBufHeaders += strlen(DATA1);
    }


    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        return lRetVal;
    }

    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}
static int http_get(int iTLSSockID, int state){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    char* pcBufHeadersINPUT;

    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, GETHEADER);
    pcBufHeaders += strlen(GETHEADER);
    strcpy(pcBufHeaders, GETHEADER);
    pcBufHeaders += strlen(GETHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int dataLength = strlen(DATA0);
    int dataLength1 = strlen(DATA1);


    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    if (state == 0){
        sprintf(cCLLength, "%d", dataLength);
   }else if(state == 1){
        sprintf(cCLLength, "%d", dataLength1);
   }
    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);
    if (state == 0){
        strcpy(pcBufHeaders, DATA0);
        pcBufHeaders += strlen(DATA0);
    }else if(state == 1){
        strcpy(pcBufHeaders, DATA1);
        pcBufHeaders += strlen(DATA1);
    }


    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        return lRetVal;
    }

    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }
    HandleInput(acRecvbuff);

    return 0;
}
int main(){
    long lRetVal = -1;
       BoardInit();

       //
       // Configure the pinmux settings for the peripherals exercised
       //
       PinMuxConfig();

       //
       // Configuring UART
       //
       InitTerm();

       //
       // Display banner
       //
       DisplayBanner(APP_NAME);
       HTTPCli_Struct httpClient;

       lRetVal = ConnectToAP();
       if(lRetVal < 0){
             LOOP_FOREVER();
       }
       lRetVal = ConnectToHTTPServer(&httpClient);
       if(lRetVal < 0){
             LOOP_FOREVER();
       }
       UART_PRINT("\n\r");
       InitializeAppVariables();

       I2C_IF_Open(I2C_MASTER_MODE_FST);

                          //
                          // Display the banner followed by the usage description
                          //
        MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
        MAP_SPIReset(GSPI_BASE);
        MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                                             SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                                                   (SPI_SW_CTRL_CS |
                                                   SPI_4PIN_MODE |
                                                   SPI_TURBO_OFF |
                                                   SPI_CS_ACTIVELOW |
                                                   SPI_WL_8));
       MAP_SPIEnable(GSPI_BASE);
       Adafruit_Init();
       fillScreen(0x0000); //fill the OLED with black
       while(1){
           setCursor(35, 25);
           Outstr ("Welcome");
           setCursor(36, 33);
           Outstr ("to the");
           setCursor(20, 41);
           Outstr ("Weather API!");
           setCursor(22, 60);
           Outstr ("Press SW[3]");
           setCursor(50, 70);
           Outstr ("to");
           setCursor(35, 80);
           Outstr ("Continue!");
           if (GPIOPinRead(GPIOA1_BASE, 0x20) && 0x20){ //check if SW[3] is pressed
                break;
            }
       }
       program(httpClient);

    while(1) {

        if(GPIOPinRead(GPIOA2_BASE,0x40) && 0x40){

            HTTPCli_Struct httpClient;

            lRetVal = ConnectToAP();
            if(lRetVal < 0){
                  LOOP_FOREVER();
            }
            lRetVal = ConnectToHTTPServer(&httpClient);
            if(lRetVal < 0){
                  LOOP_FOREVER();
            }
            UART_PRINT("\n\r");
            InitializeAppVariables();

            I2C_IF_Open(I2C_MASTER_MODE_FST);

                               //
                               // Display the banner followed by the usage description
                               //
             MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
             MAP_SPIReset(GSPI_BASE);
             MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                                                  SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                                                        (SPI_SW_CTRL_CS |
                                                        SPI_4PIN_MODE |
                                                        SPI_TURBO_OFF |
                                                        SPI_CS_ACTIVELOW |
                                                        SPI_WL_8));
            MAP_SPIEnable(GSPI_BASE);
            Adafruit_Init();
            program(httpClient);
        }
     }

}
