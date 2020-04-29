//header files 
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

#include <ns3/callback.h>
#include <ns3/mcptt-call-machine-grp-broadcast.h>
#include <ns3/mcptt-call-machine-grp-broadcast-state.h>
#include <ns3/mcptt-call-msg.h>
#include <ns3/mcptt-ptt-app.h>
#include <ns3/mcptt-timer.h>
#include <iostream>

#include "ns3/lte-module.h"
#include <cfloat>
#include <sstream>
#include <ns3/object.h>
#include <ns3/packet.h>
#include <ns3/ptr.h>
#include <ns3/type-id.h>
#include <ns3/mcptt-call-machine.h>
#include <ns3/mcptt-call-msg.h>
#include <ns3/mcptt-chan.h>
#include <ns3/mcptt-floor-machine.h>
#include <ns3/mcptt-floor-msg.h>
#include <ns3/mcptt-media-msg.h>
#include <ns3/mcptt-helper.h>
#include <ns3/lte-helper.h>
#include <ns3/simple-net-device.h>
#include <fstream>
#include <ns3/lte-sl-preconfig-pool-factory.h>
#include <ns3/lte-sidelink-helper.h>
#include <ns3/lte-sl-resource-pool-factory.h>
#include <ns3/lte-sl-tft.h>
#include <ns3/lte-sl-ue-rrc.h>
#include <ns3/lte-sl-resource-pool-factory.h>
#include <ns3/lte-spectrum-phy.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/lte-control-messages.h>
#include <ns3/lte-enb-phy.h>
#include <ns3/lte-enb-rrc.h>

using namespace ns3;
using namespace psc;

//initial environment :initial parameter configurations 
NS_LOG_COMPONENT_DEFINE ("broadcast_call_technique");

//methods
//virtual void ReceiveFloorRelease (const McpttFloorMsgRelease& msg);
//virtual void Send (const McpttFloorMsg& msg);
NetDeviceContainer
LteHelper::InstallUeDevice (NodeContainer c)
{
  NS_LOG_FUNCTION (this);
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<NetDevice> device = InstallSingleUeDevice (node);
      devices.Add (device);
    }
  return devices;
}


class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  if (InetSocketAddress::IsMatchingType (m_peer))
    {
      m_socket->Bind ();
    }
  else
    {
      m_socket->Bind6 ();
    }
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}
/*
static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

*/

static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}


void 
ConfigureBroadcastGrpCall (ApplicationContainer& apps, uint32_t usersPerGroup, uint32_t baseGroupId)
{
  uint32_t groupId = baseGroupId;

  ObjectFactory callFac;
  callFac.SetTypeId ("ns3::McpttCallMachineGrpBroadcast");

  ObjectFactory floorFac;
  floorFac.SetTypeId ("ns3::McpttFloorMachineBasic");

  for (uint32_t idx = 0; idx < apps.GetN (); idx++)
    {
      Ptr<McpttPttApp> pttApp = DynamicCast<McpttPttApp, Application> (apps.Get (0));

      callFac.Set ("GroupId", UintegerValue (groupId));

      pttApp->CreateCall (callFac, floorFac);
      pttApp->SelectLastCall ();
 
      if ((idx + 1) % usersPerGroup == 0)
        {
          groupId += 1;
        }
    }
}


