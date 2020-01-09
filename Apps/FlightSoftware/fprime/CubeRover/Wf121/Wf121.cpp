#include "Wf121.hpp"

using namespace Wf121;

Wf121Driver :: Wf121Driver(){
    m_processingCmd = false;
}


ErrorCode Wf121Driver :: Init(){
#if __USE_CTS_RTS__
  gioSetBit(gioPORTB,3,1);       // Pull high RTS : not ready to receive some data
#endif //#if __USE_CTS_RTS__


  return NO_ERROR;
}

/**
 * @brief      Transmit data to WF121 module
 *
 * @param      header   The header
 * @param      payload  The payload
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: transmitCommand(BgApiHeader *header,
                                         uint8_t *payload){

  // check if a command is already being sent out, if so, only one command can
  // be sent at a time.
  if(m_processingCmd){
    return TOO_MANY_REQUEST;
  } 
   
#if __USE_CTS_RTS__
  uint32_t timeout = 10000;
  while(gioGetBit(gioPORTB, 2) && --timeout); //block until CTS goes low
  if(!timeout) return TIMEOUT;
#endif //#if __USE_CTS_RTS__

  while(!sciIsTxReady(SCI_REG));
  sciSend(SCI_REG, sizeof(header), (uint8_t *)header);

  uint16_t payloadSize = getPayloadSizeFromHeader(header);

  if(payloadSize > 0){
    if(payload == NULL){
      return INVALID_PARAMETER;
    }

    while(!sciIsTxReady(SCI_REG));
    sciSend(SCI_REG, payloadSize, payload);
  }

  // Flag that a command is processing, cannot send a new command until the
  // current one is processed
  m_processingCmd = true;

  return NO_ERROR;
}


/**
 * @brief      Execute system callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executeSystemCallback(BgApiHeader *header,
                                               uint8_t *payload,
                                               const uint16_t payloadSize){
  uint16_t result;
  uint16_t major;
  uint16_t minor;
  uint16_t patch;
  uint16_t build;
  uint16_t bootloaderVersion;
  uint16_t tcpIpVersion;
  uint16_t hwVersion;
  uint32_t address;
  uint8_t type;
  PowerSavingState state;

  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // Sync 
        return cb_CommandSyncSystem();
      case 0x01: // Reset
        break;
      case 0x02: // Hello
        return cb_CommandHelloSystem();
      case 0x03: // Set Max Power Saving State
        memcpy(&result,
              payload,
              sizeof(result));
        return cb_CommandSetMaxPowerSavingState(result);
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // Boot
        memcpy(&major,
               payload,
               sizeof(major));
        memcpy(&minor,
               payload + 2, /* sizeof minor */
               sizeof(minor));
        mempcy(&patch,
               payload + 4, /* sizeof minor, major */
               sizeof(patch));
        memcpy(&build,
               payload + 6, /* sizeof minor major, patch */
               sizeof(build));
        memcpy(&bootloaderVersion,
               payload + 8, /* sizeof minor, major, patch, build */
               sizeof(bootloaderVersion);
        memcpy(&tcpIpVersion,
               payload + 10, /* sizeof minor, major, patch, build, bootloader version */
              sizeof(tcpIpVersion));
        memcpy(&hwVersion,
               payload + 12,  /* sizeof minor, major, patch, build, bootloader version */
               sizeof(hwVersion));
        return cb_EventBoot(major,
                            minor,
                            patch,
                            build,
                            bootloaderVersion,
                            tcpIpVersion,
                            hwVersion);
      case 0x02: // Software Exception
        if(payloadSize < sizeof(address) + sizeof(type)){
          return UNSPECIFIED_ERROR; // should not happen at that point
        }
        mempcy(&address,
               payload,
               sizeof(address));
        memcpy(&type,
               payload + sizeof(address),
               sizeof(type));
        return cb_EventSoftwareException(address, type);
      case 0x03: // Power saving state
        state = payload[0];
        return cb_EventPowerSavingState(state);
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  return NO_ERROR;
}


/**
 * @brief      Execute configuration callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executeConfigurationCallback(BgApiHeader *header,
                                                      uint8_t *payload,
                                                      const uint16_t payloadSize){
  uint16_t result;
  HardwareInterface interface;
  HardwareAddress hwAddr;

  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x00:  // Get MAC address
        memcpy(&result,
               payload,
               sizeof(result));
        interface = payload[sizeof(result)];
        return cb_CommandGetMacAddress(result, interface);
      case 0x01:  // Set MAC address
        memcpy(&result,
               payload,
               sizeof(result));
        interface = payload[sizeof(result)];
        return cb_CommandSetMacAddress(result, interface);
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // MAC address
        interface = payload[0];
        memcpy(&hwAddr,
              payload + 1 /* hardware interface */,
              sizeof(HardwareAddress));
        return cb_EventMacAddress(interface, hwAddr);
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }
  return NO_ERROR;    
}


