
#include "ns3/lte-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store.h"
#include <cfloat>
#include <sstream>
#include <math.h>
#include "ns3/gnuplot.h"
#include "ns3/netanim-module.h"
#include <ns3/mcptt-helper.h>
#include <ns3/wifi-module.h>
#include "ns3/lte-net-device.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("lte-sl-relay-cluster_3");


void
TraceSinkPC5SignalingPacketTrace (Ptr<OutputStreamWrapper> stream, uint32_t srcL2Id, uint32_t dstL2Id, Ptr<Packet> p)
{
  LteSlPc5SignallingMessageType lpc5smt;
  p->PeekHeader (lpc5smt);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << srcL2Id << "\t" << dstL2Id << "\t" << lpc5smt.GetMessageName () << std::endl;
}

int main (int argc, char *argv[])
{ 
  // bool enableNsLogs = true; 
  // relay and remote ue  declaration
  double simTime = 1.0; //s //Simulation time (in seconds) updated automatically based on number of nodes
  double relayRadius = 300.0; //m
  double remoteRadius = 50.0; //m
  uint32_t nRelayUes = 2;
  uint32_t nRemoteUesPerRelay = 5;
  bool remoteUesOoc = true;
  std::string echoServerNode ("RemoteUE");

  
  double startTimeRelay [nRelayUes];
  double startTimeRemote [nRelayUes * nRemoteUesPerRelay];

  double timeBetweenRemoteStarts = 4 * 0.32 + 0.32; //s


    double timeBetweenRelayStarts = 1.0 + nRemoteUesPerRelay * ((2 + 2 * nRemoteUesPerRelay) * 0.04 + timeBetweenRemoteStarts); //s

  for (uint32_t ryIdx = 0; ryIdx < nRelayUes; ryIdx++)
    {
      startTimeRelay [ryIdx] = 2.0 + 0.320 + timeBetweenRelayStarts * ryIdx;



      for (uint32_t rm = 0; rm < nRemoteUesPerRelay; ++rm)
        {
          uint32_t rmIdx = ryIdx * nRemoteUesPerRelay + rm;
          startTimeRemote [rmIdx] = startTimeRelay [ryIdx] + timeBetweenRemoteStarts * (rm + 1);
        }
    }

   simTime = startTimeRemote [(nRelayUes * nRemoteUesPerRelay - 1)] + 3.0 + 10.0; //s
  
  //Configure the UE for UE_SELECTED scenario
  Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (16));
  Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (6)); //The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1));
  Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (true));
  
    //connection with the gNB
  //Set the frequency
  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (5330));
  Config::SetDefault ("ns3::LteUeNetDevice::DlEarfcn", UintegerValue (5330));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (23330));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (50));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (50));
  
  // Set error models
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));

  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
  
  //Set the eNBs power in dBm
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46.0));
  
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));


   //Create nodes (eNb + UEs)
  NodeContainer enbNode;
  enbNode.Create (1);
  NS_LOG_INFO ("eNb node id = [" << enbNode.Get (0)->GetId () << "]");
  std::cout << "gnb id :" << enbNode.Get(0)->GetId() << std::endl;

  NodeContainer relayUeNodes;
  relayUeNodes.Create (nRelayUes);

  NodeContainer remoteUeNodes;
  remoteUeNodes.Create (nRelayUes * nRemoteUesPerRelay);

  NodeContainer allUeNodes = NodeContainer(remoteUeNodes,relayUeNodes);

  for (uint32_t ry = 0; ry < relayUeNodes.GetN (); ry++)
    {
      NS_LOG_INFO ("Relay UE " << ry + 1 << " node id = [" << relayUeNodes.Get (ry)->GetId () << "]");
    }
  for (uint32_t rm = 0; rm < remoteUeNodes.GetN (); rm++)
    {
      NS_LOG_INFO ("Remote UE " << rm + 1 << " node id = [" << remoteUeNodes.Get (rm)->GetId () << "]");
    }
  // NodeContainer allUeNodes = NodeContainer (relayUeNodes,remoteUeNodes);

  //Position of the nodes
  //eNodeB
  Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator> ();
  positionAllocEnb->Add (Vector (0.0, 0.0, 30.0));

  //UEs
  Ptr<ListPositionAllocator> positionAllocRelays = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAllocRemotes = CreateObject<ListPositionAllocator> ();
  for (uint32_t ry = 0; ry < relayUeNodes.GetN (); ++ry)
    {
      //Relay UE
      double ry_angle = ry * (360.0 / relayUeNodes.GetN ()); //degrees
      double ry_pos_x = std::floor (relayRadius * std::cos (ry_angle * M_PI / 180.0) );
      double ry_pos_y = std::floor (relayRadius * std::sin (ry_angle * M_PI / 180.0) );

      positionAllocRelays->Add (Vector (ry_pos_x, ry_pos_y, 1.5));

      NS_LOG_INFO ("Relay UE " << ry + 1 << " node id = [" << relayUeNodes.Get (ry)->GetId () << "]"
                   " x " << ry_pos_x << " y " << ry_pos_y);
      //Remote UEs
      for (uint32_t rm = 0; rm < nRemoteUesPerRelay; ++rm)
        {
          double rm_angle = rm * (360.0 / nRemoteUesPerRelay); //degrees
          double rm_pos_x = std::floor (ry_pos_x + remoteRadius * std::cos (rm_angle * M_PI / 180.0));
          double rm_pos_y = std::floor (ry_pos_y + remoteRadius * std::sin (rm_angle * M_PI / 180.0));

          positionAllocRemotes->Add (Vector (rm_pos_x, rm_pos_y, 1.5));

          uint32_t remoteIdx = ry * nRemoteUesPerRelay + rm;
          NS_LOG_INFO ("Remote UE " << remoteIdx << " node id = [" << remoteUeNodes.Get (remoteIdx)->GetId () << "]"
                       " x " << rm_pos_x << " y " << rm_pos_y);
        }
    }


  //Install mobility
  //eNodeB
  MobilityHelper mobilityeNodeB;
  mobilityeNodeB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityeNodeB.SetPositionAllocator (positionAllocEnb);
  mobilityeNodeB.Install (enbNode);

  //Relay UEs
  MobilityHelper mobilityRelays;
  mobilityRelays.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityRelays.SetPositionAllocator (positionAllocRelays);
  mobilityRelays.Install (relayUeNodes);

  //Remote UE
  MobilityHelper mobilityRemotes;
  mobilityRemotes.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityRemotes.SetPositionAllocator (positionAllocRemotes);
  mobilityRemotes.Install (remoteUeNodes);
  
    //Install LTE devices to the nodes
  NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNode);
  NetDeviceContainer relayUeDevs = lteHelper->InstallUeDevice (relayUeNodes);
  NetDeviceContainer remoteUeDevs = lteHelper->InstallUeDevice (remoteUeNodes);
  NetDeviceContainer allUeDevs = NetDeviceContainer (relayUeDevs, remoteUeDevs);
  
  /******************************************************/
  //Create and set the EPC helper
   std::cout << "ok 1" << std::endl;
  Ptr<NoBackhaulEpcHelper> epcHelper = CreateObject<NoBackhaulEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
   std::cout << "ok 2" << std::endl;
  //core network
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
    std::cout << "ok 3" << std::endl;
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);
  std::cout << "ok 4" << std::endl;
  //Connect controller and Helper
  Config::SetDefault ("ns3::LteSlBasicUeController::ProseHelper", PointerValue (proseHelper));

  //Set pathloss model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Hybrid3gppPropagationLossModel"));

  //Enable Sidelink
  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));
    
  //connecting sidelink and gnB
  //Configure eNodeB for Sidelink
  Ptr<LteSlEnbRrc> enbSidelinkConfiguration = CreateObject<LteSlEnbRrc> ();
  enbSidelinkConfiguration->SetSlEnabled (true);
