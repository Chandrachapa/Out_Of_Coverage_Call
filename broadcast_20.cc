
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/psc-module.h"
#include "ns3/stats-module.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/lte-module.h"


#include <ns3/callback.h>
#include <ns3/mcptt-call-machine-grp-broadcast.h>
#include <ns3/mcptt-call-machine-grp-broadcast-state.h>
#include <ns3/mcptt-call-msg.h>
#include <ns3/mcptt-ptt-app.h>
#include <ns3/mcptt-timer.h>
#include <iostream>
#include <ns3/mcptt-floor-msg.h>
#include <ns3/mcptt-floor-msg-field.h>
#include "ns3/ipv4-l3-protocol.h"

using namespace ns3;
//using namespace psc;

//initial environment :initial parameter configurations 
NS_LOG_COMPONENT_DEFINE ("broadcast_call_technique");

//waiting time 
double
waiting_time (double own_snr, double loc_accuracy, double other_device_snr, bool incoverage, double mySoc, double T_cycle )
{
double result;
if (mySoc < 20){
result = 1/(loc_accuracy +0.1)*(own_snr/other_device_snr)*(1/(incoverage+0.1))*(20/(mySoc))*T_cycle ; 
}
else {
result = 1/(loc_accuracy +0.1)*(own_snr/other_device_snr)*(1/(incoverage+0.1))*(20/20)*T_cycle;
}
return result;
}