/**
 * @brief      Execute wi-fi callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executeWifiCallback(BgApiHeader *header,
                                             uint8_t *payload,
                                             const uint16_t payloadSize){
  uint16_t result;
  uint16_t reason;
  uint8_t status;
  HardwareAddress address;
  HardwareInterface interface;
  int8_t channel;
  int16_t rssi;
  int8_t snr;
  uint8_t secure;
  Ssid ssid;
  SsidSize ssidSize;

  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // Wifi ON
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandTurnOnWifi(result);
      case 0x01: // Wifi OFF
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandTurnOffWifi(result);
      case 0x09: // Set scan channels
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandSetScanChannels(result);       
      case 0x03: // Start scan channels
        memcpy(&result,
               payload,
               sizeof(result));     
        return cb_CommandStartScanChannels(result);
      case 0x04: // Stop scan channels
        memcpy(&result,
               payload,
               sizeof(result));     
        return cb_CommandStopScanChannels(result);
      case 0x06: // Connect BSSID
        memcpy(&result,
               payload,
               sizeof(result)); 
        interface = payload[2];
        memcpy(&address,
               payload + 3, /* offset by interface + result */
               sizeof(HardwareAddress));
        return cb_CommandConnectBssid(result,
                                      interface,
                                      address);
      case 0x08: // Disconnect
        memcpy(&result,
               payload,
               sizeof(result));
        interface = payload[2]; 
        return cb_CommandDisconnect(result,
                                    interface);
      case 0x0D: // Scan results
        memcpy(&result,
               payload,
               sizeof(result));       
        return cb_CommandScanResultsSortRssi(result);
      case 0x05: // Set password
        status = payload[0];
        return cb_CommandSetPassword(status);
      case 0x07: // Connect SSID
        memcpy(&result,
               payload,
               sizeof(result)); 
        interface = payload[2];
        memcpy(&address,
               payload + 3, /* offset by interface + result */
               sizeof(HardwareAddress));
        return cb_CommandConnectSsid(result,
                                     interface,
                                     address);
      case 0x13: // Get signal quality
        memcpy(&result,
               payload,
               sizeof(result)); 
        interface = payload[2];      
        return cb_CommandGetSignalQuality(result,
                                          interface);
      case 0x14: // Start sssid scan
        memcpy(&result,
               payload,
               sizeof(result)); 
        return cb_CommandStartSsidScan(result);
      case 0x15: // set AP hidden
        memcpy(&result,
               payload,
               sizeof(result)); 
        interface = payload[2];        
        return cb_CommandSetApHidden(result,
                                     interface);
      case 0x16: // Set 11n mode
        memcpy(&result,
               payload,
               sizeof(result)); 
        interface = payload[2];
        return cb_CommandSet11nMode(result,
                                    interface);
      case 0x17: // Set Ap client isolation
        memcpy(&result,
               payload,
               sizeof(result)); 
        interface = payload[2];     
        return cb_CommandSetApClientIsolation(result,
                                              interface);
      case 0x11: // Start WPS
        memcpy(&result,
               payload,
               sizeof(result)); 
        interface = payload[2];
        return cb_CommandStartWps(result,
                                  interface);
      case 0x12: // Stop WPS
        memcpy(&result,
               payload,
               sizeof(result)); 
        interface = payload[2];
        return cb_CommandStopWps(result,
                                 interface);
      case 0x0A: // Set operating mode
        memcpy(&result,
               payload,
               sizeof(result)); 
        return cb_CommandSetOperatingMode(result);
      case 0x10: // Set AP max clients
        memcpy(&result,
               payload,
               sizeof(result)); 
        interface = payload[2];
        return cb_CommandSetApMaxClients(result,
                                        interface);
      case 0x0F: // Set AP password
        status = payload[0];
        return cb_CommandSetApPassword(status);
      case 0x0B: // Start AP mode
        memcpy(&result,
               payload,
               sizeof(result));
        interface = payload[2];         
        return cb_CommandStartApMode(result,
                                     interface);
      case 0x0C: // Stop AP mode
        memcpy(&result,
               payload,
               sizeof(result));
        interface = payload[2];         
        return cb_CommandStopApMode(result,
                                    interface);
      case 0x0E: // AP disconnect client
        memcpy(&result,
               payload,
               sizeof(result));
        interface = payload[2];         
        return cb_CommandDisconnectApClient(result,
                                            interface);
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // Wifi ON
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_EventWifiIsOn(result);
      case 0x01: // Wifi OFF
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_EventWifiIsOff(result);
      case 0x02: // Scan result
        memcpy(&address,
               payload,
               sizeof(address));
        memcpy(&channel,
               payload + sizeof(address),
               sizeof(channel));
        memcpy(&rssi,
               payload + sizeof(address) + sizeof(channel),
               sizeof(rssi));
        memcpy(snr,
               payload + sizeof(address) + sizeof(channel) + sizeof(rssi),
               sizeof(snr));
        memcpy(&secure,
               payload + sizeof(address) + sizeof(channel) + sizeof(rssi) + sizeof(snr),
               sizeof(secure));
        memcpy(&ssidSize,
               payload + sizeof(address) + sizeof(channel) + sizeof(rssi) + sizeof(snr) + sizeof(secure),
               sizeof(ssidSize));
        mempcy(&ssid,
               payload + sizeof(address) + sizeof(channel) + sizeof(rssi) + sizeof(snr) + sizeof(secure) + sizeof(ssidSize),
               ssidSize);
        return cb_EventScanResult(address,
                                  channel,
                                  rssi,
                                  snr,
                                  secure,
                                  ssid,
                                  ssidSize);
      case 0x03: // Scan result drop
        memcpy(&address,
               payload,
               sizeof(HardwareAddress));
        return cb_EventScanResultDrop(address);
      case 0x04: // Scanned
        return cb_EventScanned(payload[0]);
      case 0x0F: // Scan sort result
        memcpy(&address,
               payload,
               sizeof(address));
        memcpy(&channel,
               payload + sizeof(address),
               sizeof(channel));
        memcpy(&rssi,
               payload + sizeof(address) + sizeof(channel),
               sizeof(rssi));
        memcpy(snr,
               payload + sizeof(address) + sizeof(channel) + sizeof(rssi),
               sizeof(snr));
        memcpy(&secure,
               payload + sizeof(address) + sizeof(channel) + sizeof(rssi) + sizeof(snr),
               sizeof(secure));
        memcpy(&ssidSize,
               payload + sizeof(address) + sizeof(channel) + sizeof(rssi) + sizeof(snr) + sizeof(secure),
               sizeof(ssidSize));
        mempcy(ssid,
               payload + sizeof(address) + sizeof(channel) + sizeof(rssi) + sizeof(snr) + sizeof(secure) + sizeof(ssidSize),
               ssidSize);
        return cb_EventScanSortResult(address,
                                      channel,
                                      rssi,
                                      snr,
                                      secure,
                                      ssid,
                                      ssidSize);
      case 0x10: // Scan sort finished
        return cb_EventScanSortFinished();
      case 0x05: // Connected
        status = payload[0];
        interface = payload[1];
        bssidSize = payload[2];
        mempcy(bssid,
               payload + 3,
               bssidSize);
        return cb_EventConnected(status,
                                 interface,
                                 bssid,
                                 bssidSize);
      case 0x09: // Connect retry
        interface = payload[sizeof(reason)];        
        return cb_EventConnectRetry(interface);     
      case 0x08: // Connect failed
        memcpy(reason,
               payload,
               sizeof(reason));
        interface = payload[sizeof(reason)];        
        return cb_EventConnectFailed(reason,
                                     interface);
      case 0x06: // Disconnected
        memcpy(reason,
               payload,
               sizeof(reason));
        interface = payload[sizeof(reason)];
        return cb_EventDisconnected(reason,
                                    interface);
      case 0x14: // WPS credential SSID
        interface = payload[0];
        mempcy(&ssidSize,
               payload + 1 ,
               sizeof(ssidSize));
        return cb_EventCredentialSsid(interface,
                                      payload + 2 /* offset by interface + ssidSize */,
                                      ssidSize);
      case 0x15: // WPS credential password
        return cb_EventWpsCredentialPassword(interface,
                                             payload + 1,
                                             payload[0]);
      case 0x12: // WPS completed
        interface = payload[0];
        return cb_EventWpsCompleted(interface);
      case 0x13: // WPS failed
        memcpy(reason,
               payload,
               sizeof(reason));
        interface = payload[sizeof(reason)]; 
        return cb_EventWpsFailed(reason,
                                 interface);
      case 0x11: // WPS stopped
        interface = payload[0];
        return cb_EventWpsStopped(interface);
      case 0x16: // signal quality
        rssi = payload[0];
        interface = payload[1];
        return cb_EventSignalQuality(rssi,
                                     interface);
      case 0x0A: // AP mode started
        interface = payload[0];
        return cb_EventApModeStarted(interface);
      case 0x0B: // AP mode stopped
        interface = payload[0];
        return cb_EventApModeStopped(interface);
      case 0x0C: // AP mode failed
        memcpy(reason,
               payload,
               sizeof(reason));
        interface = payload[sizeof(reason)];
        return cb_EventApModeFailed(reason,
                                    interface);
      case 0x0D: // AP client joined
        memcpy(&address,
               payload,
               sizeof(HardwareAddress));
        interface = payload[sizeof(HardwareAddress)];
        return cb_EventApClientJoined(address,
                                      interface);
      case 0x0E: // AP client left
        memcpy(&address,
               payload,
               sizeof(HardwareAddress));
        interface = payload[sizeof(HardwareAddress)];
        return cb_EventApClientLeft(address,
                                      interface);
      case 0x07: // interface status
        interface = payload[0];
        status = payload[1];
        return cb_EventInterfaceStatus(interface,
                                       status);
      default:
        return COMMAND_NOT_RECOGNIZED;
    }    
  }

  return NO_ERROR;
}