std::cout << "ok 4.5" << std::endl;
  //-Configure communication pool
  //resource setup : UE selected
  enbSidelinkConfiguration->SetDefaultPool (proseHelper->GetDefaultSlCommTxResourcesSetupUeSelected ());
  
  //-Enable discovery
  enbSidelinkConfiguration->SetDiscEnabled (true);

  //-Configure discovery pool
  enbSidelinkConfiguration->AddDiscPool (proseHelper->GetDefaultSlDiscTxResourcesSetupUeSelected ());
  LteRrcSap::SlDiscTxResourcesSetup pool;
  LteSlDiscResourcePoolFactory pDiscFactory;
  pDiscFactory.SetDiscCpLen ("NORMAL");
  pDiscFactory.SetDiscPeriod ("rf32");
  pDiscFactory.SetNumRetx (0);
  pDiscFactory.SetNumRepetition (1);
  pDiscFactory.SetDiscPrbNum (10);
  pDiscFactory.SetDiscPrbStart (10);
  pDiscFactory.SetDiscPrbEnd (40);
  pDiscFactory.SetDiscOffset (0);
  pDiscFactory.SetDiscBitmap (0x11111);
  pDiscFactory.SetDiscTxProbability ("p100");

  pool.ueSelected.poolToAddModList.pools[0].pool =  pDiscFactory.CreatePool ();



  LteRrcSap::SlCommTxResourcesSetup commpool;
  LteSlResourcePoolFactory pfactory;
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

  commpool.ueSelected.poolToAddModList.pools[0].pool =  pfactory.CreatePool ();

  //-Configure UE-to-Network Relay parameters
  //The parameters for UE-to-Network Relay (re)selection are broadcasted in the SIB19 to the UEs attached to the eNB.
  //they are required by the
  // LteUeRrc for SD-RSRP measurement and Relay Selection
  enbSidelinkConfiguration->SetDiscConfigRelay (proseHelper->GetDefaultSib19DiscConfigRelay ());

  //Install eNodeB configuration on the eNodeB devices
  lteHelper->InstallSidelinkConfiguration (enbDevs, enbSidelinkConfiguration);
 
  //Preconfigure UEs for Sidelink
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  ueSidelinkConfiguration->SetSlEnabled (true);