std::vector < NetDeviceContainer > AssociateForBroadcast_(double txPower,
double ulEarfcn, double ulBandwidth, NetDeviceContainer ues,
double rsrpThreshold, uint32_t nTransmitters,
LteSidelinkHelper::SrsrpMethod_t compMethod)
{
  std::vector < NetDeviceContainer > groups; //groups created

  NetDeviceContainer remainingUes; //list of UEs not assigned to groups
  remainingUes.Add (ues);

  // Start the selection of the transmitters
  NetDeviceContainer candidateTx; //list of UEs that can be selected for transmission
  candidateTx.Add (ues);
  NetDeviceContainer selectedTx;
  uint32_t numTransmittersSelected = 0;
  Ptr<LteHelper> m_lteHelper;
  Ptr<Object> uplinkPathlossModel = m_lteHelper->GetUplinkPathlossModel ();
  Ptr<PropagationLossModel> lossModel = uplinkPathlossModel->GetObject<PropagationLossModel> ();
  NS_ASSERT_MSG (lossModel != 0, " " << uplinkPathlossModel << " is not a PropagationLossModel");

  while (numTransmittersSelected < nTransmitters)
    {
      //Transmitter UE is randomly selected from the total number of UEs.
      //uint32_t iTx = m_uniformRandomVariable->GetValue (0, candidateTx.GetN ());
      Ptr<NetDevice> tx = candidateTx.Get (0);
      NS_LOG_DEBUG (" Candidate Tx= " << tx->GetNode ()->GetId ());
      selectedTx.Add (tx);
      
    }

  //For each remaining UE, associate to all transmitters where RSRP is greater than X dBm
  for (uint32_t i = 0; i < numTransmittersSelected; i++)
    {
      Ptr<NetDevice> tx = selectedTx.Get (i);
      //prepare group for this transmitter
      NetDeviceContainer newGroup (tx);
      uint32_t nRxDevices = remainingUes.GetN ();
      double rsrpRx = 0;

      for (uint32_t j = 0; j < nRxDevices; ++j)
        {
          Ptr<NetDevice> rx = remainingUes.Get (j);

          Ptr<SpectrumPhy> txPhy = tx->GetObject<LteUeNetDevice> ()->GetPhy ()->GetUlSpectrumPhy ();
          Ptr<SpectrumPhy> rxPhy = rx->GetObject<LteUeNetDevice> ()->GetPhy ()->GetUlSpectrumPhy ();

          if (compMethod == LteSidelinkHelper::SLRSRP_PSBCH)
            {
              rsrpRx = SidelinkRsrpCalculator::CalcSlRsrpPsbch (lossModel, txPower, ulEarfcn, ulBandwidth, txPhy, rxPhy);
            }
          else
            {
              rsrpRx = SidelinkRsrpCalculator::CalcSlRsrpTxPw (lossModel, txPower, txPhy, rxPhy);
            }
          //If receiver UE is not within RSRP* of X dBm of the transmitter UE then randomly reselect the receiver UE among the UEs
          //that are within the RSRP of X dBm of the transmitter UE and are not part of a group already.
          NS_LOG_DEBUG ("\tCandidate Rx= " << rx->GetNode ()->GetId () << " Rsrp=" << rsrpRx << " required=" << rsrpThreshold);
          if (rsrpRx >= rsrpThreshold)
            {
              //good receiver
              NS_LOG_DEBUG ("\tAdding Rx to group");
              newGroup.Add (rx);
            }
        }

      //Initializing link to other transmitters to be able to receive SLSSs from other transmitters
      for (uint32_t k = 0; k < numTransmittersSelected; k++)
        {
          if (k != i)
            {
              Ptr<NetDevice> othertx = selectedTx.Get (k);

              Ptr<SpectrumPhy> txPhy = tx->GetObject<LteUeNetDevice> ()->GetPhy ()->GetUlSpectrumPhy ();
              Ptr<SpectrumPhy> otherTxPhy = othertx->GetObject<LteUeNetDevice> ()->GetPhy ()->GetUlSpectrumPhy ();

              if (compMethod == LteSidelinkHelper::SLRSRP_PSBCH)
                {
                  rsrpRx = SidelinkRsrpCalculator::CalcSlRsrpPsbch (lossModel, txPower, ulEarfcn, ulBandwidth, txPhy, otherTxPhy);
                }
              else
                {
                  rsrpRx = SidelinkRsrpCalculator::CalcSlRsrpTxPw (lossModel, txPower, txPhy, otherTxPhy);
                }
              NS_LOG_DEBUG ("\tOther Tx= " << othertx->GetNode ()->GetId () << " Rsrp=" << rsrpRx);
            }
        }

      groups.push_back (newGroup);
    }

  return groups;
}

void
McpttTimer::SetDelay (const Time& delay)
{
  NS_LOG_FUNCTION (this << delay);

  Timer* rawTimer = GetRawTimer ();

  NS_ASSERT_MSG (!IsRunning (), "Timer " << GetId () << " has already been started.");

  NS_LOG_LOGIC ("Setting timer " << GetId () << " delay to " << delay << "."); 

  rawTimer->SetDelay (delay);
}

Timer*
McpttTimer::GetRawTimer (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rawTimer;
}