/**
 * @brief      Execute endpoint callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executeEndpointCallback(BgApiHeader *header,
                                                 uint8_t *payload,
                                                 const uint16_t payloadSize){
  uint16_t result;
  Endpoint endpoint;
  uint8_t * data;
  DataSize dataSize;
  uint32_t endpointType;
  uint8_t streaming;
  int8_t destination;
  uint8_t active;

  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x02: // set active
        memcpy(&result,
               payload,
               sizeof(result)); 
        endpoint = payload[2];
        return cb_CommandSetActiveEndpoint(result,
                                           endpoint); 
      case 0x00: // send
        memcpy(&result,
               payload,
               sizeof(result)); 
        endpoint = payload[2];
        return cb_CommandSendEndpoint(result,
                                      endpoint);  
      case 0x05: // set transmit size
        memcpy(&result,
               payload,
               sizeof(result)); 
        endpoint = payload[2];
        return cb_CommandSetTransmitSize(result,
                                         endpoint); 
      case 0x01: // set streaming
        memcpy(&result,
               payload,
               sizeof(result)); 
        endpoint = payload[2];
        return cb_CommandSetStreaming(result,
                                      endpoint); 
      case 0x03: // set streaming destination
        memcpy(&result,
               payload,
               sizeof(result)); 
        endpoint = payload[2];
        return cb_CommandSetStreamingDestination(result,
                                                 endpoint); 
      case 0x04: // close endpoint
        memcpy(&result,
               payload,
               sizeof(result)); 
        endpoint = payload[2];
        return cb_CommandCloseEndpoint(result,
                                       endpoint);
      case 0x05: // disable endpoint
        memcpy(&result,
               payload,
               sizeof(result)); 
        endpoint = payload[2];
        return cb_CommandDisableEndpoint(result,
                                         endpoint);         
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      case 0x02: // status
      endpoint = payload[0];
      memcpy(&endpointType,
              payload + sizeof(endpoint),
              sizeof(endpointType));
      streaming = payload[sizeof(endpoint) + sizeof(endpointType) + sizeof(endpointType)];
      destination = payload[sizeof(endpoint) + sizeof(endpointType) + sizeof(endpointType) + sizeof(streaming)];
      active = payload[sizeof(endpointType) + sizeof(endpointType) + sizeof(streaming) + sizeof(destination)];
      return cb_EventEndpointStatus(endpoint,
                                    endpointType,
                                    streaming,
                                    destination,
                                    active);
      case 0x01: // data
        endpoint = payload[0];
        dataSize = payload[1];
        data = payload + sizeof(dataSize) + sizeof(endpoint);
        return cb_EventDataEndpoint(endpoint,
                                    data,
                                    dataSize); 
      case 0x03: // closing
        memcpy(&result,
               payload,
               sizeof(result)); 
        endpoint = payload[2];
        return cb_EventClosingEndpoint(result,
                                       endpoint);   
      case 0x04: // error
        memcpy(&result,
               payload,
               sizeof(result)); 
        endpoint = payload[2];
        return cb_EventErrorEndpoint(result,
                                     endpoint);  
      case 0x00: // syntax error
          memcpy(&result,
                 payload,
                 sizeof(result));
          endpoint = payload[sizeof(result)];
          cb_EventEndpointSyntaxError(result, endpoint);
        break;
      default:
        return COMMAND_NOT_RECOGNIZED;
    }    
  }

  return NO_ERROR;
}


/**
 * @brief      Execute hardware callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executeHardwareCallback(BgApiHeader *header,
                                                 uint8_t *payload,
                                                 const uint16_t payloadSize){
  uint16_t result;
  uint8_t input;
  uint16_t value;
  uint8_t handle;

  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x09: // ADC read
        memcpy(&result,
               payload,
               sizeof(result));     
        input = payload[result];
        memcpy(&value,
               payload + sizeof(result) + sizeof(input),
               sizeof(value));
        return cb_CommandAdcRead(result,
                                 input,
                                 value); 
      case 0x02: // Change notification
        break;
      case 0x03: // Change notification pullup
        memcpy(&result,
               payload,
               sizeof(result));     
        return cb_CommandConfigureChangeNotification(result);
      case 0x01: // External interrupt
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandConfigureExternalInterrupt(result);
      case 0x04: // IO port config direction
        memcpy(&result,
               payload,
               sizeof(result));     
        return cb_CommandConfigureIoPortDirection(result);
      case 0x05: // IO port config drain
        memcpy(&result,
               payload,
               sizeof(result));          
        return cb_CommandConfigureIoOpenDrain(result);
      case 0x07: // IO port read
        memcpy(&result,
               payload,
               sizeof(result)); 
        return cb_CommandReadIoPort(result);
      case 0x06: // IO port write
        memcpy(&result,
               payload,
               sizeof(result));      
        return cb_CommandWriteIoPort(result);
      case 0x08: // IO port compare
        memcpy(&result,
               payload,
               sizeof(result)); 
        return cb_CommandOutputCompare(result);
      case 0x0A: // RTC init
         memcpy(&result,
               payload,
               sizeof(result));     
        return cb_CommandRtcInit(result);
      case 0x0B: // RTC set time
         memcpy(&result,
               payload,
               sizeof(result));         
        return cb_CommandRtcSetTime(result);
      case 0x0C: // RTC get time
        memcpy(&result,
               payload,
               sizeof(result)); 
        memcpy(&year,
               payload + sizeof(result),
               sizeof(year));
        month = payload[sizeof(result) + sizeof(year)];
        day = payload[sizeof(result) + sizeof(year) + sizeof(month)];
        weekday = payload[sizeof(result)+ sizeof(year) + sizeof(month) + sizeof(day)];
        hour = payload[sizeof(result)+ sizeof(year) + sizeof(month) +
                       sizeof(day) + sizeof(weekday)];
        minute = payload[sizeof(result)+ sizeof(year) + sizeof(month) + 
                         sizeof(day) + sizeof(weekday) + sizeof(hour)];
        second = payload[sizeof(result)+ sizeof(year) + sizeof(month) + 
                         sizeof(day) + sizeof(weekday) + sizeof(hour) + sizeof(minute)];          
        return cb_CommandRtcGetTime(result,
                                    year,
                                    month,
                                    day,
                                    weekday,
                                    hour,
                                    minute,
                                    second);
      case 0x0D: // RTC set alarm
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandSetAlarm(result);
      case 0x0E: // Configure UART
        memcpy(&result,
               payload,
               sizeof(result));       
        return cb_CommandConfigureUart(result);
      case 0x0F: // Get UART configuration
        memcpy(&result,
               payload,
               sizeof(result));       
        return cb_CommandGetUartConfiguration(result);      
      case 0x00: // set soft timer
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandSetSoftTimer(result);
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      case 0x01: // change notification
        break;
      case 0x02: // external interrupt
        break;
      case 0x03: // RTC alarm
        break;
      case 0x00: // soft timer
        handle = payload[0];
       return cb_EventSoftTimer(handle);
      default:
        return COMMAND_NOT_RECOGNIZED;
    }    
  }

  return NO_ERROR;
}


/**
 * @brief      Execute TCP stack callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executeTcpStackCallback(BgApiHeader *header,
                                                 uint8_t *payload,
                                                 const uint16_t payloadSize){
  uint16_t result;
  uint8_t index;
  uint8_t clientCount;
  Endpoint endpoint;
  IpAddress localIp;
  uint16_t localPort;
  IpAddress remoteIp;
  uint16_t remotePort;
  IpAddress address;
  DnsName * dnsName;
  DnsNameSize dnsNameSize;
  uint8_t * data;
  DataSize dataSize;
  uint8_t routingEnabled;
  uint32_t leaseTime;
  Netmask subnetMask;
  HardwareAddress hwAddress;

  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x04: // TCP configure
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandConfigureTcpIp(result);
      case 0x08: // DHCP set hostname
        memcpy(&result,
               payload,
               sizeof(result));      
        return cb_CommandSetDhcpHostName(result);
      case 0x05: // DNS configure
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandDnsConfigure(result);
      case 0x06: // DNS get host by name
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandGetDnsHostByName(result);
      case 0x01: // TCP connect
        memcpy(&result,
               payload,
               sizeof(result));
        endpoint = payload[sizeof(result)];
        return cb_CommandTcpConnect(result,
                                    endpoint);
      case 0x00: // Start TCP server
        memcpy(&result,
               payload,
               sizeof(result));
        endpoint = payload[sizeof(result)];
        return cb_CommandStartTcpServer(result,
                                        endpoint);
        break;
      case 0x03: // UDP connect
        memcpy(&result,
               payload,
               sizeof(result));
        endpoint = payload[sizeof(result)];
        return cb_CommandStartUdpConnect(result,
                                        endpoint);
      case 0x07: // UDP bind
        memcpy(&result,
               payload,
               sizeof(result));     
        return cb_CommandUdpBind(result);
      case 0x02: // start UDP server
        memcpy(&result,
               payload,
               sizeof(result));
        endpoint = payload[sizeof(result)];
        return cb_CommandStartUdpServer(result,
                                        endpoint);
      case 0x09: // dhcp enable routing
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandDhcpEnableRouting(result);
      case 0x0A: // set mDNS host name
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandSetMdnsHostName(result);
      case 0x0B: // start mDNS network
        memcpy(&result,
               payload,
               sizeof(result));      
        return cb_CommandStartMDns(result); 
      case 0x0C: // stop mDNS network
        memcpy(&result,
               payload,
               sizeof(result));         
        return cb_CommandStopMDNs(result);
      case 0x0D: // sd add service
        memcpy(&result,
               payload,
               sizeof(result));
        endpoint = payload[sizeof(result)];  
        return cb_CommandDnsSdAddService(result,
                                         endpoint);
      case 0x0E: // sd add service instance
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_CommandDnsSdAddServiceInstance(result); 
      case 0x0F: // DNS Sd Add Service attribute
        memcpy(&result,
               payload,
               sizeof(result));       
        return cb_CommandDnsSdAddServiceAttribute(result);
      case 0x10: // DNS Sd Remove Service
        memcpy(&result,
               payload,
               sizeof(result));          
        return cb_CommandDnsSdRemoveService(result);
      case 0x11: // DNS SD Start Service
        memcpy(&result,
               payload,
               sizeof(result));          
        return cb_CommandDnsSdStartService(result);
      case 0x12: // DSN SD Stop Service
        memcpy(&result,
               payload,
               sizeof(result));          
        return cb_CommandDnsSdStopService(result);
      case 0x13: // Multicast join
        memcpy(&result,
               payload,
               sizeof(result));          
        return cb_CommandMulticastJoin(result);   
      case 0x14: // Multicast leave
        memcpy(&result,
               payload,
               sizeof(result));          
        return cb_CommandMulticastLeave(result);
      case 0x15: // DHCP configure
        memcpy(&result,
               payload,
               sizeof(result));          
        return cb_CommandDhcpConfigure(result);  
      case 0x16: // list DHCP clients
        memcpy(&result,
               payload,
               sizeof(result));          
        clientCount = payload[sizeof(result)];
        return cb_CommandDhcpClients(result,
                                     clientCount);                          
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // configuration
        memcpy(address,
               payload,
               sizeof(address));
        memcpy(netmask,
               payload + sizeof(address),
               sizeof(netmask));
        memcpy(gateway,
               payload + sizeof(address) + sizeof(netmask),
               sizeof(gateway));
        useDhcp = payload[sizeof(address) + sizeof(netmask) + sizeof(gateway)];
        return cb_EventConfigureTcpIp(address,
                                      netmask,
                                      gateway,
                                      useDhcp);
      case 0x01: // configuration DNS
        index = payload[0];
        memcpy(address,
               payload + sizeof(index),
               sizeof(address));        
        return cb_EventDnsConfigureTcpIp(index,
                                         address);
      case 0x03: // Get host by name result
        memcpy(&result,
               payload,
               sizeof(result));
        memcpy(address,
               payload + sizeof(result),
               sizeof(address));
        memcpy(&dnsNameSize,
               payload + sizeof(result) + sizeof(address),
               sizeof(dnsNameSize));
        dnsName = payload + sizeof(result) + sizeof(address) + sizeof(dnsNameSize);
        return cb_EventGetDnsHostByName(result,
                                        address,
                                        dnsName,
                                        dnsNameSize);
      case 0x02: // endpoint status
        endpoint = payload[0];
        memcpy(localIp,
               payload + sizeof(endpoint),
               sizeof(localIp));
        memcpy(&localPort,
               payload + sizeof(endpoint) + sizeof(localIp),
               sizeof(localPort));
        memcpy(remoteIp,
               payload + sizeof(endpoint) + sizeof(localIp) + sizeof(localPort),
               sizeof(remoteIp));
        memcpy(&remotePort,
               payload + sizeof(endpoint) + sizeof(localIp) + sizeof(localPort) + sizeof(remoteIp),
               sizeof(remotePort));
        return cb_EventTcpIpEndpointStatus(endpoint,
                                          localIp,
                                          localPort,
                                          remoteIp,
                                          remotePort);
      case 0x04: //UDP data
        memcpy(&endpoint,
               payload,
               sizeof(endpoint);
        memcpy(&remoteIp,
               payload + sizeof(endpoint),
               sizeof(IpAddress));
        memcpy(&remotePort,
               payload + sizeof(endpoint) + sizeof(IpAddress),
               sizeof(remotePort));
        memcpy(&dataSize,
               payload + sizeof(endpoint) + sizeof(IpAddress) + sizeof(remotePort),
               sizeof(dataSize));
        data = payload + sizeof(endpoint) + sizeof(IpAddress) + sizeof(remotePort) + sizeof(dataSize);

        return cb_EventUdpData(endpoint,
                               remoteIp,
                               remotePort,
                               data,
                               dataSize);
      case 0x05: //mDSN started
        return cb_EventMDnsStarted();
      case 0x06: //mDSN failed
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_EventMDnsFailed(result);
      case 0x07: //mDSN stopped
        memcpy(&result,
               payload,
               sizeof(result));
        return cb_EventMDnsStopped(result); 
      case 0x08:
        index = payload[0];
        return cb_EventDnsSdServiceStarted(index); 
      case 0x09:
        memcpy(&result,
               payload,
               sizeof(result));
        index = payload[sizeof(result)];
        return cb_EventDnsSdServiceFailed(reason,
                                           index);
      case 0x0A:
        memcpy(&result,
               payload,
               sizeof(result));
        index = payload[sizeof(result)];
        return cb_EventDnsSdServiceStopped(reason,
                                           index); 
      case 0x0B:
        routingEnabled = payload[0];
        memcpy(&address,
               payload + sizeof(routingEnabled),
               sizeof(address));
        memcpy(subnetMask,
               payload + sizeof(routingEnabled) + sizeof(address),
               sizeof(subnetMask));
        memcpy(leaseTime,
               payload + sizeof(routingEnabled) + sizeof(address) + sizeof(subnetMask),
               sizeof(leaseTime));
        return cb_EventDhcpConfiguration(routingEnabled,
                                         address,
                                         subnetMask,
                                         leaseTime);
      case 0x0C:
        memcpy(address,
               payload,
               sizeof(address));
        memcpy(hwAddress,
               payload + sizeof(address),
               sizeof(hwAddress));
        return cb_EventDhcpClient(address,
                                  hwAddress);
      default:
        return COMMAND_NOT_RECOGNIZED;
    }    
  }

  return NO_ERROR;
}


/**
 * @brief      Execute wired ethernet callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executeWiredEthernetCallback(BgApiHeader *header,
                                                      uint8_t *payload,
                                                      const uint16_t payloadSize){
  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x02: // connected
        break;
      case 0x00: // set data route
        break;
      case 0x01: // close
        break;
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // link status
        break;
      default:
        return COMMAND_NOT_RECOGNIZED;
    }    
  }

  return NO_ERROR;
}


/**
 * @brief      Execute persistent store callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executePersistentStoreCallback(BgApiHeader *header,
                                                      uint8_t *payload,
                                                      const uint16_t payloadSize){
  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x03: // PS save
        break;
      case 0x04: // PS load
        break;
      case 0x07: // PS dump
        break;
      case 0x00: // PS defrag
        break;
      case 0x05: // PS erase
        break;
      case 0x02: // PS erase all
        break;
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      case 0x01: // PS keyy changed
        break;
      case 0x00: // PS key
        break;
      default:
        return COMMAND_NOT_RECOGNIZED;
    }    
  }

  return NO_ERROR;
}


/**
 * @brief      Execute http server callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executeHttpServerCallback(BgApiHeader *header,
                                                    uint8_t *payload,
                                                    const uint16_t payloadSize){
  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // enable
        break;
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      case 0x01: // button
        break;
      case 0x00: // on req
        break;
      default:
        return COMMAND_NOT_RECOGNIZED;
    }    
  }

  return NO_ERROR;
}


/**
 * @brief      Execute firmware upgrade callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executeDeviceFirmwareUpgradeCallback(BgApiHeader *header,
                                                              uint8_t *payload,
                                                              const uint16_t payloadSize){
  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // reset
        break;
      case 0x01: // flash set address
        break;
      case 0x02: // flash upload
        break;
      case 0x03: // flash upload finished
        break;
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // boot
        break;
      default:
        return COMMAND_NOT_RECOGNIZED;
    }    
  }

  return NO_ERROR;
}


/**
 * @brief      Execute i2c callback
 *
 * @param      header       The header
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: executeI2cCallback(BgApiHeader *header,
                                            uint8_t *payload,
                                            const uint16_t payloadSize){
  // Process command reply
  if(header->bit.msgType == CMD_RSP_TYPE){
    switch(header->bit.cmdId){
      case 0x00: // start read
        break;
      case 0x01: // start write
        break;
      case 0x02: // stop
        break;
      default:
        return COMMAND_NOT_RECOGNIZED;
    }
  }

  // Process event reply
  if(header->bit.msgType == EVENT_TYPE){
    switch(header->bit.cmdId){
      default:
        return COMMAND_NOT_RECOGNIZED;
    }    
  }

  return NO_ERROR;
}

/**
 * @brief      Execute any pending callbacks
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: ExecuteCallbacks(){
  ErrorCode err = NO_ERROR;
  BgApiHeader header;
  uint16_t payloadSize;
  ErrorCode cmdResult;

  //Some data is available in the buffer, process it
  err = getReplyHeader(&header);
  if(err != NO_ERROR) return err;

  // Get the payload if any. 
  payloadSize = getPayloadSizeFromHeader(&header);

  // Some data need to be processed
  // Should not block at that point
  if(payloadSize > 0){
    err = getReplyPayload(m_payloadBuffer, payloadSize);
    if(err != NO_ERROR) return err;
  }

  // If the command is a response, then the first two bytes of the reply are
  // result of the command.
  if(header.bit.msgType == CMD_RSP_TYPE){
    // If payload > 0 than zero, it means the result of the command is in the payload
    // except for some cases (like "hello system" that acknowledge the command without payload)
    if(payloadSize > 0){
        memcpy(&cmdResult,
               m_payloadBuffer,
               sizeof(cmdResult));
    
        // If there is an error, return the command failure immediately
        if(cmdResult != NO_ERROR){
          m_processingCmd = false;
          return cmdResult;
        }
    }

    m_processingCmd = false;
  }

  // Execute class specific callback
  // Some payload data is expected at that point
  switch(header.bit.classId){
    case CLASS_SYSTEM:
      err = executeSystemCallback(&header, m_payloadBuffer, payloadSize);
      break;
    case CLASS_CONFIGURATION:
      err = executeConfigurationCallback(&header, m_payloadBuffer, payloadSize);
      break;
    case CLASS_WIFI:
      err = executeWifiCallback(&header, m_payloadBuffer, payloadSize);
      break;
    case CLASS_TCP_STACK:
      err = executeTcpStackCallback(&header, m_payloadBuffer, payloadSize);
      break;
    case CLASS_ENDPOINT:
      err = executeEndpointCallback(&header, m_payloadBuffer, payloadSize);
      break;
    case CLASS_HARDWARE:
      err = executeHardwareCallback(&header, m_payloadBuffer, payloadSize);
      break;
    case CLASS_I2C:
      err = executeI2cCallback(&header, m_payloadBuffer, payloadSize);
      break;
    case CLASS_WIRED_ETHERNET:
      err = executeWiredEthernetCallback(&header, m_payloadBuffer, payloadSize);
      break;
    case CLASS_HTTP_SERVER:
      err = executeHttpServerCallback(&header, m_payloadBuffer, payloadSize);
      break;
    case CLASS_PERSISTENT_STORE:
      err = executePersistentStoreCallback(&header, m_payloadBuffer, payloadSize);
      break;
    case CLASS_DEVICE_FIRMWARE_UPGRADE:
      err = executeDeviceFirmwareUpgradeCallback(&header, m_payloadBuffer, payloadSize);
      break;
    default:
      return COMMAND_NOT_RECOGNIZED;
  }

  return err;
}


/**
 * @brief      Gets the reply header.
 *
 * @param      header  The header
 *
 * @return     The reply header.
 */