std::cout << "ok 4.5" << std::endl;
  //-Configure preconfiguration
  // Relay UEs: Empty configuration
  LteRrcSap::SlPreconfiguration preconfigurationRelay;
  // Remote UEs: Empty configuration if in-coverage
  //             Custom configuration (see below) if out-of-coverage
  LteRrcSap::SlPreconfiguration preconfigurationRemote;
    //Install UE configuration on the UE devices with the corresponding preconfiguration
  ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRelay);
  lteHelper->InstallSidelinkConfiguration (relayUeDevs, ueSidelinkConfiguration);

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRemote);
  lteHelper->InstallSidelinkConfiguration (remoteUeDevs, ueSidelinkConfiguration);
 std::cout << "ok 4.5" << std::endl;
  if (remoteUesOoc)
    {
      //Configure general preconfiguration parameters
      preconfigurationRemote.preconfigGeneral.carrierFreq = enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlEarfcn ();
      preconfigurationRemote.preconfigGeneral.slBandwidth = enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlBandwidth ();
      std::cout << 1 << std::endl;

      //-Configure preconfigured communication pool
      preconfigurationRemote.preconfigComm = proseHelper->GetDefaultSlPreconfigCommPoolList ();
        
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


   preconfigurationRemote.preconfigRelay = proseHelper->GetDefaultSlPreconfigRelay ();
  
      }

//NAS layer 
WifiHelper wifi;
wifi.SetStandard (WIFI_PHY_STANDARD_80211g); //2.4Ghz
wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("ErpOfdmRate54Mbps"));
 
