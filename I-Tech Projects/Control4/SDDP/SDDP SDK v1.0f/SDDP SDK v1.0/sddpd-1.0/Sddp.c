/**************************************************************************//**

 @file     SDDP.c

 @brief    Implements SDDP 'Simple Device Discovery Protocol'. 
 
 @section  Requirements
            Requires SDDPNetwork module
 
 @section  Detail Detail
            This protocol provides methods for device discovery and identification, 
            security key exchange, automatic driver installation, energy profile 
            discovery, and versioning update capability discovery. Berkeley 
            socket API is used to ease porting to various platforms but only
            minimally in this module. See the SDDPNetwork module for network
            interface. This implementation has been tested on LWIP version 1.3.1 
            and gcc version 4.4.5.
            
            The API is briefly described as follows. See Sddp.h and function comments
            below for full details.
            
            SDDPInit() to start the service and open required sockets.
            SDDPTick() is called regularly within the application loop to service 
                incoming and outgoing traffic.
            SDDPIdentify() to send an "identify" packet that will populate the device
                within a project (usually physical or UI button push triggered event).
            SDDPIPChanged() should also be called if the IP of the device changes.
            SDDPLeave() can be used to send a leaving notification to the network.
            SDDPShutdown() will send a leaving notification and also close any open
                network sockets. 
                
                       
            SDDP notifications are sent in the following circumstances:
            1. When the user of the device has initiated the manufacturer-defined 
                identify process.
            2. When the device powers on.
            3. When the IP address of the device changes.
            
            SDDP responses are sent in response to the following requests:
            1. When the device receives a search request searching for all devices.
            2. When the device receives a search request searching for devices in
               the same namespace as this device.
            
            Note: Utilizes 1999 C allowed C++ style comments. If building 
            as -ansi or -std=c90 comments should be changed to C standard.

 @section  Platforms Platform(s)
            LWIP - Linux - other platforms using Berkeley socket API

******************************************************************************/
///////////////////////////////////////
// Includes
///////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>


#ifdef USE_LWIP // LWIP
#include "Lwiplib.h"
#include "DebugUtils.h"
#else // Linux and others
#include <netinet/in.h> // for sockaddr_in
#include <syslog.h>
#endif

#include "Sddp.h"
#include "SddpStatus.h"
#include "SddpNetwork.h"
#include "SddpPacket.h"

///////////////////////////////////////
// Defines
///////////////////////////////////////
#define SDDP_MULTI_ADDR         "239.255.255.250"
#define SDDP_NETIF              "lm0"
#define SDDP_VERSION            "1.0"

#ifdef DEBUG_PRINTF
#define syslog(x, ...) printf(__VA_ARGS__);
#endif


///////////////////////////////////////
// Typedefs
///////////////////////////////////////
/**************************************************************************//**
 @enum     SDDPNotificationSubtype
           Notification type
******************************************************************************/
typedef enum
{
    SDDP_NONE,
    SDDP_ALIVE,
    SDDP_OFFLINE,
    SDDP_IDENTIFY
} NotificationType;

/**************************************************************************//**
 @typedef  struct SDDPConfig
           Describes a device by providing an instance of the vendor specific
           SDDP data. This data is product specific and describes the device
           to the SDDP module.
******************************************************************************/
typedef struct
{
    char *product_name;  // Product name for SDDP search target - i.e. "c4:television"
    char *primary_proxy; // Control4 primary proxy type - i.e. "TV"
    char *proxies;       // All Control4 proxy types support - i.e. "TV,DVD"
    char *manufacturer;  // Manufacturer - i.e. "Control4"
    char *model;         // Model number - i.e. "C4-105HCTV2-EB"
    char *driver;        // Control4 driver c4i - i.e. "Control4TVGen.c4i"
    int  max_age;        // Number of seconds advertisement is valid (controls
                         // advertisement interval) - i.e. 1800
    bool local_only;     // If this device information should only be broadcast locally
} SDDPDeviceConfig;