//packet trace 
void
UePacketTrace (Ptr<OutputStreamWrapper> stream, const Address &localAddrs, std::string context, Ptr<const Packet> p, const Address &srcAddrs, const Address &dstAddrs)
{
  std::ostringstream oss;
  *stream->GetStream () << Simulator::Now ().GetNanoSeconds () / (double) 1e9 << "\t" << context << "\t" << p->GetSize () << "\t";
  if (InetSocketAddress::IsMatchingType (srcAddrs))
    {
      oss << InetSocketAddress::ConvertFrom (srcAddrs).GetIpv4 ();
      if (!oss.str ().compare ("0.0.0.0")) //srcAddrs not set
        {
          *stream->GetStream () << Ipv4Address::ConvertFrom (localAddrs) << ":" << InetSocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << InetSocketAddress::ConvertFrom (dstAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
        }
      else
        {
          oss.str ("");
          oss << InetSocketAddress::ConvertFrom (dstAddrs).GetIpv4 ();
          if (!oss.str ().compare ("0.0.0.0")) //dstAddrs not set
            {
              *stream->GetStream () << InetSocketAddress::ConvertFrom (srcAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" <<  Ipv4Address::ConvertFrom (localAddrs) << ":" << InetSocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
          else
            {
              *stream->GetStream () << InetSocketAddress::ConvertFrom (srcAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << InetSocketAddress::ConvertFrom (dstAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
        }
    }
  else if (Inet6SocketAddress::IsMatchingType (srcAddrs))
    {
      oss << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 ();
      if (!oss.str ().compare ("::")) //srcAddrs not set
        {
          *stream->GetStream () << Ipv6Address::ConvertFrom (localAddrs) << ":" << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
        }
      else
        {
          oss.str ("");
          oss << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 ();
          if (!oss.str ().compare ("::")) //dstAddrs not set
            {
              *stream->GetStream () << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << Ipv6Address::ConvertFrom (localAddrs) << ":" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
          else
            {
              *stream->GetStream () << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
        }
    }
  else
    {
      *stream->GetStream () << "Unknown address type!" << std::endl;
    }
}



int main (int argc, char *argv[])
{
  
bool useRelay = true;


// MCPTT configuration
//variable declarations for using push to talk 
uint32_t appCount;
uint32_t groupcount = 1;
uint32_t usersPerGroup =3;
DataRate dataRate = DataRate ("24kb/s");
uint32_t msgSize = 60; //60 + RTP header = 60 + 12 = 72
double maxX = 5.0;
double maxY = 5.0;
double pushTimeMean = 5.0; // seconds
double pushTimeVariance = 2.0; // seconds
double releaseTimeMean = 5.0; // seconds
double releaseTimeVariance = 2.0; // seconds
Time startTime = Seconds (1);
Time stopTime = Seconds (30);
TypeId socketFacTid = UdpSocketFactory::GetTypeId ();
//uint32_t groupId = 1;
//Ipv4Address peerAddress = Ipv4Address ("255.255.255.255");
appCount = usersPerGroup * groupcount;
// uint8_t        m_hopCount;


uint16_t floorPort = McpttPttApp::AllocateNextPortNumber ();
uint16_t speechPort = McpttPttApp::AllocateNextPortNumber ();
Ipv4AddressValue grpAddress;
//bool remoteUesOoc = true;

// set config for floor request and call generation
//McpttFloorMsgFieldSsrc ssrc;
McpttCallMsg msg;
McpttCallMsgFieldCallId m_callId; //!< The ID of the call.
McpttCallMsgFieldCallType m_callType; //!< The current call type.
McpttCallMsgFieldGrpId m_grpId; //!< The group ID of the call.
Callback<void, uint16_t> m_newCallCb; //!< The new call callback.
McpttCallMsgFieldUserId m_origId; //!< The originating MCPTT user's ID.
//McpttCall* m_owner; //!< The owner under which the call machine resides.
//uint8_t m_priority; //!< The ProSe per-packet priority.
Ptr<UniformRandomVariable> m_rndCallId; //!< The random number generator used for call ID selection.
McpttCallMsgFieldSdp m_sdp; //!< SDP information.
//bool m_started; //!< The flag that indicates if the state machine has been started.
Ptr<McpttCallMachineGrpBroadcastState> m_state; //!< The current state.
Callback<void, const McpttEntityId&, const McpttEntityId&> m_stateChangeCb; //!< The state changed callback.
TracedCallback<uint32_t, uint32_t, const std::string&, const std::string&, const std::string&> m_stateChangeTrace; //!< The state change traced callback.
//Ptr<McpttTimer> m_tfb1; //!< The timer TFB1.
//Ptr<McpttTimer> m_tfb2; //!< The timer TFB2.
//Ptr<McpttTimer> m_tfb3; //!< The timer TFB3.
//bool m_userAckReq; //!< Indicates if user acknowledgments are required.
Time delayTfb1 = Seconds(10);
Time delayTfb2 = Seconds(5);
Time delayTfb3 = Seconds(5);

//Physical layer 

//creating 3 nodes
  NodeContainer nodes;
  nodes.Create(appCount);
  
  NodeContainer gnBnode;
  gnBnode.Create(1);
  
//positioning nodes
NS_LOG_INFO ("Building physical topology...");
Ptr<RandomBoxPositionAllocator> rndBoxPosAllocator = CreateObject <RandomBoxPositionAllocator> ();
rndBoxPosAllocator->SetX (CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (0.0), "Max", DoubleValue (maxX)));
rndBoxPosAllocator->SetY (CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (0.0), "Max", DoubleValue (maxY)));
rndBoxPosAllocator->SetZ (CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (1.5)));


Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint32_t count = 0; count < appCount; count++)
    {
      Vector position = rndBoxPosAllocator->GetNext ();

      NS_LOG_INFO ("UE " << (count + 1) << " located at " << position << ".");

      positionAlloc->Add (position);
    }

 //eNodeB
  Ptr<ListPositionAllocator> positionAllocGnb = CreateObject<ListPositionAllocator> ();
  positionAllocGnb->Add (Vector (0.0, 0.0, 30.0));

  //eNodeB
  MobilityHelper mobilityeNodeB;
  mobilityeNodeB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityeNodeB.SetPositionAllocator (positionAllocGnb);
  mobilityeNodeB.Install (gnBnode);


//mobility of nodes set to stationary
MobilityHelper mobility;
mobility.SetPositionAllocator (positionAlloc);
mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
mobility.Install (nodes);

//AsciiTraceHelper ascii;
//mobility.EnableAsciiAll (ascii.CreateFileStream ("MobilityTrace.txt"));

//sidelink pre-configuration
//Configure the UE for UE_SELECTED scenario
Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (8));
Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (5)); //The number of RBs allocated per UE for Sidelink
Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1));
Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (true)); //use default Trp index of 0
//Config::SetDefault ("ns3::LteUeMac::SlScheduler", StringValue ("MaxCoverage")); //Values include Fixed, Random, MinPrb, MaxCoverage

//for tracing
Config::SetDefault ("ns3::McpttMsgStats::CallControl", BooleanValue (true));
Config::SetDefault ("ns3::McpttMsgStats::FloorControl", BooleanValue (true));
Config::SetDefault ("ns3::McpttMsgStats::Media", BooleanValue (true));
Config::SetDefault ("ns3::McpttMsgStats::IncludeMessageContent", BooleanValue (true));


//Set the frequency
uint32_t ulEarfcn = 18100;
uint16_t ulBandwidth = 50;

// Set error models
Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));
Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (false));
  
//Set the UEs power in dBm
Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));


//Sidelink bearers activation time
//Time slBearersActivationTime = startTime;

//Create the helpers
Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

//Create and set the EPC helper
Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
//lteHelper->SetEpcHelper (epcHelper);

//Create Sidelink helper and set lteHelper
Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
proseHelper->SetLteHelper (lteHelper);
Config::SetDefault ("ns3::LteSlBasicUeController::ProseHelper",PointerValue (proseHelper));

