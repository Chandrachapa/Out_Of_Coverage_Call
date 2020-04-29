/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * NIST-developed software is provided by NIST as a public
 * service. You may use, copy and distribute copies of the software in
 * any medium, provided that you keep intact this entire notice. You
 * may improve, modify and create derivative works of the software or
 * any portion of the software, and you may copy and distribute such
 * modifications or works. Modified works should carry a notice
 * stating that you changed the software and should note the date and
 * nature of any such change. Please explicitly acknowledge the
 * National Institute of Standards and Technology as the source of the
 * software.
 *
 * NIST-developed software is expressly provided "AS IS." NIST MAKES
 * NO WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT OR ARISING BY
 * OPERATION OF LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * NON-INFRINGEMENT AND DATA ACCURACY. NIST NEITHER REPRESENTS NOR
 * WARRANTS THAT THE OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED
 * OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE CORRECTED. NIST DOES NOT
 * WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE
 * SOFTWARE OR THE RESULTS THEREOF, INCLUDING BUT NOT LIMITED TO THE
 * CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE SOFTWARE.
 *
 * You are solely responsible for determining the appropriateness of
 * using and distributing the software and you assume all risks
 * associated with its use, including but not limited to the risks and
 * costs of program errors, compliance with applicable laws, damage to
 * or loss of data, programs or equipment, and the unavailability or
 * interruption of operation. This software is not intended to be used
 * in any situation where a failure could cause risk of injury or
 * damage to property. The software developed by NIST employees is not
 * subject to copyright protection within the United States.
 */


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
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestSidelinkOutOfCoverageComm");
/*
 * \ingroup lte-test
 * \ingroup tests
 *
 * \brief Sidelink out of coverage communication test case.
 
class SidelinkOutOfCoverageCommTestCase : public TestCase
{
public:
  
   * Constructor
   
  SidelinkOutOfCoverageCommTestCase ();
  virtual ~SidelinkOutOfCoverageCommTestCase ();

private:
  virtual void DoRun (void);
  
   * \brief Sink Rx function
   *
   * \param p The packet
   * \param add Address
   
  void SinkRxNode (Ptr<const Packet> p, const Address &add);
  uint32_t m_numPacketRx; ///< Total number of Rx packets
};

SidelinkOutOfCoverageCommTestCase::SidelinkOutOfCoverageCommTestCase ()
  : TestCase ("Scenario with 2 out of coverage UEs performing Sidelink communication"),
    m_numPacketRx (0)
{
}

SidelinkOutOfCoverageCommTestCase::~SidelinkOutOfCoverageCommTestCase ()
{
}

void
SidelinkOutOfCoverageCommTestCase::SinkRxNode (Ptr<const Packet> p, const Address &add)
{
  NS_LOG_INFO ("Node received " << m_numPacketRx << " packets");
  m_numPacketRx++;
}
*/


static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}