/**************************************************************************//**
 @typedef  struct SDDPPrivateState
           Describes private SDDP information about a device and associated
           controller.
******************************************************************************/
typedef struct
{
    bool                search_response;    // True if this a search response
    char               *tran;               // Request transaction
    NotificationType    nt;                 // Determines notification type
    time_t              time_prev;          // Used for periodic announcements
    unsigned int        alive_interval;     // Used for periodic announcements
} SDDPPrivateState;

typedef struct _SDDPDevice
{
    struct _SDDPDevice *next;
    SDDPDeviceConfig    config;
    SDDPPrivateState    private_state;
} SDDPDevice;


/**************************************************************************//**
 @typedef  struct SDDPState
           This is a wrapper for SDDP datastructures provided to SDDP API calls.

           'devices' is a list of device specific configuration required prior to
           using SDDP.

           'network_info' provides access to the socket file descriptors used
           by the SDDP module.

           'private_state' is information required by the SDDP module itself,
           and should not be modified.
******************************************************************************/
typedef struct
{
    SDDPDevice      *devices;
    SDDPNetworkInfo  network_info;
    bool             sddp_enabled;        // True if sddp is running
    char             host[MAX_HOST_SIZE]; // The hostname of the device
} SDDPState;


/**************************************************************************//**
 @enum     SDDPMessageType
           Announce destination
******************************************************************************/
typedef enum
{
    SDDP_MULTICAST,
    SDDP_UNICAST
} SDDPMessageType;


///////////////////////////////////////
// Variables
///////////////////////////////////////


///////////////////////////////////////
// Private Functions
///////////////////////////////////////
static SDDPStatus SDDPNotify(SDDPState *sddp_state, SDDPDevice *sddp_dev, SDDPMessageType type, struct sockaddr_in *dest);
static SDDPStatus SDDPCreateNotify(SDDPState *sddp_state, SDDPDevice *sddp_dev, char *dst, int max_len, const SDDPInterface *iface);
static int IsStateValid(const SDDPState *sddp_state);
SDDPStatus SDDPIsConfigValid(const SDDPDeviceConfig *sddp_config);
static char *dupStr(const char *str);


///////////////////////////////////////
// Public Functions
///////////////////////////////////////
/***************************************************************************//**
 @fn        SDDPStatus SDDPInit(SDDPHandle *handle)

 @brief     Initialize the SDDP handle.

 @param     handle - ptr to new handle to SDDP, caller must free with SDDPShutdown

 @return    SDDP_STATUS_SUCCESS on success
*******************************************************************************/
SDDPStatus SDDPInit(SDDPHandle *handle)
{
	SDDPState *sddp_state = malloc(sizeof(SDDPState));

	memset(sddp_state, 0, sizeof(SDDPState));
	*handle = (SDDPHandle)sddp_state;

	return SDDP_STATUS_SUCCESS;
}

/***************************************************************************//**
 @fn        SDDPStatus SDDPSetHost(const SDDPHandle handle, const char * host)

 @brief     Sets the hostname for SDDP to advertise

 @param     handle - handle to SDDP
 @param     host - hostname to use, will be copied

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_INVALID_PARAM on invalid parameters
*******************************************************************************/
SDDPStatus SDDPSetHost(const SDDPHandle handle, const char * host)
{
	SDDPState *sddp_state = (SDDPState *)handle;

    // Check state is valid
    if (sddp_state == NULL)
    {
        // Invalid state pointer
        syslog(LOG_INFO, "SDDP initialization failed");
        return SDDP_STATUS_INVALID_PARAM;
    }

    if (strlen(host) >= MAX_HOST_SIZE)
    {
        syslog(LOG_INFO, "SDDPSetHost failed: host param too long");
        return SDDP_STATUS_INVALID_PARAM;
    }
    strcpy(sddp_state->host, host);
    return SDDP_STATUS_SUCCESS;
}