WifiMacHelper wifiMac;
YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
YansWifiChannelHelper wifiChannel;
wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel",
                                "Frequency", DoubleValue (2.407e9)); //2.4Ghz
wifiMac.SetType ("ns3::AdhocWifiMac");
YansWifiPhyHelper phy = wifiPhy;
phy.SetChannel (wifiChannel.Create ());
WifiMacHelper mac = wifiMac;
allUeDevs = wifi.Install (phy, mac, allUeNodes);


  //Install the IP stack on the UEs and assign network IP addresses
  InternetStackHelper internet;
  internet.Install (relayUeNodes);
  internet.Install (remoteUeNodes);
  internet.Install(pgw);
  Ipv6AddressHelper ipv6h;
  ipv6h.SetBase (Ipv6Address ("6001:db80::"), Ipv6Prefix (64));
//   Ipv6InterfaceContainer internetIpIfaces = ipv6h.Assign (internetDevices);
  Ipv6InterfaceContainer internet_uedevices = ipv6h.Assign (remoteUeDevs);
  
  Ipv6InterfaceContainer ueIpIfaceRelays;
  ueIpIfaceRelays.SetForwarding (0, true);
  ueIpIfaceRelays.SetDefaultRouteInAllNodes (0);

  ueIpIfaceRelays = epcHelper->AssignUeIpv6Address (relayUeDevs);
  std::cout << "ok 54" << std::endl;
  //Set the default gateway for the UEs
  Ipv6StaticRoutingHelper Ipv6RoutingHelper;
  for (uint32_t u = 0; u < allUeNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = allUeNodes.Get (u);
      Ptr<Ipv6StaticRouting> ueStaticRouting = Ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
    }
 std::cout << "ok 6" << std::endl;
    //Configure IP for the nodes in the Internet (PGW and RemoteHost)
  // Ipv6AddressHelper ipv6h;
  ipv6h.SetBase (Ipv6Address ("6001:db80::"), Ipv6Prefix (64));
  // Ipv6InterfaceContainer internetIpIfaces = epcHelper->AssignUeIpv6Address (pgw);

  
  
  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  Ptr<Ipv6StaticRouting> relayStaticRouting = ipv6RoutingHelper.GetStaticRouting (relayUeNodes.Get(0)->GetObject<Ipv6> ());
  relayStaticRouting->AddNetworkRouteTo ("7777:f000::", Ipv6Prefix (60), ueIpIfaceRelays.GetAddress (0, 1), 1, 0);
 std::cout << "ok 7" << std::endl;

  //Configure UE-to-Network Relay network
  proseHelper->SetIpv6BaseForRelayCommunication ("7777:f00e::", Ipv6Prefix (48));
  //Attach Relay UEs to the eNB
//   lteHelper->Attach (relayUeDevs);
//   //If the Remote UEs are not OOC attach them to the eNodeB as well
//   if (!remoteUesOoc)
//     {
//       lteHelper->Attach (remoteUeDevs);
//     }
//   internetIpIfaces.SetForwarding (0, true);
//   internetIpIfaces.SetDefaultRouteInAllNodes (0);
//   Ipv6InterfaceContainer ueIpIfaceRelays;
//   Ipv6InterfaceContainer ueIpIfaceRemotes;
//   ueIpIfaceRelays = epcHelper->AssignUeIpv6Address (relayUeDevs);
//   ueIpIfaceRemotes = epcHelper->AssignUeIpv6Address (remoteUeDevs);

//   //Set the default gateway for the UEs
//   Ipv6StaticRoutingHelper Ipv6RoutingHelper;
//   for (uint32_t u = 0; u < allUeNodes.GetN (); ++u)
//     {
//       Ptr<Node> ueNode = allUeNodes.Get (u);
//       Ptr<Ipv6StaticRouting> ueStaticRouting = Ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
//       ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
//     }

    /* mcptt app**************************************/ 
    //creating mcptt application for each device 

