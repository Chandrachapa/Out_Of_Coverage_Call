  
#include <sstream>
#include "ns3/object.h"
#include "ns3/test.h"
#include "ns3/lte-helper.h"
#include "ns3/lte-sidelink-helper.h"
#include "ns3/lte-sl-preconfig-pool-factory.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/application-container.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/internet-module.h"
//added 
#include <ns3/mcptt-call-machine-grp-broadcast.h> 
#include <ns3/mcptt-call-machine-grp-basic.h>
#include <ns3/mcptt-floor-machine-basic.h>
#include <ns3/mcptt-helper.h>
#include <ns3/udp-socket-factory.h>
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-module.h"
#include <ns3/mcptt-call.h>
#include <ns3/ipv4-address-generator.h>
#include <ns3/udp-echo-helper.h>
#include <ns3/udp-echo-server.h>
#include <ns3/udp-echo-client.h>
#include <ns3/netanim-module.h>
#include <ns3/ipv6-interface-address.h>
#include <ns3/ipv6-interface.h>

using namespace ns3;
// class SlOoc1Relay1RemoteRegularTestCase : public TestCase
// {
// public:
//   /**
//    * Constructor
//    *
//    * \param simTime simTime
//    * \param echoClientMaxPackets echoClientMaxPackets
//    * \param echoClientInterval echoClientInterval
//    * \param echoClientPacketSize echoClientPacketSize
//    */
//   SlOoc1Relay1RemoteRegularTestCase (double simTime, double echoClientMaxPackets, double echoClientInterval, double echoClientPacketSize);
//   virtual ~SlOoc1Relay1RemoteRegularTestCase ();
//   /**
//    * DataPacketSinkTxNode
//    *
//    * \param p Packet
//    * \param src src
//    * \param dst dst
//    */
//   void DataPacketSinkTxNode (Ptr<const Packet> p, const Address &src, const Address &dst);
//   /**
//    * DataPacketSinkRxNode
//    *
//    * \param p Packet
//    * \param src src
//    * \param dst dst
//    */
//   void DataPacketSinkRxNode (Ptr<const Packet> p, const Address &src, const Address &dst);
//   /**
//    * PC5SignalingPacketTraceFunction
//    *
//    * \param srcL2Id srcL2Id
//    * \param dstL2Id dstL2Id
//    * \param p Packet
//    */
//   void PC5SignalingPacketTraceFunction (uint32_t srcL2Id, uint32_t dstL2Id, Ptr<Packet> p);

// private:
//   virtual void DoRun (void);
//   double m_simTime; ///< simTime
//   double m_echoClientMaxPackets; ///< echoClientMaxPackets
//   double m_echoClientInterval; ///< echoClientInterval
//   double m_echoClientPacketSize; ///< echoClientPacketSize
//   bool m_recvRUIRq; ///< recvRUIRq
//   bool m_recvRUIRs; ///< recvRUIRs
//   uint32_t m_srcL2IdToCompareLater; ///< srcL2IdToCompareLater
//   uint32_t m_dstL2IdToCompareLater; ///< dstL2IdToCompareLater
//   bool m_txPacket; ///< txPacket
//   bool m_rxPacket; ///< rxPacket
//   Ipv6Address m_srcAddressToCompareLater; ///< srcAddressToCompareLater
//   Ipv6Address m_dstAddressToCompareLater; ///< dstAddressToCompareLater
// };

// SlOoc1Relay1RemoteRegularTestCase::SlOoc1Relay1RemoteRegularTestCase (double simTime, double echoClientMaxPackets, double echoClientInterval, double echoClientPacketSize)
//   : TestCase ("Scenario with 1 eNodeB and 2 UEs using SideLink"),
//   m_simTime (simTime),
//   m_echoClientMaxPackets (echoClientMaxPackets),
//   m_echoClientInterval (echoClientInterval),
//   m_echoClientPacketSize (echoClientPacketSize),
//   m_recvRUIRq (false),
//   m_recvRUIRs (false),
//   m_srcL2IdToCompareLater (0),
//   m_dstL2IdToCompareLater (0),
//   m_txPacket (false),
//   m_rxPacket (false)
// {
//   m_srcAddressToCompareLater = Ipv6Address::GetOnes ();
//   m_dstAddressToCompareLater = Ipv6Address::GetOnes ();
// }

// SlOoc1Relay1RemoteRegularTestCase::~SlOoc1Relay1RemoteRegularTestCase ()
// {
// }