/***************************************************************************//**
 @fn        SDDPStatus SDDPSetDevice(const SDDPHandle handle, int index, const char *product_name,
                                     const char *primary_proxy, const char *proxies,
                                     const char *manufacturer, const char *model,
                                     const char *driver, int max_age)


 @brief     Sets up the device info for SDDP to advertise.

 @param     handle - handle to SDDP
            index - index of device (usually 0, but there can be multiple devices)
            product_name - Product name for SDDP search target - i.e. "c4:television"
            primary_proxy - Control4 primary proxy type - i.e. "TV"
            proxies - All Control4 proxy types support - i.e. "TV,DVD"
            manufacturer - Manufacturer - i.e. "Control4"
            model - Model number - i.e. "C4-105HCTV2-EB"
            driver - Control4 driver c4i - i.e. "Control4TVGen.c4i"
            local_only - Whether to broadcast this information only locally

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_INVALID_PARAM on failure
*******************************************************************************/
SDDPStatus SDDPSetDevice(const SDDPHandle handle, int index, const char *product_name,
						 const char *primary_proxy, const char *proxies,
						 const char *manufacturer, const char *model,
						 const char *driver, int max_age, bool local_only)
{
	SDDPState *sddp_state = (SDDPState *)handle;
	SDDPDevice *dev;
	int i = index;

    // Check state is valid
    if (sddp_state == NULL)
    {
        // Invalid state pointer
        syslog(LOG_INFO, "SDDP initialization failed");
        return SDDP_STATUS_INVALID_PARAM;
    }

	dev = sddp_state->devices;
    if (sddp_state->devices == NULL)
    {
    	if (index != 0)
    	{
        	// bad index
            syslog(LOG_INFO, "Unable to set device (index = %d)", index);
            return SDDP_STATUS_INVALID_PARAM;
    	}

    	// create a new first device
    	sddp_state->devices = malloc(sizeof(SDDPDevice));
    	memset(sddp_state->devices, 0, sizeof(SDDPDevice));
    	dev = sddp_state->devices;
    }
    else if (index > 0)
    {
    	// index to the device before the one we want to set
    	while (i > 1)
    	{
    		dev = dev->next;
    		i--;

    		if (dev == NULL)
    		{
                syslog(LOG_INFO, "Unable to set device (index = %d)", index);
                return SDDP_STATUS_INVALID_PARAM;
    		}
    	}

    	// if we need to create a new device, do so
    	if (dev->next == NULL)
    	{
        	dev->next = malloc(sizeof(SDDPDevice));
        	memset(dev->next, 0, sizeof(SDDPDevice));
    	}

    	// whether or not we created a device, dev pointed to the device before the one we wanted, so advance
    	dev = dev->next;
    }

    // fill in the SDDPDevice
    if (dev->config.product_name)
    	free(dev->config.product_name);
	dev->config.product_name = dupStr(product_name);
    if (dev->config.primary_proxy)
    	free(dev->config.primary_proxy);
	dev->config.primary_proxy = dupStr(primary_proxy);
    if (dev->config.proxies)
    	free(dev->config.proxies);
	dev->config.proxies = dupStr(proxies);
    if (dev->config.manufacturer)
    	free(dev->config.manufacturer);
	dev->config.manufacturer = dupStr(manufacturer);
    if (dev->config.model)
    	free(dev->config.model);
	dev->config.model = dupStr(model);
    if (dev->config.driver)
    	free(dev->config.driver);
	dev->config.driver = dupStr(driver);
	dev->config.max_age = max_age;
	dev->config.local_only = local_only;

    return 	SDDPIsConfigValid(&dev->config);
}