ErrorCode Wf121Driver :: getReplyHeader(BgApiHeader *header){
  Timeout counter;
  bool dataReady = false;

#if __USE_CTS_RTS__
  gioSetBit(gioPORTB,3,0);       // Pull low RTS : ready to receive some data
#endif //#if __USE_CTS_RTS__

  for(counter = BLOCKING_TIMEOUT_US; counter > 0; counter--){
    if(sciIsRxReady(SCI_REG)){
      dataReady = true;
      break;
    }
  }

  if(dataReady == false){
#if __USE_CTS_RTS__
    gioSetBit(gioPORTB,3,1);       // Pull low RTS : ready to receive some data
#endif //#if __USE_CTS_RTS__
    return TIMEOUT;
  }

  // Always receive 4 bytes to start the message
  sciReceive(SCI_REG, sizeof(header), (uint8_t*) header);

#if __USE_CTS_RTS__
  gioSetBit(gioPORTB,3,1);    // Release RTS
#endif //#if __USE_CTS_RTS__

  // Check for consistency of the message received
  if(header->bit.technologyType != TT_BLUETOOTH && header->bit.technologyType != TT_WIFI){
      return COMMAND_NOT_RECOGNIZED;
  }

  if(header->bit.msgType != CMD_RSP_TYPE && header->bit.msgType != EVENT_TYPE){
      return COMMAND_NOT_RECOGNIZED;
  }

  if(header->bit.classId > CLASS_WIRED_ETHERNET){
      return COMMAND_NOT_RECOGNIZED;
  }

  return NO_ERROR;
}