// void
// SlOoc1Relay1RemoteRegularTestCase::DataPacketSinkTxNode (Ptr<const Packet> p, const Address &src, const Address &dst)
// {
//   m_srcAddressToCompareLater = Inet6SocketAddress::ConvertFrom (src).GetIpv6 ();
//   m_dstAddressToCompareLater = Inet6SocketAddress::ConvertFrom (dst).GetIpv6 ();
//   m_txPacket = true;
// }

// void
// SlOoc1Relay1RemoteRegularTestCase::DataPacketSinkRxNode (Ptr<const Packet> p, const Address &src, const Address &dst)
// {
//   if (Inet6SocketAddress::ConvertFrom (src).GetIpv6 () == m_dstAddressToCompareLater && Inet6SocketAddress::ConvertFrom (dst).GetIpv6 () == m_srcAddressToCompareLater)
//     {
//       m_rxPacket = true;
//     }
// }

// void
// SlOoc1Relay1RemoteRegularTestCase::PC5SignalingPacketTraceFunction (uint32_t srcL2Id, uint32_t dstL2Id, Ptr<Packet> p)
// {
//   LteSlPc5SignallingMessageType lpc5smt;
//   p->PeekHeader (lpc5smt);

//   if (lpc5smt.GetMessageType () == LteSlPc5SignallingMessageType::RemoteUeInfoRequest)
//     {
//       m_srcL2IdToCompareLater = srcL2Id;
//       m_dstL2IdToCompareLater = dstL2Id;
//       m_recvRUIRq = true;
//     }

//   if (lpc5smt.GetMessageType () == LteSlPc5SignallingMessageType::RemoteUeInfoResponse)
//     {
//       if ((srcL2Id == m_dstL2IdToCompareLater) && (dstL2Id == m_srcL2IdToCompareLater))
//         {
//           m_recvRUIRs = true;
//         }
//     }

// }