//Enable Sidelink
lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));

//Set pathloss model
lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Cost231PropagationLossModel"));

// channel model initialization
lteHelper->Initialize ();

// Since we are not installing eNB, we need to set the frequency attribute of pathloss model here
double ulFreq = LteSpectrumValueHelper::GetCarrierFrequency (ulEarfcn);
NS_LOG_LOGIC ("UL freq: " << ulFreq);
Ptr<Object> uplinkPathlossModel = lteHelper->GetUplinkPathlossModel ();
Ptr<PropagationLossModel> lossModel = uplinkPathlossModel->GetObject<PropagationLossModel> ();
NS_ABORT_MSG_IF (lossModel == NULL, "No PathLossModel");
bool ulFreqOk = uplinkPathlossModel->SetAttributeFailSafe ("Frequency", DoubleValue (ulFreq));
  if (!ulFreqOk)
    {
      NS_LOG_WARN ("UL propagation model does not have a Frequency attribute");
    }
NetDeviceContainer devices;
 // NetDeviceContainer devices = lteHelper->InstallUeDevice (nodes);
Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
ueSidelinkConfiguration->SetSlEnabled (true);

LteRrcSap::SlPreconfiguration preconfiguration;

preconfiguration.preconfigGeneral.carrierFreq = ulEarfcn;
preconfiguration.preconfigGeneral.slBandwidth = ulBandwidth;
preconfiguration.preconfigComm.nbPools = 1;

//-Configure preconfigured communication pool
preconfiguration.preconfigComm = proseHelper->GetDefaultSlPreconfigCommPoolList ();

//-Configure preconfigured discovery pool
preconfiguration.preconfigDisc = proseHelper->GetDefaultSlPreconfigDiscPoolList ();

//-Configure preconfigured UE-to-Network Relay parameters
preconfiguration.preconfigRelay = proseHelper->GetDefaultSlPreconfigRelay ();
  
//-Enable discovery
ueSidelinkConfiguration->SetDiscEnabled (true);
//-Set frequency for discovery messages monitoring //this is for another scenario with remote host
//ueSidelinkConfiguration->SetDiscInterFreq (ueDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlEarfcn ());
  
LteRrcSap::SlCommTxResourcesSetup pool;
pool.setup = LteRrcSap::SlCommTxResourcesSetup::UE_SELECTED;
pool.ueSelected.havePoolToRelease = false;
pool.ueSelected.havePoolToAdd = true;
pool.ueSelected.poolToAddModList.nbPools = 1;
pool.ueSelected.poolToAddModList.pools[0].poolIdentity = 1;

LteSlPreconfigPoolFactory pfactory;

//Control
pfactory.SetControlPeriod ("sf40");
pfactory.SetControlBitmap (0x00000000FF); //8 subframes for PSCCH
pfactory.SetControlOffset (0);
pfactory.SetControlPrbNum (22);
pfactory.SetControlPrbStart (0);
pfactory.SetControlPrbEnd (49);

//Data
pfactory.SetDataBitmap (0xFFFFFFFFFF);
pfactory.SetDataOffset (8); //After 8 subframes of PSCCH
pfactory.SetDataPrbNum (25);
pfactory.SetDataPrbStart (0);
pfactory.SetDataPrbEnd (49);

//synchronization

preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();
pfactory.SetHaveUeSelectedResourceConfig (true);


  //NetDeviceContainer relayUeDevs = lteHelper->InstallUeDevice (relayUeNodes);
  //NetDeviceContainer remoteUeDevs = lteHelper->InstallUeDevice (remoteUeNodes);
  //NetDeviceContainer devices =lteHelper->InstallUeDevice (nodes);

ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
lteHelper->InstallSidelinkConfiguration (devices, ueSidelinkConfiguration);
    LteRrcSap::SlPreconfiguration preconfigurationRemote;
  LteRrcSap::SlPreconfiguration preconfigurationRelay;

  if (useRelay)
    {
      //General
      preconfigurationRemote.preconfigGeneral.carrierFreq = 23330;
      preconfigurationRemote.preconfigGeneral.slBandwidth = 50;

      //Discovery
      preconfigurationRemote.preconfigDisc.nbPools = 1;
      LteSlDiscPreconfigPoolFactory preconfDiscPoolFactory;
      preconfDiscPoolFactory.SetDiscCpLen ("NORMAL");
      preconfDiscPoolFactory.SetDiscPeriod ("rf32");
      preconfDiscPoolFactory.SetNumRetx (0);
      preconfDiscPoolFactory.SetNumRepetition (1);
      preconfDiscPoolFactory.SetDiscPrbNum (10);
      preconfDiscPoolFactory.SetDiscPrbStart (10);
      preconfDiscPoolFactory.SetDiscPrbEnd (40);
      preconfDiscPoolFactory.SetDiscOffset (0);
      preconfDiscPoolFactory.SetDiscBitmap (0x11111);
      preconfDiscPoolFactory.SetDiscTxProbability ("p100");

      preconfigurationRemote.preconfigDisc.pools[0] = preconfDiscPoolFactory.CreatePool ();

      //Communication
      preconfigurationRemote.preconfigComm.nbPools = 1;
      LteSlPreconfigPoolFactory preconfCommPoolFactory;
      //-Control
      preconfCommPoolFactory.SetControlPeriod ("sf40");
      preconfCommPoolFactory.SetControlBitmap (0x00000000FF); //8 subframes for PSCCH
      preconfCommPoolFactory.SetControlOffset (0);
      preconfCommPoolFactory.SetControlPrbNum (22);
      preconfCommPoolFactory.SetControlPrbStart (0);
      preconfCommPoolFactory.SetControlPrbEnd (49);
      //-Data
      preconfCommPoolFactory.SetDataBitmap (0xFFFFFFFFFF);
      preconfCommPoolFactory.SetDataOffset (8); //After 8 subframes of PSCCH
      preconfCommPoolFactory.SetDataPrbNum (25);
      preconfCommPoolFactory.SetDataPrbStart (0);
      preconfCommPoolFactory.SetDataPrbEnd (49);

      preconfigurationRemote.preconfigComm.pools[0] = preconfCommPoolFactory.CreatePool ();

      //-Relay UE (re)selection
      preconfigurationRemote.preconfigRelay.haveReselectionInfoOoc = true;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.filterCoefficient = 0;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.minHyst = 0;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.qRxLevMin = -125;
    }


/*Network layer***************************************************************************************/
//installing wifi interface in nodes

// WifiHelper wifi;
// wifi.SetStandard (WIFI_PHY_STANDARD_80211g); //2.4Ghz
// wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
//                                 "DataMode", StringValue ("ErpOfdmRate54Mbps"));
 
// WifiMacHelper wifiMac;
// YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
// YansWifiChannelHelper wifiChannel;
// wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
// wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel",
//                                 "Frequency", DoubleValue (2.407e9)); //2.4Ghz

// wifiMac.SetType ("ns3::AdhocWifiMac");

// YansWifiPhyHelper phy = wifiPhy;
// phy.SetChannel (wifiChannel.Create ());

// WifiMacHelper mac = wifiMac;
// devices = wifi.Install (phy, mac, nodes);

//installing internet access to all nodes
NS_LOG_INFO ("Installing internet stack on all nodes...");
InternetStackHelper internet;
internet.Install (nodes);
  
// assigning ip addresses to all devices of nodes
NS_LOG_INFO ("Assigning IP addresses to each net device...");


  uint32_t groupL2Address = 255;
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4Address groupAddress4 = Ipv4Address ("255.255.255.255");
 // Ipv4Address groupAddress4("225.0.0.0");
  //Ipv4Address groupAddress4 ("225.0.0.0");     //use multicast address as destination
  Ipv6Address groupAddress6 ("ff0e::1");     //use multicast address as destination
  Address remoteAddress;
  Address localAddress;
  Ptr<LteSlTft> tft;
  bool useIPv6 = false;
  if (!useIPv6)
    {
      Ipv4InterfaceContainer ueIpIface;
      ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (devices));

      // set the default gateway for the UE
      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      for (uint32_t u = 0; u < nodes.GetN (); ++u)
        {
          Ptr<Node> ueNode = nodes.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
        }
      remoteAddress = InetSocketAddress (groupAddress4, 8000);
      localAddress = InetSocketAddress (Ipv4Address::GetAny (), 8000);
      tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupAddress4, groupL2Address);
    }
  else
    {
      Ipv6InterfaceContainer ueIpIface;
      ueIpIface = epcHelper->AssignUeIpv6Address (NetDeviceContainer (devices));

      // set the default gateway for the UE
      Ipv6StaticRoutingHelper ipv6RoutingHelper;
      for (uint32_t u = 0; u < nodes.GetN (); ++u)
        {
          Ptr<Node> ueNode = nodes.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv6StaticRouting> ueStaticRouting = ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
        }
      remoteAddress = Inet6SocketAddress (groupAddress6, 8000);
      localAddress = Inet6SocketAddress (Ipv6Address::GetAny (), 8000);
      tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupAddress6, groupL2Address);
    }


  // Ipv6AddressHelper ipv6h;
  // ipv6h.SetBase (Ipv6Address ("6001:db80::"), Ipv6Prefix (64));
  // //Ipv6InterfaceContainer internetIpIfaces = ipv6h.Assign (internetDevices);
  // Ipv6InterfaceContainer internet_uedevices = ipv6h.Assign (devices);
  //  internet_uedevices.SetForwarding (0, true);
  //  internet_uedevices.SetDefaultRouteInAllNodes (0);
  //   if (!useRelay)
  //   {
  //     lteHelper->Attach (devices.Get (1));
  //   }


  // Ipv6Address remoteUeaddress =  internet_uedevices.GetAddress(0,1);
  // Ipv6Address relayUeaddress =  internet_uedevices.GetAddress(1,1);
 
  // std::cout << "remote ue address : " << remoteUeaddress << std::endl;
  // std::cout << "relay ue address : " << relayUeaddress << std::endl;

  