/**
 * @brief      Receive data payload from WF121 module
 *
 * @param      payload      The payload
 * @param[in]  payloadSize  The payload size
 *
 * @return     The error code.
 */
ErrorCode Wf121Driver :: getReplyPayload(uint8_t *payload,
                                         const uint16_t payloadSize){

  // Check if there is more data to read
  if(payload == NULL){
    return INVALID_PARAMETER;
  }

#if __USE_CTS_RTS__
  gioSetBit(gioPORTB,3,0);       // Pull low RTS : ready to receive some data
#endif //#if __USE_CTS_RTS__

  while(!sciIsRxReady(SCI_REG));
  sciReceive(SCI_REG, payloadSize, payload);

#if __USE_CTS_RTS__
  gioSetBit(gioPORTB,3,1);    // Release RTS
#endif //#if __USE_CTS_RTS__

  return NO_ERROR;
}


/**
 * @brief      Gets the payload size from header.
 *
 * @param      header  The header
 *
 * @return     The payload size from header.
 */
uint16_t Wf121Driver :: getPayloadSizeFromHeader(BgApiHeader *header){
    return header->bit.lengthLow + (header->bit.lengthHigh << 8);
}


/**
 * @brief      Sets the header payload size.
 *
 * @param      header  The header
 * @param[in]  size    The size
 */
void Wf121Driver :: setHeaderPayloadSize(BgApiHeader *header, const uint16_t size){
    header->bit.lengthLow = size & 0xFF;
    header->bit.lengthHigh = size >> 8 & 0x7;
}


/**
 * @brief      Return if a command is processing or not
 *
 * @return     If a command is processing then the function return true,
 *             otherwise it returns false.
 */
bool Wf121Driver :: CommandIsProcessing(){
  return m_processingCmd;
}