int main (int argc, char *argv[])
{
  
  Time simTime = Seconds (20);
  bool useRelay = true;
  double relayUeInitXPos = 300.0;
  double remoteUeInitXPos = 350.0;

  //Configure One-To-One Communication timers and counters between Relay and Remote UE
  Config::SetDefault ("ns3::LteSlO2oCommParams::RelayT4108", UintegerValue (10000));
  Config::SetDefault ("ns3::LteSlO2oCommParams::RemoteT4100", UintegerValue (10000));
  Config::SetDefault ("ns3::LteSlO2oCommParams::RemoteT4101", UintegerValue (50000));
  Config::SetDefault ("ns3::LteSlO2oCommParams::RemoteT4102", UintegerValue (1000));

  //Enable the Remote UE information request procedure
  Config::SetDefault ("ns3::LteSlUeRrc::RuirqEnabled",  BooleanValue (true));

  //Configure the UE for UE_SELECTED scenario
  Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (16));
  Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (6)); //The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1));
  Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (false)); //use default Trp index of 0

  //Set the frequency
  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (5330));
  Config::SetDefault ("ns3::LteUeNetDevice::DlEarfcn", UintegerValue (5330));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (23330));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (50));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (50));

  // Set error models
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlFullDuplexEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::LteUePhy::DownlinkCqiPeriodicity", TimeValue (MilliSeconds (79)));

  //Set the UEs power in dBm
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));
  //Set the eNBs power in dBm
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46.0));

  //Sidelink bearers activation time
  Time slBearersActivationTime = Seconds (2.0);

  //Create and set the helpers
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);
  Config::SetDefault ("ns3::LteSlBasicUeController::ProseHelper", PointerValue (proseHelper));

  //Set pathloss model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Hybrid3gppPropagationLossModel"));

  //Enable Sidelink
  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);
  

  //Create nodes (eNb + UEs)
  NodeContainer enbNode;
  enbNode.Create (1);
  // NS_LOG_INFO ("eNb node id = [" << enbNode.Get (0)->GetId () << "]");
  NodeContainer ueNodes;
  ueNodes.Create (2);
 // NS_LOG_INFO ("UE 1 node id = [" << ueNodes.Get (0)->GetId () << "]");
 // NS_LOG_INFO ("UE 2 node id = [" << ueNodes.Get (1)->GetId () << "]");

  Ptr<Node> remoteUE = ueNodes.Get(0);
  Ptr<Node> relayUE = ueNodes.Get(1);
  PointToPointHelper p2p_;
  p2p_.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2p_.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2p_.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices_ = p2p_.Install (remoteUE, relayUE);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);// p2p is installed between  3 and 0 nodes 
  std::cout << pgw->GetId () << std::endl;

  //Position of the nodes
  //eNodeB
  Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator> ();
  positionAllocEnb->Add (Vector (0.0, 0.0, 30.0));
  //Relay UE
  Ptr<ListPositionAllocator> positionAllocUe1 = CreateObject<ListPositionAllocator> ();
  positionAllocUe1->Add (Vector (relayUeInitXPos, 0.0, 1.5));
  //Remote UE
  Ptr<ListPositionAllocator> positionAllocUe2 = CreateObject<ListPositionAllocator> ();
  positionAllocUe2->Add (Vector (remoteUeInitXPos, 0.0, 1.5));

  //Install mobility
  //eNodeB
  MobilityHelper mobilityeNodeB;
  mobilityeNodeB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityeNodeB.SetPositionAllocator (positionAllocEnb);
  mobilityeNodeB.Install (enbNode);

  //Relay UE
  MobilityHelper mobilityUe1;
  mobilityUe1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUe1.SetPositionAllocator (positionAllocUe1);
  mobilityUe1.Install (ueNodes.Get (0));

  //Remote UE
  MobilityHelper mobilityUe2;
  mobilityUe2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUe2.SetPositionAllocator (positionAllocUe2);
  mobilityUe2.Install (ueNodes.Get (1));

  //Install LTE devices to the nodes
  NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNode);
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);

  
  std::cout << enbDevs.Get (0)->GetNode ()->GetId () << std::endl;
  std::cout << ueDevs.GetN() << std::endl;

  //Configure Sidelink
  Ptr<LteSlEnbRrc> enbSidelinkConfiguration = CreateObject<LteSlEnbRrc> ();
  enbSidelinkConfiguration->SetSlEnabled (true);

  //Configure communication pool
  LteRrcSap::SlCommTxResourcesSetup pool;

  pool.setup = LteRrcSap::SlCommTxResourcesSetup::UE_SELECTED;
  pool.ueSelected.havePoolToRelease = false;
  pool.ueSelected.havePoolToAdd = true;
  pool.ueSelected.poolToAddModList.nbPools = 1;
  pool.ueSelected.poolToAddModList.pools[0].poolIdentity = 1;

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

  pool.ueSelected.poolToAddModList.pools[0].pool =  pfactory.CreatePool ();

  uint32_t groupL2Address = 255;
  enbSidelinkConfiguration->AddPreconfiguredDedicatedPool (groupL2Address, pool);
  enbSidelinkConfiguration->AddPreconfiguredDedicatedPool (1, pool); //update to use L2ID for remote UE
  enbSidelinkConfiguration->AddPreconfiguredDedicatedPool (2, pool); //update to use L2ID for remote UE

  //Configure default communication pool **
  enbSidelinkConfiguration->SetDefaultPool (pool);

  //Configure discovery pool
  enbSidelinkConfiguration->SetDiscEnabled (true);

  LteRrcSap::SlDiscTxResourcesSetup discPool;
  discPool.setup =  LteRrcSap::SlDiscTxResourcesSetup::UE_SELECTED;
  discPool.ueSelected.havePoolToRelease = false;
  discPool.ueSelected.havePoolToAdd = true;
  discPool.ueSelected.poolToAddModList.nbPools = 1;
  discPool.ueSelected.poolToAddModList.pools[0].poolIdentity = 1;

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

  discPool.ueSelected.poolToAddModList.pools[0].pool =  pDiscFactory.CreatePool ();

  enbSidelinkConfiguration->AddDiscPool (discPool);

  //Install Sidelink configuration for eNBs
  lteHelper->InstallSidelinkConfiguration (enbDevs, enbSidelinkConfiguration);

  //Configure Sidelink Preconfiguration for the UEs
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  ueSidelinkConfiguration->SetSlEnabled (true);
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

  ueSidelinkConfiguration->SetDiscEnabled (true);
  uint8_t nb = 3;
  ueSidelinkConfiguration->SetDiscTxResources (nb);
  ueSidelinkConfiguration->SetDiscInterFreq (enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlEarfcn ());

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRelay);
  lteHelper->InstallSidelinkConfiguration (ueDevs.Get (0), ueSidelinkConfiguration);

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRemote);
  lteHelper->InstallSidelinkConfiguration (ueDevs.Get (1), ueSidelinkConfiguration);


  //Install the IP stack on the UEs and assign IP address
  internet.Install (ueNodes);
  Ipv6InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv6Address (NetDeviceContainer (ueDevs));

  //Define and set routing
  Ipv6StaticRoutingHelper Ipv6RoutingHelper;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv6StaticRouting> ueStaticRouting = Ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
    }
  ueIpIface.SetForwarding (0, true);

  Ipv6AddressHelper ipv6h;
  ipv6h.SetBase (Ipv6Address ("6001:db80::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer internetIpIfaces = ipv6h.Assign (internetDevices);
  Ipv6InterfaceContainer internet_uedevices = ipv6h.Assign (ueDevs);
  internetIpIfaces.SetForwarding (0, true);
  internetIpIfaces.SetDefaultRouteInAllNodes (0);

  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  Ptr<Ipv6StaticRouting> remoteHostStaticRouting = ipv6RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv6> ());
  remoteHostStaticRouting->AddNetworkRouteTo ("7777:f000::", Ipv6Prefix (60), internetIpIfaces.GetAddress (0, 1), 1, 0);

  Ptr<Ipv6StaticRouting> pgwStaticRouting = ipv6RoutingHelper.GetStaticRouting (pgw->GetObject<Ipv6> ());
  pgwStaticRouting->AddNetworkRouteTo ("7777:f00e:0:0::", Ipv6Prefix (60), Ipv6Address ("::"), 1, 0);

  //Attach the relay UE to the eNodeB
  lteHelper->Attach (ueDevs.Get (0));
  //  lteHelper->Attach (ueDevs.Get (1));
  //If not using relay, attach the remote to the eNodeB as well
  // if (!useRelay)
  //   {
  //     lteHelper->Attach (ueDevs.Get (1));
  //   }

  // interface 0 is localhost, 1 is the p2p device
  Ipv6Address pgwAddr = internetIpIfaces.GetAddress (0,1);
  Ipv6Address remoteHostAddr = internetIpIfaces.GetAddress (1,1);
  Ipv6Address remoteUeaddress =  internet_uedevices.GetAddress(1,1);
  Ipv6Address relayUeaddress =  internet_uedevices.GetAddress(0,1);
  std::cout << " pgw address : " << pgwAddr << std::endl;
  std::cout << "remote host address : " << remoteHostAddr << std::endl;
  std::cout << "remote ue address : " << remoteUeaddress << std::endl;
  std::cout << "relay ue address : " << relayUeaddress << std::endl;

  uint16_t echoPort = 8000;
  
  uint16_t echoPort2 = 9000;
  //Set echo server in the remote host

  UdpEchoServerHelper echoServer (echoPort);
  ApplicationContainer serverApps = echoServer.Install (ueNodes);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (simTime);

  std::cout << remoteHost->GetId () << std::endl;

  //Set echo client in the UEs
  UdpEchoClientHelper echoClient (relayUeaddress, echoPort);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (20));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (200));

  ApplicationContainer clientApps;
  clientApps = echoClient.Install (ueNodes.Get (1)); //Only the remote UE will have traffic
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (simTime);


  //Set echo client in the UEs
  UdpEchoClientHelper echoClient2 (remoteUeaddress, echoPort2);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (20));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (200));

  ApplicationContainer clientApps_;
  clientApps_ = echoClient2.Install (ueNodes.Get (0)); //Only the remote UE will have traffic
  clientApps_.Start (Seconds (2.0));
  clientApps_.Stop (simTime);

