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

using namespace ns3;
//using namespace psc;

//initial environment :initial parameter configurations 
NS_LOG_COMPONENT_DEFINE ("broadcast_call_technique");

//virtual void ReceiveFloorRelease (const McpttFloorMsgRelease& msg);
//virtual void Send (const McpttFloorMsg& msg);

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


class BroadcastTestCallMachine : public McpttCallMachineGrpBroadcast
{
public:
 static TypeId GetTypeId (void);
 BroadcastTestCallMachine (void);
 virtual ~BroadcastTestCallMachine (void);
 virtual void ChangeState (Ptr<McpttCallMachineGrpBroadcastState>  newState);
 virtual TypeId GetInstanceTypeId (void) const;
 virtual void Receive (const McpttCallMsg& msg);
 virtual void Start (void);
 virtual void Send (const McpttCallMsg& msg);
protected:
 virtual void ExpiryOfTfb1 (void);
 virtual void ExpiryOfTfb2 (void);
 virtual void ExpiryOfTfb3 (void);
private:
 Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&> m_postRxCb;
 Callback<void, const BroadcastTestCallMachine&, const McpttTimer&> m_postTimerExpCb;
 Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&> m_postTxCb;
 Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&> m_preRxCb;
 Callback<void, const BroadcastTestCallMachine&, const McpttTimer&> m_preTimerExpCb;
 Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&> m_preTxCb;
 Ptr<McpttCallMachineGrpBroadcastState> m_startState;
 Callback<void, const BroadcastTestCallMachine&, Ptr<McpttCallMachineGrpBroadcastState> , Ptr<McpttCallMachineGrpBroadcastState> > m_stateChangeCb;
public:
 virtual Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&> GetPostRxCb (void) const;
 virtual Callback<void, const BroadcastTestCallMachine&, const McpttTimer&> GetPostTimerExpCb (void) const;
 virtual Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&> GetPostTxCb (void) const;
 virtual Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&> GetPreRxCb (void) const;
 virtual Callback<void, const BroadcastTestCallMachine&, const McpttTimer&> GetPreTimerExpCb (void) const;
 virtual Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&> GetPreTxCb (void) const;
 virtual Ptr<McpttCallMachineGrpBroadcastState> GetStartState(void) const;
 virtual Callback<void, const BroadcastTestCallMachine&, Ptr<McpttCallMachineGrpBroadcastState> , Ptr<McpttCallMachineGrpBroadcastState> > GetStateChangeCb (void) const;
 virtual void SetPostRxCb (const Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&>  postRxCb);
 virtual void SetPostTimerExpCb (const Callback<void, const BroadcastTestCallMachine&, const McpttTimer&>  timerExpCb);
 virtual void SetPostTxCb (const Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&>  postTxCb);
 virtual void SetPreRxCb (const Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&>  preRxCb);
 virtual void SetPreTimerExpCb (const Callback<void, const BroadcastTestCallMachine&, const McpttTimer&>  timerExpCb);
 virtual void SetPreTxCb (const Callback<void, const BroadcastTestCallMachine&, const McpttCallMsg&>  preTxCb);
 virtual void SetStartState (Ptr<McpttCallMachineGrpBroadcastState>  startState);
 virtual void SetStateChangeTestCb (const Callback<void, const BroadcastTestCallMachine&, Ptr<McpttCallMachineGrpBroadcastState> , Ptr<McpttCallMachineGrpBroadcastState> >  stateChangeCb);
};



int main (int argc, char *argv[])
{
   
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
Ipv4Address peerAddress = Ipv4Address ("255.255.255.255");
appCount = usersPerGroup * groupcount;
  
uint16_t floorPort = McpttPttApp::AllocateNextPortNumber ();
uint16_t speechPort = McpttPttApp::AllocateNextPortNumber ();
Ipv4AddressValue grpAddress;


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
nodes.Create (appCount);

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
  

//mobility of nodes set to stationary
MobilityHelper mobility;
mobility.SetPositionAllocator (positionAlloc);
mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
mobility.Install (nodes);

//sidelink pre-configuration
//Configure the UE for UE_SELECTED scenario
Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (16));
Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (5)); //The number of RBs allocated per UE for Sidelink
Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1));
Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (true)); //use default Trp index of 0

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

//cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
//cmd.AddValue ("enableNsLogs", "Enable ns-3 logging (debug builds)", enableNsLogs);
//cmd.Parse (argc, argv);

//Sidelink bearers activation time
//Time slBearersActivationTime = startTime;

//Create the helpers
Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

//Create and set the EPC helper
//Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
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

//NetDeviceContainer devices = lteHelper->InstallUeDevice (nodes);
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
    
NetDeviceContainer devices;
ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
lteHelper->InstallSidelinkConfiguration (devices, ueSidelinkConfiguration);
  
/*Network layer***************************************************************************************/
//installing wifi interface in nodes
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
devices = wifi.Install (phy, mac, nodes);

//installing internet access to all nodes
NS_LOG_INFO ("Installing internet stack on all nodes...");
InternetStackHelper internet;
internet.Install (nodes);
  