/***************************************************************************//**
 @fn        SDDPStatus SDDPStart(const SDDPHandle handle)

 @brief     Start up the SDDP module. Note the SDDPSetProductDescription()
            call may be used to alter the device descriptor advertised over 
            SDDP later.

 @param     handle - handle to SDDP

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_INVALID_PARAM on invalid parameters
            SDDP_STATUS_NETWORK_ERROR - could not initialize or send on network
            SDDP_STATUS_TIME_ERROR - time API not implemetned
*******************************************************************************/
SDDPStatus SDDPStart(const SDDPHandle handle)
{
	SDDPState *sddp_state = (SDDPState *)handle;
    SDDPDevice *dev;
    SDDPStatus status = SDDP_STATUS_SUCCESS;

    syslog(LOG_INFO, "SDDP initializing");
      
    // Check state is valid
    if (!IsStateValid(sddp_state))
    {
        // Invalid state pointer
        syslog(LOG_INFO, "SDDP initialization failed");
        return SDDP_STATUS_INVALID_PARAM;   
    }
    
    sddp_state->sddp_enabled = true; // Enable SDDP   
       
    // Init network interface
    memset(&sddp_state->network_info, 0, sizeof(SDDPNetworkInfo));
    if (SDDPNetworkInit(&sddp_state->network_info) != SDDP_STATUS_SUCCESS)
    {
        // Network interface failed to initialize
        syslog(LOG_INFO, "SDDP network initialization failed");
        return SDDP_STATUS_NETWORK_ERROR;
    } 

    // Init public SDDP data and send notify
    // Enforce a minimum announcement period 
    for (dev = sddp_state->devices; dev; dev = dev->next)
    {
        // Init private SDDP data 
        memset(&dev->private_state, 0, sizeof(SDDPPrivateState));    
        dev->private_state.nt = SDDP_NONE;
        dev->private_state.search_response = false;
    
        if(dev->config.max_age > DISABLE_PERIODIC_ADVERTS && dev->config.max_age < MIN_ADVERT_PERIOD)
        {
            syslog(LOG_INFO, "Setting SDDP max_age to minimum of %d seconds", MIN_ADVERT_PERIOD);
            dev->config.max_age = MIN_ADVERT_PERIOD;
        }
        
        dev->private_state.alive_interval = (dev->config.max_age * 2) / 3;
        
        // Send startup notify
        status = SDDPNotify(sddp_state, dev, SDDP_MULTICAST, NULL);
        if (status == SDDP_STATUS_SUCCESS)
    	    time(&dev->private_state.time_prev);
    }
       
    // Check if time API is implemented
    if(time(NULL) == -1)
    {
        status = SDDP_STATUS_TIME_ERROR;
    }
    
    return status;
}