/*****************************


Ipv4AddressHelper ipv4;
ipv4.SetBase ("10.1.1.0", "255.255.255.0");
Ipv4InterfaceContainer i = ipv4.Assign (devices);*/
 
ns3::PacketMetadata::Enable ();
/*Application layer*************************************************************************************/

// NS_LOG_INFO ("Creating applications...");
// uint16_t echoPort = 8000;

//   //Set echo server in the remote host

//   UdpEchoServerHelper echoServer (echoPort);
//   ApplicationContainer serverApps = echoServer.Install (nodes);
//   serverApps.Start (Seconds (1.0));
//   serverApps.Stop (stopTime);

//   //std::cout << remoteHost->GetId () << std::endl;

//   //Set echo client in the UEs
//   UdpEchoClientHelper echoClient (remoteUeaddress, echoPort);
//   echoClient.SetAttribute ("MaxPackets", UintegerValue (20));
//   echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
//   echoClient.SetAttribute ("PacketSize", UintegerValue (200));

//   ApplicationContainer udpclientApps;
//   udpclientApps = echoClient.Install (nodes.Get (1)); //Only the remote UE will have traffic
//   udpclientApps.Start (Seconds (2.0));
//   udpclientApps.Stop (stopTime);

//creating mcptt application for each device 
McpttHelper mcpttHelper;
ApplicationContainer clientApps;

McpttTimer mcpttTimer;
  
//creating mcptt service on each node

mcpttHelper.SetPttApp ("ns3::McpttPttApp",
                        "PeerAddress", Ipv4AddressValue (groupAddress4), 
                        "PushOnStart", BooleanValue (true));
mcpttHelper.SetMediaSrc ("ns3::McpttMediaSrc",
                        "Bytes", UintegerValue (msgSize),
                        "DataRate", DataRateValue (dataRate));
mcpttHelper.SetPusher ("ns3::McpttPusher",
                        "Automatic", BooleanValue (false));
mcpttHelper.SetPusherPushVariable ("ns3::NormalRandomVariable",
                        "Mean", DoubleValue (pushTimeMean),
                        "Variance", DoubleValue (pushTimeVariance));
mcpttHelper.SetPusherReleaseVariable ("ns3::NormalRandomVariable",
                        "Mean", DoubleValue (releaseTimeMean),
                        "Variance", DoubleValue (releaseTimeVariance));

mcpttHelper.EnableStateMachineTraces();       

clientApps.Add (mcpttHelper.Install (nodes));
clientApps.Start (startTime);
clientApps.Stop (stopTime);

/*
// PacketSinkHelper sidelinkSink ("ns3::UdpSocketFactory", localAddress);
// ApplicationContainer serverApps = sidelinkSink.Install (nodes.Get (1));
// serverApps.Start (startTime);
// serverApps.Stop (stopTime);
*/

  //Set Sidelink bearers
  //proseHelper->ActivateSidelinkBearer (Seconds(startTime), devices, tft);

/*Call flow process************************************************************************************/

//set call owner, call id, mcptt id, call type, etc.**************************************



//create floor and call control machines************************************************ 
ObjectFactory callFac;
callFac.SetTypeId ("ns3::McpttCallMachineGrpBroadcast");

ObjectFactory floorFac;
floorFac.SetTypeId ("ns3::McpttFloorMachineBasic");
  
Ipv4AddressValue grpAddr;

//creating location to store application info of UEs A, B 
Ptr<McpttPttApp> ueAPttApp = DynamicCast<McpttPttApp, Application> (clientApps.Get (0));
Ptr<McpttPttApp> ueBPttApp = DynamicCast<McpttPttApp, Application> (clientApps.Get (1));
Ptr<McpttPttApp> ueCPttApp = DynamicCast<McpttPttApp, Application> (clientApps.Get (2));

  //UE A
  uint32_t grpId = 1;
  uint16_t callId = 1;
  