ns3::PacketMetadata::Enable ();
McpttHelper mcpttHelper;
ApplicationContainer mcpttclientApps;
McpttTimer mcpttTimer;
uint32_t msgSize = 60; 
Time startTime = Seconds (1);
Time stopTime = Seconds (20); 
double pushTimeMean = 5.0; // seconds
double pushTimeVariance = 2.0; // seconds
double releaseTimeMean = 5.0; // seconds
double releaseTimeVariance = 2.0; // seconds
TypeId socketFacTid = UdpSocketFactory::GetTypeId ();
DataRate dataRate = DataRate ("24kb/s");
Ipv6Address PeerAddress = Ipv6Address ("6001:db80::");
Ipv4Address groupAddress4 = Ipv4Address ("255.255.255.255");


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

mcpttclientApps.Add (mcpttHelper.Install (allUeNodes));
mcpttclientApps.Start (startTime);
mcpttclientApps.Stop (stopTime);

//creating a call initiated by the press of push button 
ObjectFactory callFac;
callFac.SetTypeId ("ns3::McpttCallMachineGrpBroadcast");

ObjectFactory floorFac;
floorFac.SetTypeId ("ns3::McpttFloorMachineBasic");
  


//creating location to store application info of UEs A, B 
Ptr<McpttPttApp> ueAPttApp = DynamicCast<McpttPttApp, Application> (mcpttclientApps.Get (0));

//floor and call machines generate*******************************
ueAPttApp->CreateCall (callFac, floorFac);
ueAPttApp->SelectLastCall ();

Ipv4AddressValue grpAddr;

Simulator::Schedule (Seconds (2.2), &McpttPttApp::TakePushNotification, ueAPttApp);
uint16_t floorPort = McpttPttApp::AllocateNextPortNumber ();
uint16_t speechPort = McpttPttApp::AllocateNextPortNumber ();
Ptr<McpttCall> ueACall = ueAPttApp->GetSelectedCall ();
Simulator::Schedule (Seconds (2.15), &McpttCall::OpenFloorChan, ueACall, grpAddr.Get (), floorPort);
Simulator::Schedule (Seconds (2.15), &McpttCall::OpenFloorChan, ueACall, grpAddr.Get (), speechPort);


//   //Schedule the start of the relay service in each UE with their corresponding
//   //roles and service codes
//   for (uint32_t ryDevIdx = 0; ryDevIdx < relayUeDevs.GetN (); ryDevIdx++)
//     {

//       std::cout << "ok 1" << std::endl;

//       uint32_t serviceCode = relayUeDevs.Get (ryDevIdx)->GetObject<LteUeNetDevice> ()->GetImsi ();
//       std::cout << "ok 2" << std::endl;

//       Simulator::Schedule (Seconds (startTimeRelay [ryDevIdx]), &LteSidelinkHelper::StartRelayService, proseHelper, relayUeDevs.Get (ryDevIdx), serviceCode, LteSlUeRrc::ModelA, LteSlUeRrc::RelayUE);
//       std::cout << "ok 3" << std::endl;

//       NS_LOG_INFO ("Relay UE " << ryDevIdx << " node id = [" << relayUeNodes.Get (ryDevIdx)->GetId () << "] provides Service Code " << serviceCode << " and start service at " << startTimeRelay [ryDevIdx] << " s");
//       //Remote UEs
//       for (uint32_t rm = 0; rm < nRemoteUesPerRelay; ++rm)
//         {
//           std::cout << "ok 4" << std::endl;  
//           uint32_t rmDevIdx = ryDevIdx * nRemoteUesPerRelay + rm;
//           std::cout << "ok 5" << std::endl;
//           Simulator::Schedule ((Seconds (startTimeRemote [rmDevIdx])), &LteSidelinkHelper::StartRelayService, proseHelper, remoteUeDevs.Get (rmDevIdx), serviceCode, LteSlUeRrc::ModelA, LteSlUeRrc::RemoteUE);
//           std::cout << "ok 6" << std::endl;
//           NS_LOG_INFO ("Remote UE " << rmDevIdx << " node id = [" << remoteUeNodes.Get (rmDevIdx)->GetId () << "] interested in Service Code " << serviceCode << " and start service at " << startTimeRemote [rmDevIdx] << " s");
//         }
//     }