/***************************************************************************//**
 @fn        SDDPStatus SDDPTick(const SDDPHandle handle)

 @brief     Handles processing of incoming and outgoing SDDP packets. This should 
            be called regularly by the application. This does not block. If a 
            select is used, it can gate the call to this tick based upon the 
            sddp_state->network_info sockets.

 @param     handle - handle to SDDP

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_INVALID_PARAM if product information is incorrect
            SDDP_STATUS_NETWORK_ERROR if network is not initialized

*******************************************************************************/
SDDPStatus SDDPTick(const SDDPHandle handle)
{
	SDDPState *sddp_state = (SDDPState *)handle;
    static char data[MAX_SDDP_FRAME_SIZE+1];
    int recv_len;
    struct sockaddr_in from;
    unsigned int from_len = sizeof(struct sockaddr_in);
    SDDPStatus status = SDDP_STATUS_SUCCESS;
    SddpPacket *received;
    SDDPDevice *dev;
    
    // Check state is valid
    if (!IsStateValid(sddp_state))
    {
        return SDDP_STATUS_INVALID_PARAM;   
    }
    
    if(!sddp_state->sddp_enabled)
    {
        return SDDP_STATUS_UNINITIALIZED;
    }
       
    // Check if an advertisement notify should go out
    for (dev = sddp_state->devices; dev; dev = dev->next)
    {
        if(dev->config.max_age != DISABLE_PERIODIC_ADVERTS && 
           difftime(time(NULL), dev->private_state.time_prev) > dev->private_state.alive_interval)
        {
            syslog(LOG_INFO, "Sending Periodic SDDP advertisement");
            time(&dev->private_state.time_prev);
            dev->private_state.search_response = false;    
            if(SDDPNotify(sddp_state, dev, SDDP_MULTICAST, NULL) != SDDP_STATUS_SUCCESS)
            {
                syslog(LOG_INFO, "SDDP send failed");
            }
        }
    }

    // Notify SDDP module of received data
    // Check for multicast receive
    recv_len = SDDPNetworkReceive(&sddp_state->network_info, data, sizeof(data), (struct sockaddr*)&from, &from_len);
    if (recv_len <= 0)
    {
        // If set for non-block, no receive has occured - return
        return SDDP_STATUS_SUCCESS;   
    }
    
    // Limit receive to buffer size
    if (recv_len >= MAX_SDDP_FRAME_SIZE)    
    {
        recv_len = MAX_SDDP_FRAME_SIZE;
    }
    data[recv_len] = 0x00;
    
    received = ParseSDDPPacket(data);
    if (received)
    {
    	if (received->packet_type == SddpPacketRequest)
    	{
    		if (!strcmp(received->h.request.version, SDDP_VERSION))
    		{
    			if (!strcmp(received->h.request.method, "SEARCH"))
    			{
    				for (dev = sddp_state->devices; dev; dev = dev->next)
    				{
	    				if (!strcmp(received->h.request.argument, "*") ||
	    				    !strcmp(received->h.request.argument, dev->config.product_name) ||
	    				    (!strncmp(received->h.request.argument, dev->config.product_name, strlen(received->h.request.argument)) && 
	    				     dev->config.product_name[strlen(received->h.request.argument)] == ':'))
	    				{
	    				    if (dev->config.local_only && !SDDPNetworkIsLocalAddress(from.sin_addr))
	    				        continue;
	    				    
	    					syslog(LOG_DEBUG, "SDDP Search target MATCH");
	    					dev->private_state.search_response = true; // Is a search response
	    					dev->private_state.tran = received->tran;
	    					status = SDDPNotify(sddp_state, dev, SDDP_UNICAST, &from);
	    					dev->private_state.tran = NULL;
	    					if (status != SDDP_STATUS_SUCCESS)
	    					{
	    						syslog(LOG_INFO, "Send failure status %d", status);
	    					}
	    				}
	    			}
    			}
    			else if (!strcmp(received->h.request.method, "NOTIFY"))
    			{
    				// Currently nothing to do here, in future may handle notifications
    			}
    		}
    		else
    		{
    			syslog(LOG_INFO, "Unsupported SDDP request packet version: %s", received->h.request.version);
    		}
    	}
    	else if (received->packet_type == SddpPacketResponse)
    	{
    		if (!strcmp(received->h.request.version, SDDP_VERSION))
    		{
    			// Currently nothing to do here, in future may handle responses
    		}
    		else
    		{
    			syslog(LOG_INFO, "Unsupported SDDP response packet version: %s", received->h.response.version);
    		}
    	}
    	
    	FreeSDDPPacket(received);
    }
    else
    {
        syslog(LOG_INFO, "Unrecognized SDDP packet: %s", data);
    }

    return status;   
}


/***************************************************************************//**
 @fn        SDDPStatus SDDPIdentify(const SDDPHandle handle)

 @brief     Sends an SDDP identify. This is very similar to an notify, but is 
            typically triggered by a user event, such as a UI or physical button
            push. This sends a packet with the X-SDDPIDENTIFY: TRUE to notify a
            controller of the identify event on the device.

 @param     handle - handle to SDDP

 @return    SDDP_STATUS_SUCCESS if successful
            SDDP_STATUS_NETWORK_ERROR if network is not initialized or had an error
            
*******************************************************************************/
SDDPStatus SDDPIdentify(const SDDPHandle handle)
{
	SDDPState *sddp_state = (SDDPState *)handle;
    SDDPStatus status, notifyStatus;
    SDDPDevice *dev;
    SDDPInterface *ifaces, *ifa;
    char msg_buf[MAX_SDDP_FRAME_SIZE + 1]; // Response buffer

    // Check state is valid
    if (!IsStateValid(sddp_state))
    {
        return SDDP_STATUS_INVALID_PARAM;   
    }
    
    if(!sddp_state->sddp_enabled)
    {
        return SDDP_STATUS_UNINITIALIZED;
    }
    
    status = SDDP_STATUS_NETWORK_ERROR;
    for (dev = sddp_state->devices; dev; dev = dev->next)
    {
        ifaces = SDDPGetInterfaces(dev->config.local_only);
        for (ifa = ifaces; ifa != NULL; ifa = ifa->next)
        {
            // Init private data
            dev->private_state.nt = SDDP_IDENTIFY;
            dev->private_state.search_response = false; 
            memset(msg_buf, 0, sizeof(msg_buf));

            // Create sddp packet
            notifyStatus = SDDPCreateNotify(sddp_state, dev, msg_buf, MAX_SDDP_FRAME_SIZE, ifa);
            if (notifyStatus != SDDP_STATUS_SUCCESS)
                continue;

            notifyStatus = SDDPNetworkSendMulticast(&sddp_state->network_info, ifa, msg_buf); // Send
            if (notifyStatus != SDDP_STATUS_SUCCESS)
                continue;
            
            status = SDDP_STATUS_SUCCESS;
        }
    }
    
    free(ifaces);
    return status;
}