//McpttCallMsgFieldUserLoc m_userLoc;

  //UE B 
  std::string orgName = "EMS";
  Time joinTime = Seconds (2.2);
  uint32_t origId = ueAPttApp->GetUserId ();
  int no_devices = ueAPttApp->GetNode()->GetNDevices();
for (int i=0;i<no_devices;i++){

  std::cout << "A address" << ueAPttApp->GetNode()->GetDevice(i)->GetAddress() << std::endl;

}

  //fix this // device ekka id within the range 
  // 0.0.0.0

  McpttCallMsgFieldSdp sdp;
  sdp.SetFloorPort (floorPort);
  sdp.SetGrpAddr (grpAddress.Get ());
  //sdp.SetOrigAddr (origAddress);
  sdp.SetSpeechPort (speechPort);

 
//floor and call machines generate*******************************
ueAPttApp->CreateCall (callFac, floorFac);
ueAPttApp->SelectLastCall ();
ueBPttApp->CreateCall (callFac, floorFac);
ueBPttApp->SelectLastCall ();
ueCPttApp->CreateCall (callFac, floorFac);
ueCPttApp->SelectLastCall ();


//creating call interfaces and location to store call of UEs A, B 
Ptr<McpttCall> ueACall = ueAPttApp->GetSelectedCall ();
Ptr<McpttCall> ueBCall = ueBPttApp->GetSelectedCall ();
Ptr<McpttCall> ueCCall = ueCPttApp->GetSelectedCall ();


//Ptr<McpttCallMachine> ueAMachine = ueACall->GetCallMachine ();
Ptr<McpttCallMachineGrpBroadcast> Abroadcastgroupmachine = DynamicCast<McpttCallMachineGrpBroadcast, McpttCallMachine>(ueACall->GetCallMachine ());
Ptr<McpttCallMachineGrpBroadcast> Bbroadcastgroupmachine =  DynamicCast<McpttCallMachineGrpBroadcast, McpttCallMachine>(ueBCall->GetCallMachine ());
Ptr<McpttCallMachineGrpBroadcast> Cbroadcastgroupmachine =  DynamicCast<McpttCallMachineGrpBroadcast, McpttCallMachine>(ueBCall->GetCallMachine ());


  //UE A 
  Abroadcastgroupmachine->SetCallId (callId);
  Abroadcastgroupmachine->SetGrpId (grpId);
  Abroadcastgroupmachine->SetOrigId (origId);
  Abroadcastgroupmachine->SetSdp (sdp);
  Abroadcastgroupmachine->SetCallType (McpttCallMsgFieldCallType::BROADCAST_GROUP);
  Abroadcastgroupmachine->SetPriority (McpttCallMsgFieldCallType::GetCallTypePriority (McpttCallMsgFieldCallType::BROADCAST_GROUP));
  
  McpttCallMachineGrpBroadcastStateB2::GetInstance ();
 
  // UE B
  Bbroadcastgroupmachine ->SetCallId (callId);
  Bbroadcastgroupmachine ->SetGrpId (grpId);
  Bbroadcastgroupmachine ->SetOrigId (origId);
  Bbroadcastgroupmachine ->SetSdp (sdp);
  Bbroadcastgroupmachine ->SetCallType (McpttCallMsgFieldCallType::BROADCAST_GROUP);
  Bbroadcastgroupmachine ->SetPriority (McpttCallMsgFieldCallType::GetCallTypePriority (McpttCallMsgFieldCallType::BROADCAST_GROUP));
  McpttCallMachineGrpBroadcastStateB2::GetInstance ();
 

  // UE C
  Cbroadcastgroupmachine ->SetCallId (callId);
  Cbroadcastgroupmachine ->SetGrpId (grpId);
  Cbroadcastgroupmachine ->SetOrigId (origId);
  Cbroadcastgroupmachine ->SetSdp (sdp);
  Cbroadcastgroupmachine ->SetCallType (McpttCallMsgFieldCallType::BROADCAST_GROUP);
  Cbroadcastgroupmachine ->SetPriority (McpttCallMsgFieldCallType::GetCallTypePriority (McpttCallMsgFieldCallType::BROADCAST_GROUP));
  McpttCallMachineGrpBroadcastStateB2::GetInstance ();

  //push button press schedule

  Simulator::Schedule (Seconds (2.2), &McpttPttApp::TakePushNotification, ueAPttApp);
  McpttCallMachineGrpBroadcastStateB1::GetStateId ();
  McpttCallMachineGrpBroadcastStateB1::GetInstance ();
  Ptr<McpttChan> AcallChan = ueAPttApp->GetCallChan ();
  Ptr<McpttChan> BcallChan = ueBPttApp->GetCallChan ();
  Ptr<McpttChan> CcallChan = ueCPttApp->GetCallChan ();

  // UE A
  Abroadcastgroupmachine->SetCallId (callId);
  Abroadcastgroupmachine->SetGrpId (grpId);
  Abroadcastgroupmachine->SetOrigId (origId);
  Abroadcastgroupmachine->SetSdp (sdp);
  Abroadcastgroupmachine->SetCallType (McpttCallMsgFieldCallType::BROADCAST_GROUP);
  Abroadcastgroupmachine->SetPriority (McpttCallMsgFieldCallType::GetCallTypePriority (McpttCallMsgFieldCallType::BROADCAST_GROUP));
  