//start
int main (int argc, char *argv[])
{
//variables 
 Time startTime = Seconds (1);
 Time simTime = Seconds (25);
 Time stopTime = simTime;

 bool enableNsLogs = false; 
 // MCPTT configuration
 //variable declarations for using push to talk 
uint32_t appCount;
uint32_t groupcount = 1;
uint32_t usersPerGroup = 5;
DataRate dataRate = DataRate ("24kb/s");
uint32_t msgSize = 60; //60 + RTP header = 60 + 12 = 72
double maxX = 5.0;
double maxY = 5.0;
double pushTimeMean = 1.0; // seconds
double pushTimeVariance = 1.0; // seconds
double releaseTimeMean = 1.0; // seconds
double releaseTimeVariance = 1.0; // seconds

TypeId socketFacTid = UdpSocketFactory::GetTypeId ();
Ipv4Address peerAddress = Ipv4Address ("255.255.255.255");
appCount = usersPerGroup * groupcount;
McpttCallMsg msg;
Ptr<LteSlTft> tft;
 std::cout << "okay 1" << "\n" ;
 //building environment
//creating 3 nodes

//physical layer configurations  

//sidelink activation
std::cout << "okay 2" << "\n" ;

//Configure the UE for UE_SELECTED scenario
  Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (16));
  Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (5)); //The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1));
  Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (true)); //use default Trp index of 0

 std::cout << "okay 3" << "\n" ;

  //Set the frequency uplink
  uint32_t ulEarfcn = 18100;
  
  // Set error models
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (false));
 
 std::cout << "okay 4" << "\n" ;
 
  //Set the UEs power in dBm
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));

  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("enableNsLogs", "Enable ns-3 logging (debug builds)", enableNsLogs);
  cmd.Parse (argc, argv);

  std::cout << "okay 5" << "\n" ;

  //Sidelink bearers activation time
  Time slBearersActivationTime = startTime;

  std::cout << "okay 6" << "\n" ;
  
  //Create the helpers
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  std::cout << "okay 7" << "\n" ;
  
  //Create and set the EPC helper
  //Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  std::cout << "okay 8" << "\n" ;
  //lteHelper->SetEpcHelper (epcHelper);
  std::cout << "okay 9" << "\n" ;
  std::cout << "okay 10" << "\n" ;

  //Enable Sidelink
  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));
  std::cout << "okay 11" << "\n" ;
  //Set pathloss model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Cost231PropagationLossModel"));
  std::cout << "okay 12" << "\n" ;
  // channel model initialization
  lteHelper->Initialize ();
  std::cout << "okay 13" << "\n" ;
    // Since we are not installing eNB, we need to set the frequency attribute of pathloss model here
  double ulFreq = LteSpectrumValueHelper::GetCarrierFrequency (ulEarfcn);
  NS_LOG_LOGIC ("UL freq: " << ulFreq);
   std::cout << "okay 14" << "\n" ;
  Ptr<Object> uplinkPathlossModel = lteHelper->GetUplinkPathlossModel ();
   std::cout << "okay 15" << "\n" ;
  Ptr<PropagationLossModel> lossModel = uplinkPathlossModel->GetObject<PropagationLossModel> ();
   std::cout << "okay 16" << "\n" ;

  NS_ABORT_MSG_IF (lossModel == NULL, "No PathLossModel");
  bool ulFreqOk = uplinkPathlossModel->SetAttributeFailSafe ("Frequency", DoubleValue (ulFreq));
  if (!ulFreqOk)
    {
      NS_LOG_WARN ("UL propagation model does not have a Frequency attribute");
    }

  NS_LOG_INFO ("Deploying UE's...");
  
  std::cout << "okay 16.2" << "\n" ;

  //Create Sidelink prosehelper and set lteHelper
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);
  lteHelper->DisableEnbPhy (true);
  
  std::cout << "okay 16.5" << "\n" ;
  
  NodeContainer ueNodes;
  ueNodes.Create (appCount);
  
  //Install LTE UE devices to the nodes //error
  //topologer configuration  
  NS_LOG_INFO ("Building physical topology...");
  Ptr<RandomBoxPositionAllocator> rndBoxPosAllocator = CreateObject <RandomBoxPositionAllocator> ();
  rndBoxPosAllocator->SetX (CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (0.0), "Max", DoubleValue (maxX)));
  rndBoxPosAllocator->SetY (CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (0.0), "Max", DoubleValue (maxY)));
  rndBoxPosAllocator->SetZ (CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (1.5)));
  
   std::cout << "okay 22" << "\n" ;

  //allocating position of nodes 
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint32_t count = 0; count < appCount; count++)
    {
      Vector position = rndBoxPosAllocator->GetNext ();

      NS_LOG_INFO ("UE " << (count + 1) << " located at " << position << ".");

      positionAlloc->Add (position);
    }
  
  //mobility of nodes set to stationary
  MobilityHelper mobility;
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ueNodes);

  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);
  std::cout << "okay 17" << "\n" ;
  
  
  uint16_t ulBandwidth = 50;
  //Sidelink pre-configuration for the UEs
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  std::cout << "okay 26" << "\n" ;
  
  ueSidelinkConfiguration->SetSlEnabled (true);
  std::cout << "okay 27" << "\n" ;

  LteRrcSap::SlPreconfiguration preconfiguration;
  std::cout << "okay 28" << "\n" ;
  
  preconfiguration.preconfigGeneral.carrierFreq = ulEarfcn;
  preconfiguration.preconfigGeneral.slBandwidth = ulBandwidth;
  preconfiguration.preconfigComm.nbPools = 1;
  std::cout << "okay 29" << "\n" ;

  LteSlPreconfigPoolFactory pfactory;
  std::cout << "okay 30" << "\n" ;
  
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

  std::cout << "okay 31" << "\n" ;
  //synchronization

  
  int16_t syncTxThreshOoC = -60; //dBm
  uint16_t filterCoefficient = 0;  //k = 4 ==> a = 0.5, k = 0 ==> a = 1 No filter
  uint16_t syncRefMinHyst = 0; //dB
  uint16_t syncRefDiffHyst = 0; //dB
  std::cout << "okay 32" << "\n" ;
  
  //Synchronization
  preconfiguration.preconfigSync.syncOffsetIndicator1 = 18;
  preconfiguration.preconfigSync.syncOffsetIndicator2 = 29;
  preconfiguration.preconfigSync.syncTxThreshOoC = syncTxThreshOoC;
  preconfiguration.preconfigSync.syncRefDiffHyst = syncRefDiffHyst;
  preconfiguration.preconfigSync.syncRefMinHyst = syncRefMinHyst;
  preconfiguration.preconfigSync.filterCoefficient = filterCoefficient;
   std::cout << "okay 33" << "\n" ;

  preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();
   std::cout << "okay 34" << "\n" ;
  pfactory.SetHaveUeSelectedResourceConfig (true);
    
  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
  std::cout << "okay 35" << "\n" ;
  
  //error
  lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);
  //proseHelper->ActivateSidelinkBearer (Seconds (1.0), ueDevs, tft);
  
  //error
  //double ueTxPower = 23.0; 
 
  //error here
  //std::vector < NetDeviceContainer > createdgroups;
  //createdgroups = AssociateForBroadcast_(ueTxPower, ulEarfcn, ulBandwidth, ueDevs, -112, 1, LteSidelinkHelper::SLRSRP_PSBCH);
  std::cout << "okay 35" << "\n" ;
   
  std::cout << "okay 23" << "\n" ;
  //error??
  //Transport layer configs 
  //Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (ueNodes.Get (0), TcpSocketFactory::GetTypeId ());

  //network layer configurations 
  //installing wifi interface in nodes
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

  std::cout << "okay 24" << "\n" ;
 
  std::cout << "okay 25" << "\n" ;

  //installing internet access to all nodes
  NS_LOG_INFO ("Installing internet stack on all nodes...");
  InternetStackHelper internet;
  internet.Install (ueNodes);
  
  // assigning ip addresses to all devices of ueNodes
  NS_LOG_INFO ("Assigning IP addresses to each net device...");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (ueDevs);
  
  std::cout << "okay 36" << "\n" ;

  //????
  //Set Sidelink bearers
  //activate sidelink
  uint32_t groupL2Address = 10;
  NS_LOG_INFO ("Creating Sidelink configuration...");
  tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, peerAddress, groupL2Address);
  //error : need epchelper
  
   std::cout << "okay 37" << "\n" ;
  
  //application layer configurations 
  NS_LOG_INFO ("Creating applications...");

  ApplicationContainer clientApps;
  McpttHelper mcpttClient;
  McpttTimer mcpttTimer;
  
  //creating mcptt service on each node
  clientApps.Add (mcpttClient.Install (ueNodes));
  std::cout << "okay 38" << "\n" ;
  
  std::cout << "okay 39" << "\n" ;

  
  std::cout << "okay 40" << "\n" ;

  mcpttClient.SetPttApp ("ns3::McpttPttApp",
                         "PeerAddress", Ipv4AddressValue (peerAddress),
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

  std::cout << "okay 41" << "\n";

  clientApps.Start (startTime);
  clientApps.Stop (stopTime);
  std::cout << "okay 42" << "\n";

  /*Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, peerAddress, 1040, 1000, DataRate ("24kb/s"));
  ueNodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (startTime));
  app->SetStopTime (Seconds (stopTime));
  */
 
  //Actuating the call
  //creating location to store application info of UEs A, B 
  //floor control info (SCI) share to all participants
  ObjectFactory callFac;
  callFac.SetTypeId ("ns3::McpttCallMachineGrpBroadcast");
  std::cout << "okay 48" << "\n" ;
  ObjectFactory floorFac;
  floorFac.SetTypeId ("ns3::McpttFloorMachineBasic");
  std::cout << "okay 49" << "\n" ;
  
  Ptr<Packet> pkt = Create<Packet> ();

  pkt->AddHeader (msg);
  //NS_LOG_LOGIC (Simulator::Now ().GetSeconds () << "s: PttApp sending " << msg << ".");
 
  // generate call 
  Ptr<McpttPttApp> ueAPttApp = DynamicCast<McpttPttApp, Application> (clientApps.Get (0));


  ueAPttApp->CreateCall (callFac, floorFac);
  ueAPttApp->SelectLastCall ();
  std::cout << "okay 50" << "\n" ;
  
  std::cout << "okay 51" << "\n" ;
 
    
  std::cout << "okay 43" << "\n" ;
  //creating call interfaces and location to store call of UEs A, B 
  Ptr<McpttCall> ueACall = ueAPttApp->GetSelectedCall ();
  
  std::cout << "okay 44" << "\n" ;
  
  Ptr<McpttPusher> ueAPusher = ueAPttApp->GetPusher ();
    
  std::cout << "okay 45" << "\n" ;
  Ptr<McpttMediaSrc> ueAMediaSrc = ueAPttApp->GetMediaSrc ();
  
  
  std::cout << "okay 46" << "\n" ;
  //???
  Ptr<McpttChan> callChan = ueAPttApp->GetCallChan ();
  //callChan->Send (pkt);

 //push button press schedule
  Simulator::Schedule (Seconds (1.1), &McpttPttApp::TakePushNotification, ueAPttApp);
  std::cout << "okay 47" << "\n" ;
 //floor request
 //floor grant 
 //assume floor granted 
   
  //ConfigureBroadcastGrpCall (clientApps, usersPerGroup, 1);
  std::cout << "okay 52" << "\n" ;

  //creating call interfaces and location to store call of UEs A, B 
  uint16_t floorPort = McpttPttApp::AllocateNextPortNumber ();
  uint16_t speechPort = McpttPttApp::AllocateNextPortNumber ();
  Ipv4AddressValue grpAddress;
  
  /* 
  McpttTimer* m_rawTimer;
  Timer* rawTimer = m_rawTimer->GetRawTimer ();
  */
  //set a delay of delaytfb1
  //Simulator::Schedule (Seconds (5), &McpttTimer::Start,rawTimer);
  Simulator::Schedule (Seconds (5.15), &McpttCall::OpenFloorChan, ueACall, grpAddress.Get (), floorPort);
  Simulator::Schedule (Seconds (6), &McpttCall::OpenMediaChan, ueACall, grpAddress.Get (), speechPort);
 
  // synchronization and in progress 
  
  //end of call
  std::cout << "okay 53" << "\n" ;
  Simulator::Schedule (Seconds (6), &McpttPttApp::ReleaseCall, ueAPttApp);
  std::cout << "okay 54" << "\n" ;
  Simulator::Schedule (Seconds (15), &McpttPttApp::TakePushNotification, ueAPttApp);
  std::cout << "okay 55" << "\n" ;
  

  //results
  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("b5.pcap", std::ios::out, PcapHelper::DLT_PPP);
  std::cout << "okay 56" << "\n" ;
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("b5.cwnd");
  ueDevs.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));
  std::cout << "okay 57" << "\n" ;
 
  NS_LOG_INFO ("Enabling Sidelink traces...");
  std::cout << "okay 58" << "\n" ;
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
  std::cout << "okay 61" << "\n" ;
  NS_LOG_INFO ("Starting simulation...");
  AnimationInterface anim("b5.xml");
  anim.SetMaxPktsPerTraceFile(500000); 
  std::cout << "okay 62" << "\n" ;
  
  Simulator::Stop (Seconds (stopTime));
  //anim.SetConstantPosition(nodes.Get(0),1.0,2.0);
  //anim.SetConstantPosition(nodes.Get(1),4.0,5.0);
  std::cout << "okay 63" << "\n" ;
  Simulator::Run ();
  std::cout << "okay 64" << "\n" ;
  Simulator::Destroy();
  std::cout << "okay 65" << "\n" ;
  NS_LOG_UNCOND ("Done Simulator");
  
  NS_LOG_INFO ("Done.");
  
  
}