AsciiTraceHelper ascii;

  //Tracing PC5 signaling messages
  Ptr<OutputStreamWrapper> PC5SignalingPacketTraceStream = ascii.CreateFileStream ("PC5SignalingPacketTrace.txt");
  *PC5SignalingPacketTraceStream->GetStream () << "time(s)\ttxId\tRxId\tmsgType" << std::endl;

  for (uint32_t ueDevIdx = 0; ueDevIdx < relayUeDevs.GetN (); ueDevIdx++)
    {
      Ptr<LteUeRrc> rrc = relayUeDevs.Get (ueDevIdx)->GetObject<LteUeNetDevice> ()->GetRrc ();
      PointerValue ptrOne;
      rrc->GetAttribute ("SidelinkConfiguration", ptrOne);
      Ptr<LteSlUeRrc> slUeRrc = ptrOne.Get<LteSlUeRrc> ();
      slUeRrc->TraceConnectWithoutContext ("PC5SignalingPacketTrace",
                                           MakeBoundCallback (&TraceSinkPC5SignalingPacketTrace,
                                                              PC5SignalingPacketTraceStream));
    }
  for (uint32_t ueDevIdx = 0; ueDevIdx < remoteUeDevs.GetN (); ueDevIdx++)
    {
      Ptr<LteUeRrc> rrc = remoteUeDevs.Get (ueDevIdx)->GetObject<LteUeNetDevice> ()->GetRrc ();
      PointerValue ptrOne;
      rrc->GetAttribute ("SidelinkConfiguration", ptrOne);
      Ptr<LteSlUeRrc> slUeRrc = ptrOne.Get<LteSlUeRrc> ();
      slUeRrc->TraceConnectWithoutContext ("PC5SignalingPacketTrace",
                                           MakeBoundCallback (&TraceSinkPC5SignalingPacketTrace,
                                                              PC5SignalingPacketTraceStream));
    }


//The ns-3 LTE model currently supports the output to file of PHY, MAC, RLC and PDCP level Key Performance
// Indicators (KPIs). You can enable it in the following way:

  std::ostringstream txWithAddresses;
  txWithAddresses << "/NodeList/" << remoteUeDevs.Get (0)->GetNode ()->GetId () << "/ApplicationList/0/TxWithAddresses";

  //Config::ConnectWithoutContext (txWithAddresses.str (), MakeCallback (&SlOoc1Relay1RemoteRegularTestCase::DataPacketSinkTxNode, this));
  std::ostringstream rxWithAddresses;
  rxWithAddresses << "/NodeList/" << remoteUeDevs.Get (0)->GetNode ()->GetId () << "/ApplicationList/0/RxWithAddresses";

  //pdcp traces 
  lteHelper->EnablePdcpTraces ();
  
  //pscch
  lteHelper->EnableSlPscchMacTraces ();
  lteHelper->EnableSlPscchRxPhyTraces ();

  //pssch
  lteHelper->EnableSlPsschMacTraces ();
  
  //sidelink receiver phy layer
  lteHelper->EnableSlRxPhyTraces ();
 
   //saving animation in netanim
  AnimationInterface anim("b_lte_sl_relay_cluster_3.xml");
  std::cout << 7 << std::endl;
  anim.SetMaxPktsPerTraceFile(500000); 
  anim.EnablePacketMetadata (true);
  anim.SetConstantPosition(pgw,1.0,2.0,2.0);

  Simulator::Stop (Seconds (simTime));
  std::cout << 8 << std::endl;
  Simulator::Run ();
  std::cout << 9 << std::endl;
  Simulator::Destroy ();
  return 0;
}