Ptr<McpttPusher> ueAPusher = ueAPttApp->GetPusher ();
Ptr<McpttPusher> ueBPusher = ueBPttApp->GetPusher ();
Ptr<McpttPusher> ueCPusher = ueCPttApp->GetPusher ();

Ptr<McpttMediaSrc> ueAMediaSrc = ueAPttApp->GetMediaSrc ();
Ptr<McpttMediaSrc> ueBMediaSrc = ueBPttApp->GetMediaSrc ();
Ptr<McpttMediaSrc> ueCMediaSrc = ueCPttApp->GetMediaSrc ();


McpttCallMachineGrpBroadcast AbroadcastMachines;
AbroadcastMachines.SetDelayTfb1(delayTfb1);
AbroadcastMachines.SetDelayTfb2(delayTfb2);
AbroadcastMachines.SetDelayTfb3(delayTfb3);

Ptr<McpttTimer> Atfb1 =AbroadcastMachines.GetTfb1();
Ptr<McpttTimer> Atfb2 =AbroadcastMachines.GetTfb2();
Ptr<McpttTimer> Atfb3 =AbroadcastMachines.GetTfb3();
Ptr<McpttCallMachineGrpBroadcastState> stateA = AbroadcastMachines.GetState ();


McpttCallMachineGrpBroadcast BbroadcastMachines;
BbroadcastMachines.SetDelayTfb1(delayTfb1);
BbroadcastMachines.SetDelayTfb3(delayTfb3);

Ptr<McpttTimer> Btfb1 =BbroadcastMachines.GetTfb1();
Ptr<McpttTimer> Btfb3 =BbroadcastMachines.GetTfb3();
Ptr<McpttCallMachineGrpBroadcastState> stateB = BbroadcastMachines.GetState ();

McpttCallMachineGrpBroadcast CbroadcastMachines;
CbroadcastMachines.SetDelayTfb1(delayTfb1);
CbroadcastMachines.SetDelayTfb3(delayTfb3);

Ptr<McpttTimer> Ctfb1 =CbroadcastMachines.GetTfb1();
Ptr<McpttTimer> Ctfb3 =CbroadcastMachines.GetTfb3();
Ptr<McpttCallMachineGrpBroadcastState> stateC = CbroadcastMachines.GetState ();


//generating floor control message 
  McpttFloorMsgFieldIndic indic = McpttFloorMsgFieldIndic ();
  indic.Indicate (McpttFloorMsgFieldIndic::BROADCAST_CALL);

  McpttFloorMsgFieldPriority priority = McpttFloorMsgFieldPriority ();
  priority.SetPriority (1);

  McpttFloorMsgFieldTrackInfo trackInfo = McpttFloorMsgFieldTrackInfo ();
  trackInfo.SetQueueCap (1);
  trackInfo.AddRef (5);

  McpttFloorMsgFieldUserId id = McpttFloorMsgFieldUserId ();
  id.SetUserId (9);
  
  McpttCallMsgFieldCallId call_Id;
  call_Id.SetCallId (callId);

  McpttCallMsgFieldCallType callType;
  callType.SetType (McpttCallMsgFieldCallType::EMERGENCY_GROUP);

  McpttCallMsgFieldGrpId grp_Id;
  grp_Id.SetGrpId (grpId);

  McpttCallMsgFieldLastChgTime lastChgTime;
  lastChgTime.SetTime (Seconds (4));

  McpttCallMsgFieldUserId lastChgUserId;
  lastChgUserId.SetId (15);

Ptr<Packet> pkt = Create<Packet> ();
pkt->AddHeader (msg);


//callChan->Send (pkt);  o
 
NS_LOG_LOGIC (Simulator::Now ().GetSeconds () << "s: PttApp sending " << msg << ".");