/***************************************************************************//**
 @fn        SDDPStatus SDDPLeave(const SDDPHandle handle)

 @brief     Sends byebye but leaves SDDP initialized (i.e. keeps the socket(s) 
            open and continues to send automatic advertisements). This is intended
            to be used prior to shutting down or disconnecting from a project,
            but does not mean the device is no longer present on the network.
            The last is to send a 'byebye' leave notification prior to leaving
            a network. 

 @param     handle - handle to SDDP

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_NETWORK_ERROR on nework errors
*******************************************************************************/
SDDPStatus SDDPLeave(const SDDPHandle handle)
{
	SDDPState *sddp_state = (SDDPState *)handle;
    SDDPStatus status, notifyStatus;
    SDDPDevice *dev;
    SDDPInterface *iface, *ifa;
    char msg_buf[MAX_SDDP_FRAME_SIZE + 1]; // Response buffer

    // Check state is valid   
    if (!IsStateValid(sddp_state))
    {
        return SDDP_STATUS_INVALID_PARAM;   
    }
    
    if(!sddp_state->sddp_enabled)
    {
        return SDDP_STATUS_UNINITIALIZED;
    }

    status = SDDP_STATUS_NETWORK_ERROR;
    for (dev = sddp_state->devices; dev; dev = dev->next)
    {
        iface = SDDPGetInterfaces(dev->config.local_only);
        for (ifa = iface; ifa != NULL; ifa = ifa->next)
        {
            // Init private data
            dev->private_state.nt = SDDP_OFFLINE;
            dev->private_state.search_response = false;
            memset(msg_buf, 0, sizeof(msg_buf));

            // Create sddp packet
            notifyStatus = SDDPCreateNotify(sddp_state, dev, msg_buf, MAX_SDDP_FRAME_SIZE, ifa);
            if (notifyStatus != SDDP_STATUS_SUCCESS)
                continue;

            notifyStatus = SDDPNetworkSendMulticast(&sddp_state->network_info, ifa, msg_buf);
            if (notifyStatus != SDDP_STATUS_SUCCESS)
                continue;
            
            status = SDDP_STATUS_SUCCESS;
        }
    }
    
    free(iface);
    return status;
}


/***************************************************************************//**
 @fn        SDDPStatus SDDPShutdown(SDDPHandle handle)

 @brief     Shutdowns the SDDP service, sends byebye, and closes sockets halting
            SDDP automatic advertisments. After calling this, the application 
            must call SDDPInit to reinitialize and the handle is invalid.

 @param     handle - handle to SDDP

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_INVALID_PARAM on invalid parameters
*******************************************************************************/
SDDPStatus SDDPShutdown(SDDPHandle * handle)
{
    // Check state is valid
    if (handle == NULL || *handle == NULL)
    {
        return SDDP_STATUS_INVALID_PARAM;
    }

	SDDPState *sddp_state = *((SDDPState **)handle);
	SDDPDevice *dev, *tempDev;

    syslog(LOG_DEBUG, "SDDP shutdown");

    // Send leave notify
    SDDPLeave(sddp_state);
    
    // Disable SDDP
    sddp_state->sddp_enabled = false; 

    // Close network interface
    SDDPNetworkClose(&sddp_state->network_info);
    
    dev = sddp_state->devices;
    while (dev != NULL)
    {
    	free(dev->config.product_name);
    	free(dev->config.primary_proxy);
    	free(dev->config.proxies);
    	free(dev->config.manufacturer);
    	free(dev->config.model);
    	free(dev->config.driver);
    	tempDev = dev;
    	dev = dev->next;
    	free(tempDev);
    }

    free(sddp_state);
    *handle = NULL;

    return SDDP_STATUS_SUCCESS;
}