int main (int argc, char *argv[]){
 /**
  * Create a scenario with two UEs, which are out of coverage.
  * One of the UE will send traffic directly to the other UE via Sidelink
  * The resource scheduling is performed by the transmitting UE, i.e.,
  * MODE 2. Default configuration will send 10 packets per second for 2 seconds.
  * The expected output is that the receiver UE node would receive 20 packets.
  */
  /*
   LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);
   LogComponentEnable ("LteUePhy", logLevel);
   LogComponentEnable ("LteUeRrc", logLevel);
   LogComponentEnable ("LteUeMac", logLevel);
   LogComponentEnable ("LteSlPool", logLevel);
  */
  //Initialize simulation time
    Time startTime = Seconds (1);
    Time simulationTime = Seconds (25);
    Time stopTime = simulationTime;


  double simTime = 25;
  //uint32_t groupL2Address = 0xFF;

   
  // Configure the UE for UE_SELECTED scenarios
  Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1));
  Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (16));
  Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (5)); //The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (true)); //use default Trp index of 0

  //Set the frequency to use the Public Safety case (band 14 : 700 MHz)
  uint32_t ulEarfcn = 23330;
  uint16_t ulBandwidth = 50;

  //Set the UEs power in dBm
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));

  // Set error models
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (false));
  
    //Sidelink bearers activation time
  //Time slBearersActivationTime = startTime;
  //Create the helpers
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  //ProSe
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  //Set pathloss model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Cost231PropagationLossModel"));

  //Enable Sidelink
  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));
  lteHelper->DisableEnbPhy (true);
  // channel model initialization
  lteHelper->Initialize ();

  // Since we are not installing eNB, we need to set the frequency attribute of pathloss model here
  double ulFreq = LteSpectrumValueHelper::GetCarrierFrequency (ulEarfcn);
  NS_LOG_LOGIC ("UL freq: " << ulFreq);
  Ptr<Object> uplinkPathlossModel = lteHelper->GetUplinkPathlossModel ();
  Ptr<PropagationLossModel> lossModel = uplinkPathlossModel->GetObject<PropagationLossModel> ();
  NS_ABORT_MSG_IF (lossModel == nullptr, "No PathLossModel");
  bool ulFreqOk = uplinkPathlossModel->SetAttributeFailSafe ("Frequency", DoubleValue (ulFreq));
  if (!ulFreqOk)
    {
      NS_LOG_WARN ("UL propagation model does not have a Frequency attribute");
    }

  NS_LOG_INFO ("Deploying UE's...");

  // Create UE nodes
  NodeContainer ueNodes;
  ueNodes.Create (2);
  NS_LOG_INFO ("Ue 0 node id = " << ueNodes.Get (0)->GetId ());
  NS_LOG_INFO ("Ue 1 node id = " << ueNodes.Get (1)->GetId ());

  //Position of the nodes
  Ptr<ListPositionAllocator> positionAllocUe1 = CreateObject<ListPositionAllocator> ();
  positionAllocUe1->Add (Vector (100, 10, 1.5));
  Ptr<ListPositionAllocator> positionAllocUe2 = CreateObject<ListPositionAllocator> ();
  positionAllocUe2->Add (Vector (100, -10, 1.5));

  MobilityHelper mobilityUe1;
  mobilityUe1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUe1.SetPositionAllocator (positionAllocUe1);
  mobilityUe1.Install (ueNodes.Get (0));

  MobilityHelper mobilityUe2;
  mobilityUe2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUe2.SetPositionAllocator (positionAllocUe2);
  mobilityUe2.Install (ueNodes.Get (1));

  //Install LTE devices to the nodes
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);

  //Sidelink pre-configuration for the UEs
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  ueSidelinkConfiguration->SetSlEnabled (true);

  LteRrcSap::SlPreconfiguration preconfiguration;

  preconfiguration.preconfigGeneral.carrierFreq = ulEarfcn;
  preconfiguration.preconfigGeneral.slBandwidth = ulBandwidth;
  preconfiguration.preconfigComm.nbPools = 1;

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

  preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();
  pfactory.SetHaveUeSelectedResourceConfig (true);
  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
  lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);
  
  //network layer configurations 
  //installing wifi interface in nodes
  /*
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211g); //2.4Ghz
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("ErpOfdmRate54Mbps"));
  std::cout << "okay 18" << "\n" ;

  WifiMacHelper wifiMac;
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel",
                                "Frequency", DoubleValue (2.407e9)); //2.4Ghz

  wifiMac.SetType ("ns3::AdhocWifiMac");
  std::cout << "okay 19" << "\n" ;

  YansWifiPhyHelper phy = wifiPhy;
  phy.SetChannel (wifiChannel.Create ());
  std::cout << "okay 20" << "\n" ;

  WifiMacHelper mac = wifiMac;
  ueDevs = wifi.Install (phy, mac, ueNodes);
  std::cout << "okay 21" << "\n" ;
  */ 
  //Install the IP stack on the UEs
  InternetStackHelper internet;
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

  //Assign IP address to UEs, and install applications
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  ///*** Configure applications ***///
  