//start timer TFB1 and TFB2***************************
//establish media session***************************** 
//release button : send call***************************
  Simulator::Schedule (Seconds (2.1), &McpttTimer::Start, Atfb1);
  Simulator::Schedule (Seconds (2.1), &McpttTimer::Start, Atfb2);
  Simulator::Schedule (Seconds (2.1), &McpttTimer::Start, Btfb1);
  Simulator::Schedule (Seconds (2.1), &McpttTimer::Start, Ctfb1);
  McpttCallMachineGrpBroadcastStateB2::GetStateId ();
  
  Simulator::Schedule (Seconds (2.15), &McpttCall::OpenFloorChan, ueACall, grpAddress.Get (), floorPort);
  Simulator::Schedule (Seconds (2.15), &McpttCall::OpenMediaChan, ueACall, grpAddress.Get (), speechPort);
  Simulator::Schedule (Seconds (2.15), &McpttCall::OpenFloorChan, ueBCall, grpAddress.Get (), floorPort);
  Simulator::Schedule (Seconds (2.15), &McpttCall::OpenMediaChan, ueBCall, grpAddress.Get (), speechPort);
  Simulator::Schedule (Seconds (2.15), &McpttCall::OpenFloorChan, ueCCall, grpAddress.Get (), floorPort);
  Simulator::Schedule (Seconds (2.15), &McpttCall::OpenMediaChan, ueCCall, grpAddress.Get (), speechPort);
  
//// synchronization and call in progress 
 //UdpEchoServer listening in the Remote UE port


//end call
 Simulator::Schedule (Seconds (5.25), &McpttPttApp::ReleaseCall, ueAPttApp);

//broadcast end message
McpttCallMsgGrpBroadcastEnd Endmsg;
//B, C receive end message
BbroadcastMachines.ReceiveGrpCallBroadcastEnd(Endmsg);
CbroadcastMachines.ReceiveGrpCallBroadcastEnd(Endmsg);
Simulator::Schedule (Seconds (5.25), &McpttTimer::Stop, Atfb1);
Simulator::Schedule (Seconds (5.25), &McpttTimer::Stop, Atfb2);
Simulator::Schedule (Seconds (5.25), &McpttTimer::Stop, Btfb1);
Simulator::Schedule (Seconds (5.25), &McpttTimer::Stop, Ctfb1);

//Result generation*******************************************
//Packets traces

ns3::PacketMetadata::Enable ();
AsciiTraceHelper ascii;
Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("b20.tr");
//wifiPhy.EnableAsciiAll (stream);
internet.EnableAsciiIpv4All (stream);
*stream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;


  AsciiTraceHelper ascii_wifi;
  //wifiPhy.EnableAsciiAll (ascii_wifi.CreateFileStream ("wifi-packet-socket.tr"));
  

  AsciiTraceHelper ascii_r;
  Ptr<OutputStreamWrapper> rtw = ascii_r.CreateFileStream ("routing_table");

  AsciiTraceHelper ascii_trace;

  std::ostringstream oss;
  Ptr<OutputStreamWrapper> packetOutputStream = ascii_trace.CreateFileStream ("b20_trace.txt");
  *packetOutputStream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;


  //Trace file table header
  *stream->GetStream () << "time(sec)\ttx/rx\tNodeID\tIMSI\tPktSize(bytes)\tIP[src]\tIP[dst]" << std::endl;
  /*******************************************************/
    //Packets traces
  Ptr<OutputStreamWrapper> appStream = ascii.CreateFileStream ("AppPacketTrace.txt");
  *appStream->GetStream () << "time(s)\ttx/rx\tNodeID\tIMSI\tPktSize(bytes)\tIP[src]\tIP[dst]" << std::endl;


  auto localAddrs =  clientApps.Get(0)->GetNode()->GetObject<Ipv4L3Protocol> ();
  std::cout << "ip address" << localAddrs << std::endl;
  // //Upward: Tx by UE / Rx by Remote Host
  // oss << "rx-RH\t" << serverApps.Get (0)->GetNode ()->GetId () << "\t" << "-";
  // serverApps.Get (0)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, appStream, localAddrs));
  // oss.str ("");

  // //Downward: Tx by Remote Host / Rx by UE
  // oss << "tx-RH\t" << serverApps.Get (0)->GetNode ()->GetId () << "\t" << "-";
  // serverApps.Get (0)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, appStream, localAddrs));
  // oss.str ("");



NS_LOG_INFO ("Enabling MCPTT traces...");
mcpttHelper.EnableMsgTraces ();
mcpttHelper.EnableStateMachineTraces ();
  
NS_LOG_INFO ("Starting simulation...");
AnimationInterface anim("b20.xml");
anim.SetMaxPktsPerTraceFile(500000);
std::cout << "okay" << std::endl;
//Simulator::Stop (Seconds (stopTime));
//anim.SetConstantPosition(nodes.Get(0),1.0,2.0);
//anim.SetConstantPosition(nodes.Get(1),4.0,5.0);
Simulator::Run ();
Simulator::Destroy();

NS_LOG_UNCOND ("Done Simulator");

NS_LOG_INFO ("Done.");

  
}