//NS_LOG_INFO ("Creating applications...");

//creating mcptt application for each device 
McpttHelper mcpttHelper;
ApplicationContainer mcpttclientApps;
McpttTimer mcpttTimer;

Time startTime = Seconds (1);
Time stopTime = Seconds (30); 
double pushTimeMean = 5.0; // seconds
double pushTimeVariance = 2.0; // seconds
double releaseTimeMean = 5.0; // seconds
double releaseTimeVariance = 2.0; // seconds
TypeId socketFacTid = UdpSocketFactory::GetTypeId ();
DataRate dataRate = DataRate ("24kb/s");
Ipv6Address PeerAddress = Ipv6Address ("6001:db80::");
Ipv4Address groupAddress4 = Ipv4Address ("255.255.255.255");
uint32_t msgSize = 60; //60 + RTP header = 60 + 12 = 72
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

mcpttclientApps.Add (mcpttHelper.Install (ueNodes));
mcpttclientApps.Start (startTime);
mcpttclientApps.Stop (stopTime);

ObjectFactory callFac;
callFac.SetTypeId ("ns3::McpttCallMachineGrpBroadcast");

ObjectFactory floorFac;
floorFac.SetTypeId ("ns3::McpttFloorMachineBasic");
  
Ipv4AddressValue grpAddr;