//mcptt application
//configurations 
//variable declarations for using push to talk 
//uint32_t appCount;
//uint32_t groupcount = 1;
//uint32_t usersPerGroup = 5;
DataRate dataRate = DataRate ("24kb/s");
uint32_t msgSize = 60; //60 + RTP header = 60 + 12 = 72
//double maxX = 5.0;
//double maxY = 5.0;
double pushTimeMean = 1.0; // seconds
double pushTimeVariance = 1.0; // seconds
double releaseTimeMean = 1.0; // seconds
double releaseTimeVariance = 1.0; // seconds

TypeId socketFacTid = UdpSocketFactory::GetTypeId ();
std::vector<uint32_t> groupL2Addresses;
uint32_t groupL2Address = 255;
Ipv4AddressGenerator::Init (Ipv4Address ("225.0.0.0"), Ipv4Mask ("255.0.0.0"));
Ipv4Address groupAddress = Ipv4AddressGenerator::NextAddress (Ipv4Mask ("255.0.0.0"));
//Ipv4Address groupAddress = Ipv4Address ("10.255.255.255"); //use multicast address as destination  
  
  ApplicationContainer clientApps;
  McpttHelper mcpttClient;
  McpttTimer mcpttTimer;
  
  //creating mcptt service on each node
  clientApps.Add (mcpttClient.Install (ueNodes));

  mcpttClient.SetPttApp ("ns3::McpttPttApp",
                         "PeerAddress", Ipv4AddressValue (groupAddress),
                         "PushOnStart", BooleanValue (true));
  mcpttClient.SetMediaSrc ("ns3::McpttMediaSrc",
                         "Bytes", UintegerValue (msgSize),
                         "DataRate", DataRateValue (dataRate));
  mcpttClient.SetPusher ("ns3::McpttPusher",
                         "Automatic", BooleanValue (true));
  mcpttClient.SetPusherPushVariable ("ns3::NormalRandomVariable",
                         "Mean", DoubleValue (pushTimeMean),
                         "Variance", DoubleValue (pushTimeVariance));
  mcpttClient.SetPusherReleaseVariable ("ns3::NormalRandomVariable",
                         "Mean", DoubleValue (releaseTimeMean),
                         "Variance", DoubleValue (releaseTimeVariance));

  clientApps.Start (Seconds(1));
  clientApps.Stop (stopTime);