///////////////////////////////////////
// Private Functions
///////////////////////////////////////
/***************************************************************************//**
 @fn        static SDDPStatus SDDPNotify(SDDPState *sddp_state, SDDPDevice *sddp_dev, 
                                                    SDDPMessageType type, struct sockaddr_in *dest)

 @brief     Sends an SDDP notify advertisment. This is similar to identify with 
            a couple of minor differences. This is typically used for periodic
            keep alives, but is also used to generate a search response during
            search requests.

 @param     sddp_state - pointer to SDDPState
 @param     sddp_dev - pointer to SDDPDevice
 @param     type - SDDP_MULTICAST or SDDP_UNICAST
 @param     dest - if NULL use private_description.controller
                   if non-NULL use dest as unicast destination
 @param     search_response - Set true if this is a search response

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_NETWORK_ERROR on nework errors

*******************************************************************************/
static SDDPStatus SDDPNotify(SDDPState *sddp_state, SDDPDevice *sddp_dev, SDDPMessageType type, struct sockaddr_in *dest)
{
    SDDPStatus status;
    SDDPInterface *iface, *ifa;
    char msg_buf[MAX_SDDP_FRAME_SIZE + 1]; // Response buffer

    // Check parameters
    if (type == SDDP_UNICAST && dest == NULL)
    {
        syslog(LOG_INFO, "Invalid unicast destination for announce");
        return SDDP_STATUS_INVALID_PARAM;
    }

    status = SDDP_STATUS_NETWORK_ERROR;
    iface = SDDPGetInterfaces(sddp_dev->config.local_only);
    for (ifa = iface; ifa != NULL; ifa = ifa->next)
    {
        // Init private data
        sddp_dev->private_state.nt = SDDP_ALIVE;
        memset(msg_buf, 0, sizeof(msg_buf));

        // Create sddp packet
        status = SDDPCreateNotify(sddp_state, sddp_dev, msg_buf, MAX_SDDP_FRAME_SIZE, ifa);
        if (status != SDDP_STATUS_SUCCESS)
            break;

        if (type == SDDP_MULTICAST)
        {
            // Send multicast    
            status = SDDPNetworkSendMulticast(&sddp_state->network_info, ifa, msg_buf); 
        }
        else
        {
            // Send unicast
            // TODO: Here we are only sending a packet with the From header using the IP
            //       from the first interface...
            status = SDDPNetworkSendTo(&sddp_state->network_info, msg_buf, dest);
            break;
        }
    }
    
    free(iface);
    return status;
}


