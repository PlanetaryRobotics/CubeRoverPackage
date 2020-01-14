#ifndef __CUBEROVER_NETWORK_MANAGER__
#define __CUBEROVER_NETWORK_MANAGER__

#include "CubeRover/Wf121/Wf121.hpp"

#define TRIES_EXECUTE_CALLBACK    10000
#define ROVER_IP_ADDRESS        {192, 168, 1, 2}
#define ROVER_MASK_ADDRESS      {255, 255, 255, 0}
#define ROVER_GATEWAY_ADDRESS   {192, 168, 1, 3}

using namespace Wf121;

class CubeRoverNetworkManager : public Wf121Driver{
public:
  CubeRoverNetworkManager();
  ErrorCode InitializeCubeRoverNetworkManager();

  // Callback event
  ErrorCode cb_EventEndpointSyntaxError(const uint16_t result, const Endpoint endpoint);
  ErrorCode cb_EventBoot(const uint16_t major,
                         const uint16_t minor,
                         const uint16_t patch,
                         const uint16_t build,
                         const uint16_t bootloaderVersion,
                         const uint16_t tcpIpVersion,
                         const uint16_t hwVersion);
  ErrorCode cb_EventPowerSavingState(const PowerSavingState state);
  ErrorCode cb_EventMacAddress(const HardwareInterface interface,
                               const HardwareAddress hwAddr);

  // Callback command
  ErrorCode cb_CommandHelloSystem();
  ErrorCode cb_CommandTurnOnWifi(const uint16_t result);
  ErrorCode cb_CommandSetPowerSavingState(const uint16_t result);
  ErrorCode cb_CommandConfigureTcpIp(const uint16_t result);

private:
  bool m_wifiModuleDetected;
  bool m_wifiModuleTurnedOn;
  PowerSavingState m_powerSavingState;
  bool m_wifiModuleIdentified;
  bool m_macAddressIdentified;
  HardwareAddress m_macAddress;
  bool m_powerSavingStateSet;
  IpAddress m_roverIpAddress = ROVER_IP_ADDRESS;
  Netmask m_roverMaskAddress = ROVER_MASK_ADDRESS;
  Gateway m_udpGatewayAddress = ROVER_GATEWAY_ADDRESS;
};

#endif