// assigning ip addresses to all devices of nodes
NS_LOG_INFO ("Assigning IP addresses to each net device...");
Ipv4AddressHelper ipv4;
ipv4.SetBase ("10.1.1.0", "255.255.255.0");
Ipv4InterfaceContainer i = ipv4.Assign (devices);
ns3::PacketMetadata::Enable ();
/*Application layer*************************************************************************************/

NS_LOG_INFO ("Creating applications...");
//creating mcptt application for each device 
 
ApplicationContainer clientApps;
McpttHelper mcpttHelper;
McpttTimer mcpttTimer;
  
  //creating mcptt service on each node
  clientApps.Add (mcpttHelper.Install (nodes));
  
  clientApps.Start (startTime);
  clientApps.Stop (stopTime);

  mcpttHelper.SetPttApp ("ns3::McpttPttApp",
                         "PeerAddress", Ipv4AddressValue (peerAddress),
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
  


  //UE B 
  
  
  std::string orgName = "EMS";
  Time joinTime = Seconds (2.2);
  uint32_t origId = ueAPttApp->GetUserId ();
  Ipv4Address origAddress = ueBPttApp->GetLocalAddress ();

  ueBPttApp->GetAttribute ("PeerAddress", grpAddress);

  McpttCallMsgFieldSdp sdp;
  sdp.SetFloorPort (floorPort);
  sdp.SetGrpAddr (grpAddress.Get ());
  sdp.SetOrigAddr (origAddress);
  sdp.SetSpeechPort (speechPort);
    //UE A
 
  //uint16_t BcallId = 2;
  //uint32_t BorigId = ueBPttApp->GetUserId ();
 
   
    //UE C
  
  //uint16_t CcallId = 3;
  //uint32_t CorigId = ueCPttApp->GetUserId ();
 
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
  

  //m_startStatertState (McpttCallMachineGrpBroadcastStateB2::GetInstance ());

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

//Simulator::Schedule (Seconds (5.15), &McpttCall::OpenFloorChan, ueBCall, grpAddress.Get (), floorPort);


/*McpttCallMsgGrpEmergAlert::McpttCallMsgGrpEmergAlert (void)
  : McpttCallMsg (McpttCallMsgGrpEmergAlert::CODE),
    m_grpId (McpttCallMsgFieldGrpId ()),
    m_orgName (McpttCallMsgFieldOrgName ()),
    m_userId (McpttCallMsgFieldUserId ()),
    m_userLoc (McpttCallMsgFieldUserLoc ());
*/
  
  //floor request generate
  //  ns3::McpttFloorMachine ns3::McpttFloorMachineBasic
//ns3::McpttFloorMachineBasicState
//ns3:McpttFloorMachineBasicStateHasPerm
//ns3::McpttCallTypeMachine

  //turn off floor control ns3::McpttFloorMachineNull
  //
  //ns3::McpttMsg
  //ns3::McpttCallMsg
  //ns3::McpttFloorMsg
  //ns3::McpttFloorMsgRequest
  //ns3::McpttFloorMsgFieldUserId

  //ns3::McpttMediaMsg
  //ns3::McpttCallMsg

  //ns3::McpttFloorQueue ==0

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
 //SetUserLoc (const McpttCallMsgFieldUserLoc& userLoc)*************


//******************************* 
       
//send floor control info (SCI) share to all participants**************

//relay : reception of SCI and decide to join the call


//generate message************************************ 

//Ptr<McpttTimer> ueATfb1 = ueAMachine->GetTfb1 ();
Ptr<Packet> pkt = Create<Packet> ();
pkt->AddHeader (msg);
//callChan->Send (pkt);  
 
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
  wifiPhy.EnableAsciiAll (stream);
  internet.EnableAsciiIpv4All (stream);
*stream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;


  AsciiTraceHelper ascii_wifi;
  wifiPhy.EnableAsciiAll (ascii_wifi.CreateFileStream ("wifi-packet-socket.tr"));
  

  AsciiTraceHelper ascii_r;
  Ptr<OutputStreamWrapper> rtw = ascii_r.CreateFileStream ("routing_table");

  AsciiTraceHelper ascii_trace;

  std::ostringstream oss;
  Ptr<OutputStreamWrapper> packetOutputStream = ascii_trace.CreateFileStream ("b20_trace.txt");
  *packetOutputStream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;


  //Trace file table header
  *stream->GetStream () << "time(sec)\ttx/rx\tNodeID\tIMSI\tPktSize(bytes)\tIP[src]\tIP[dst]" << std::endl;

NS_LOG_INFO ("Enabling MCPTT traces...");
mcpttHelper.EnableMsgTraces ();
mcpttHelper.EnableStateMachineTraces ();
  
NS_LOG_INFO ("Starting simulation...");
AnimationInterface anim("b20.xml");
anim.SetMaxPktsPerTraceFile(500000);

//Simulator::Stop (Seconds (stopTime));
//anim.SetConstantPosition(nodes.Get(0),1.0,2.0);
//anim.SetConstantPosition(nodes.Get(1),4.0,5.0);
Simulator::Run ();
Simulator::Destroy();

NS_LOG_UNCOND ("Done Simulator");

NS_LOG_INFO ("Done.");
  
  
}