//creating floor and call interfaces for  call

    ObjectFactory callFac;
    callFac.SetTypeId (McpttCallMachineGrpBasic::GetTypeId ());
    callFac.Set ("GroupId", UintegerValue (1));
    ObjectFactory floorFac;
    floorFac.SetTypeId (McpttFloorMachineBasic::GetTypeId ());
    for (uint32_t idx = 0; idx < clientApps.GetN (); idx++)
    {
    Ptr<McpttPttApp> pttApp =
    DynamicCast<McpttPttApp, Application> (clientApps.Get (idx));
    //assuming floor is granted a group call is created for all the N users in the group
    pttApp->CreateCall (callFac, floorFac);
    //select the call of the first user 
    pttApp->SelectCall (0);
    if (idx == 0)
    {//ptt button press for first user
    Simulator::Schedule (Seconds (1.0), &McpttPttApp::TakePushNotification, pttApp);
    }
    }
  //Ptr<McpttCall> ueACall = PttApp->GetSelectedCall ();
  Ptr<McpttPttApp> ueAPttApp =  DynamicCast<McpttPttApp, Application> (clientApps.Get (0));
  
  Ptr<McpttCall> ueACall = ueAPttApp->GetSelectedCall ();
  
  McpttCallMsg msg;
  uint16_t floorPort = McpttPttApp::AllocateNextPortNumber ();
  uint16_t speechPort = McpttPttApp::AllocateNextPortNumber ();
  /*
  //set a delay of delaytfb1
  //Simulator::Schedule (Seconds (5), &McpttTimer::Start,rawTimer);
   */
  Ipv4AddressValue grpAddress;
  Simulator::Schedule (Seconds (5.15), &McpttCall::OpenFloorChan, ueACall, grpAddress.Get (), floorPort);
  Simulator::Schedule (Seconds (6), &McpttCall::OpenMediaChan, ueACall, grpAddress.Get (), speechPort);
  

  //Set Application in the UEs
  //Ipv4Address groupAddress ("225.0.0.0"); //use multicast address as destination
  //UdpClientHelper udpClient (groupAddress, 8000);
  //udpClient.SetAttribute ("MaxPackets", UintegerValue (500));
  //udpClient.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
  //udpClient.SetAttribute ("PacketSize", UintegerValue (280));

  //ApplicationContainer clientApps = udpClient.Install (ueNodes.Get (0));
  //clientApps.Get (0)->SetStartTime (Seconds (3.0));
  //clientApps.Stop (Seconds (5.0));

  //ApplicationContainer serverApps;
  //PacketSinkHelper sidelinkSink ("ns3::UdpSocketFactory",Address (InetSocketAddress (Ipv4Address::GetAny (), 8000)));
  //serverApps = sidelinkSink.Install (ueNodes.Get (1));
  //serverApps.Get (0)->SetStartTime (Seconds (3.0));
  
  //Trace receptions; use the following to be robust to node ID changes
  std::ostringstream oss;
  oss << "/NodeList/" << ueNodes.Get (1)->GetId () << "/ApplicationList/0/$ns3::PacketSink/Rx";
  //Config::ConnectWithoutContext (oss.str (), MakeCallback (&SidelinkOutOfCoverageCommTestCase::SinkRxNode,this));

  //Set Sidelink bearers
  Ptr<LteSlTft> tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupAddress, groupL2Address);
  proseHelper->ActivateSidelinkBearer (Seconds (2.0), ueDevs, tft);
  NoBackhaulEpcHelper nobackhaulHelper;
  Ptr<NetDevice> uedevice;
  //nobackhaulHelper.ActivateSidelinkBearer (uedevice,tft);
  ///*** End of application configuration ***///
  
  //results
  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("b5.pcap", std::ios::out, PcapHelper::DLT_PPP);
  std::cout << "okay 56" << "\n" ;
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("b5.cwnd");
  ueDevs.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));
  std::cout << "okay 57" << "\n" ;

  //Enable traces
  lteHelper->EnableTraces ();
  lteHelper->EnableSlPscchMacTraces ();
  lteHelper->EnableSlPsschMacTraces ();
  std::cout << "okay 59" << "\n" ;
  lteHelper->EnableSlRxPhyTraces ();
  lteHelper->EnableSlPscchRxPhyTraces ();
  std::cout << "okay 60" << "\n" ;

  //receiver : reception of floor request 
  NS_LOG_INFO ("Enabling MCPTT traces...");
  mcpttClient.EnableMsgTraces ();
  mcpttClient.EnableStateMachineTraces ();

  AnimationInterface anim("sl_OoC.xml");
  anim.SetMaxPktsPerTraceFile(500000); 
  
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
  std::cout << "done simulator" << "\n";
  //NS_TEST_ASSERT_MSG_EQ (m_numPacketRx, 20, "20 packets should be received at the receiver!");
}

/*
 * \ingroup lte-test
 * \ingroup tests
 *
 * \brief Sidelink out of coverage communication test suite.

class SidelinkOutOfCoverageCommTestSuite : public TestSuite
{
public:
  SidelinkOutOfCoverageCommTestSuite ();
};

SidelinkOutOfCoverageCommTestSuite::SidelinkOutOfCoverageCommTestSuite ()
  : TestSuite ("sidelink-out-of-coverage-comm", SYSTEM)
{
  // LogComponentEnable ("TestSidelinkOutOfCoverageComm", LOG_LEVEL_ALL);

  //Test 1
  AddTestCase (new SidelinkOutOfCoverageCommTestCase (), TestCase::QUICK);
}

static SidelinkOutOfCoverageCommTestSuite staticSidelinkOutOfCoverageCommTestSuite;
 */