//creating location to store application info of UEs A, B 
Ptr<McpttPttApp> ueAPttApp = DynamicCast<McpttPttApp, Application> (mcpttclientApps.Get (0));
// Ptr<McpttPttApp> ueBPttApp = DynamicCast<McpttPttApp, Application> (clientApps.Get (1));
// Ptr<McpttPttApp> ueCPttApp = DynamicCast<McpttPttApp, Application> (clientApps.Get (2));

//   //UE A
//   uint32_t grpId = 1;
//   uint16_t callId = 1;
  
// //McpttCallMsgFieldUserLoc m_userLoc;

//   //UE B 
//   std::string orgName = "EMS";
//   Time joinTime = Seconds (2.2);
//   uint32_t origId = ueAPttApp->GetUserId ();
  int no_devices = ueAPttApp->GetNode()->GetNDevices();
for (int i=0;i<no_devices;i++){

  std::cout << "A address" << ueAPttApp->GetNode()->GetDevice(i)->GetAddress() << std::endl;

}

  //fix this // device ekka id within the range 
  // 0.0.0.0

  // McpttCallMsgFieldSdp sdp;
  // sdp.SetFloorPort (floorPort);
  // sdp.SetGrpAddr (grpAddress.Get ());
  // //sdp.SetOrigAddr (origAddress);
  // sdp.SetSpeechPort (speechPort);

 
//floor and call machines generate*******************************
ueAPttApp->CreateCall (callFac, floorFac);
ueAPttApp->SelectLastCall ();
// ueBPttApp->CreateCall (callFac, floorFac);
// ueBPttApp->SelectLastCall ();
// ueCPttApp->CreateCall (callFac, floorFac);
// ueCPttApp->SelectLastCall ();

 Simulator::Schedule (Seconds (2.2), &McpttPttApp::TakePushNotification, ueAPttApp);
  ///*** End of application configuration ***///


  ///*** Configure Relaying ***///
  if (useRelay)
    {
      Ptr<EpcTft> tft = Create<EpcTft> ();
      EpcTft::PacketFilter dlpf;
      dlpf.localIpv6Address.Set ("7777:f00e::");
      dlpf.localIpv6Prefix = Ipv6Prefix (64);
      tft->Add (dlpf);
      EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
      lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (0), bearer, tft);

      Simulator::Schedule (Seconds (2.0), &LteSidelinkHelper::StartRelayService, proseHelper, ueDevs.Get (0), 33, LteSlUeRrc::ModelA, LteSlUeRrc::RelayUE);
      Simulator::Schedule (Seconds (4.0), &LteSidelinkHelper::StartRelayService, proseHelper, ueDevs.Get (1), 33, LteSlUeRrc::ModelA, LteSlUeRrc::RemoteUE);
    }


std::cout << ueDevs.Get (0)->GetNode ()->GetId () << std::endl;


std::cout << ueDevs.Get (1)->GetNode ()->GetId () << std::endl;


  //Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/LteUeRrc/SidelinkConfiguration/PC5SignalingPacketTrace", MakeCallback (&SlOoc1Relay1RemoteRegularTestCase::PC5SignalingPacketTraceFunction, this));

  std::ostringstream txWithAddresses;
  txWithAddresses << "/NodeList/" << ueDevs.Get (1)->GetNode ()->GetId () << "/ApplicationList/0/TxWithAddresses";
  //Config::ConnectWithoutContext (txWithAddresses.str (), MakeCallback (&SlOoc1Relay1RemoteRegularTestCase::DataPacketSinkTxNode, this));

  std::ostringstream rxWithAddresses;
  rxWithAddresses << "/NodeList/" << ueDevs.Get (1)->GetNode ()->GetId () << "/ApplicationList/0/RxWithAddresses";
  //Config::ConnectWithoutContext (rxWithAddresses.str (), MakeCallback (&SlOoc1Relay1RemoteRegularTestCase::DataPacketSinkRxNode, this));

 // NS_LOG_INFO ("Starting simulation...");

  Simulator::Stop (simTime);
  AnimationInterface anim("b21.xml");
  anim.SetMaxPktsPerTraceFile(500000);
  Simulator::Run ();
  Simulator::Destroy ();

  //NS_TEST_ASSERT_MSG_EQ ((m_recvRUIRq && m_recvRUIRs), true, "Error in sending and receiving RemoteUeInfoRequest and RemoteUeInfoResponse. Test regular scenario failed");
  //NS_TEST_ASSERT_MSG_EQ ((m_txPacket && m_rxPacket), true, "Error in sending and receiving (echoing) of atleast one packet at the Remote UE. Test regular scenario failed");
}