/***************************************************************************//**
 @fn        static SDDPStatus SDDPCreateNotify(SDDPState *sddp_state,
                                                        char *dst, int max_len)

 @brief     Create a notification. This is used for a couple of purpose in various
            forms.

 @param     sddp_state - pointer to SDDPState
 @param     sddp_dev - pointer to SDDPDevice
 @param     dst - pointer to buffer for notify
 @param     max_len - buffer length
 @param     product_desc - manufacturer adjustable SDDP notify data
 @param     private_desc - private SDDP notify data

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_FATAL_ERROR on failure
*******************************************************************************/
static SDDPStatus SDDPCreateNotify(SDDPState *sddp_state, SDDPDevice *sddp_dev, char *dst, int max_len, const SDDPInterface *iface)
{
	static char version[] = "1.0";
	static char status_code[] = "200";
	static char reason[] = "OK";
	static char notify[] = "NOTIFY";
	static char alive[] = "ALIVE";
	static char offline[] = "OFFLINE";
	static char identify[] = "IDENTIFY";
	char max_age[32];
	
	snprintf(max_age, sizeof(max_age), "%d", sddp_dev->config.max_age);
	
	SddpPacket packet;
	int devInfo = 0;
	char from[sizeof(iface->ip_addr) + 12];
	
	if (sddp_dev->private_state.search_response)
	{
		BuildSDDPResponse(&packet, version, status_code, reason);
		packet.tran = sddp_dev->private_state.tran;
		packet.max_age = max_age;
		devInfo = 1;
	}
	else
	{
		if (sddp_dev->private_state.nt == SDDP_ALIVE)
		{
			BuildSDDPRequest(&packet, version, notify, alive);
			packet.max_age = max_age;
			devInfo = 1;
		}
		else if (sddp_dev->private_state.nt == SDDP_OFFLINE)
		{
			BuildSDDPRequest(&packet, version, notify, offline);
			packet.type = sddp_dev->config.product_name;
		}
		else if (sddp_dev->private_state.nt == SDDP_IDENTIFY)
		{
			BuildSDDPRequest(&packet, version, notify, identify);
			devInfo = 1;
		}
		else
			return SDDP_STATUS_FATAL_ERROR;
	}
	
	packet.host = sddp_state->host;
	
	snprintf(from, sizeof(from), "%s:%d", iface->ip_addr, SDDP_PORT);
	packet.from = from;
	
	if (devInfo)
	{
		packet.type = sddp_dev->config.product_name;
		packet.primary_proxy = sddp_dev->config.primary_proxy;
		packet.proxies = sddp_dev->config.proxies;
		packet.manufacturer = sddp_dev->config.manufacturer;
		packet.model = sddp_dev->config.model;
		packet.driver = sddp_dev->config.driver;
	}
	
	if (WriteSDDPPacket(&packet, dst, max_len) > max_len)
	{
		syslog(LOG_INFO, "Create notify could not build the packet");
		dst = NULL;
		return SDDP_STATUS_FATAL_ERROR;
	}
	
	return SDDP_STATUS_SUCCESS;
}


/***************************************************************************//**
 @fn        static int IsStateValid(const SDDPState *sddp_state)

 @brief     Checks the SDDP state is valid. Currently this only checks if the 
            device config pointers are non-null. This can be expanded to mroe
            rigorously scrub the data in teh future.

 @param     sddp_state - pointer to SDDPState

 @return    1 if state is valid, 0 otherwise

*******************************************************************************/
static int IsStateValid(const SDDPState *sddp_state)
{
    SDDPDevice *dev;
    
    // Check for NULL state
    if(sddp_state == NULL || sddp_state->devices == NULL)
        return 0;
    if(sddp_state->host[0] == '\0')
        return 0;
    
    for (dev = sddp_state->devices; dev; dev = dev->next)
    {
        if (SDDPIsConfigValid(&dev->config) != SDDP_STATUS_SUCCESS)
    	    return 0;
    }

    
    // Everything passed
    return 1;              
}

SDDPStatus SDDPIsConfigValid(const SDDPDeviceConfig *sddp_config)
{
    // Check for NULL pointers
    if(sddp_config->product_name == NULL ||
       sddp_config->primary_proxy == NULL ||
       sddp_config->proxies == NULL ||       
       sddp_config->manufacturer == NULL ||
       sddp_config->model == NULL ||
       sddp_config->driver == NULL)
    {
        return SDDP_STATUS_INVALID_PARAM;
    }
    
    return SDDP_STATUS_SUCCESS;
}

static char *dupStr(const char *str)
{
	char *buf;
	size_t len;

	len = str ? strlen(str) : 0;
	buf = malloc(len + 1);
	if (!buf)
		return NULL;
	if (str)
		memcpy(buf, str, len + 1);
	else
		buf[0] = '\0';
	return buf;
}
