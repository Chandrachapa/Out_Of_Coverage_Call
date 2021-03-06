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
#include <ns3/netanim-module.h>
#include <ns3/wifi-module.h>
#include <ns3/mcptt-helper.h>
#include <ns3/mcptt-call-machine-grp-broadcast.h>
#include <ns3/mcptt-call-machine-grp-broadcast-state.h>
#include <ns3/callback.h>

#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/lte-rlc.h"
#include "ns3/lte-rlc-tag.h"
#include "ns3/lte-mac-sap.h"
#include "ns3/lte-rlc-sap.h"
#include "ns3/ff-mac-sched-sap.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wns3-2017-discovery");

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

void
PrintGnuplottableNodeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  outFile << "set xrange [-200:200]; set yrange [-200:200]" << std::endl;
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> vdev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (vdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << vdev->GetMac()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 7 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;

              // Simulator::Schedule (Seconds (1), &PrintHelper::UpdateGnuplottableNodeListToFile, filename, node);
            }
        }
    }
}

/*Synchronization traces*/
void
NotifyChangeOfSyncRef (Ptr<OutputStreamWrapper> stream, LteUeRrc::SlChangeOfSyncRefStatParameters param)
{
  *stream->GetStream () << Simulator::Now ().GetMilliSeconds () << "\t" << param.imsi << "\t" << param.prevSlssid << "\t" << param.prevRxOffset << "\t" << param.prevFrameNo << "\t" << param.prevSubframeNo <<
  "\t" << param.currSlssid << "\t" << param.currRxOffset << "\t" << param.currFrameNo << "\t" << param.currSubframeNo << std::endl;
}

void
NotifySendOfSlss (Ptr<OutputStreamWrapper> stream, uint64_t imsi, uint64_t slssid, uint16_t txOffset, bool inCoverage, uint16_t frame, uint16_t subframe)
{
  *stream->GetStream () << Simulator::Now ().GetMilliSeconds () << "\t" << imsi << "\t"  << slssid << "\t" << txOffset << "\t" << inCoverage << "\t" << frame <<  "\t" << subframe << std::endl;
}


int main (int argc, char *argv[])
{
  // out of coverage scenario 
  // Initialize some values
  double simTime = 10;
  uint32_t nbUes = 10;
  uint16_t txProb = 100;
  bool useRecovery = false;
  bool  enableNsLogs = false; // If enabled will output NS LOGs

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time", simTime);
  cmd.AddValue ("numUe", "Number of UEs", nbUes);
  cmd.AddValue ("txProb", "initial transmission probability", txProb);
  cmd.AddValue ("enableRecovery", "error model and HARQ for D2D Discovery", useRecovery);
  cmd.AddValue ("enableNsLogs", "Enable NS logs", enableNsLogs);

  cmd.Parse (argc, argv);

  if (enableNsLogs)
    {
      LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);
      LogComponentEnable ("wns3-2017-discovery", logLevel);
      LogComponentEnable ("LteSpectrumPhy", logLevel);
      LogComponentEnable ("LteUePhy", logLevel);
      LogComponentEnable ("LteUeRrc", logLevel);
      LogComponentEnable ("LteEnbPhy", logLevel);
      LogComponentEnable ("LteUeMac", logLevel);
      LogComponentEnable ("LteSlUeRrc", logLevel);
      LogComponentEnable ("LteSidelinkHelper", logLevel);
      LogComponentEnable ("LteHelper", logLevel);
    }

  

  // Set the UEs power in dBm
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));
  // Use error model and HARQ for D2D Discovery (recovery process)
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDiscoveryErrorModelEnabled", BooleanValue (useRecovery));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (true));

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

  // Set error models
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  NS_LOG_INFO ("Creating helpers...");
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  Ptr<LteSidelinkHelper> sidelinkHelper = CreateObject<LteSidelinkHelper> ();
  sidelinkHelper->SetLteHelper (lteHelper);

  // Set pathloss model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisPropagationLossModel"));

  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));
  lteHelper->Initialize ();

  // Since we are not installing eNB, we need to set the frequency attribute of pathloss model here
  // Frequency for Public Safety use case (band 14 : 788 - 798 MHz for Uplink)
  double ulFreq = LteSpectrumValueHelper::GetCarrierFrequency (23330);
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
  NodeContainer ueNodes;
  ueNodes.Create (nbUes);

  //Position of the nodes
  Ptr<ListPositionAllocator> positionAllocUe = CreateObject<ListPositionAllocator> ();

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      double x = rand->GetValue (-100,100);
      double y = rand->GetValue (-100,100);
      double z = 1.5;
      positionAllocUe->Add (Vector (x, y, z));
    }

  // Install mobility

  MobilityHelper mobilityUe;
  mobilityUe.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUe.SetPositionAllocator (positionAllocUe);
  mobilityUe.Install (ueNodes);


  lteHelper->DisableEnbPhy (true);
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);

  //Fix the random number stream
  uint16_t randomStream = 1;
  randomStream += lteHelper->AssignStreams (ueDevs, randomStream);

  AsciiTraceHelper asc;
  Ptr<OutputStreamWrapper> st = asc.CreateFileStream ("discovery_nodes.txt");
  *st->GetStream () << "id\tx\ty" << std::endl;

  for (uint32_t i = 0; i < ueDevs.GetN (); ++i)
    {
      Ptr<LteUeNetDevice> ueNetDevice = DynamicCast<LteUeNetDevice> (ueDevs.Get (i));
      uint64_t imsi = ueNetDevice->GetImsi ();
      Vector pos = ueNodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ();
      std::cout << "UE " << i << " id = " << ueNodes.Get (i)->GetId () << " / imsi = " << imsi << " / position = [" << pos.x << "," << pos.y << "," << pos.z << "]" << std::endl;
      *st->GetStream () << imsi << "\t" << pos.x << "\t" << pos.y << std::endl;
    }

  NS_LOG_INFO ("Configuring discovery pool for the UEs...");
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  ueSidelinkConfiguration->SetDiscEnabled (true);

  //todo: specify parameters before installing in UEs if needed
  LteRrcSap::SlPreconfiguration preconfiguration;

  preconfiguration.preconfigGeneral.carrierFreq = 23330;
  preconfiguration.preconfigGeneral.slBandwidth = 50;
  preconfiguration.preconfigDisc.nbPools = 1;

  //-Configure preconfigured communication pool
preconfiguration.preconfigComm = sidelinkHelper->GetDefaultSlPreconfigCommPoolList ();

//-Configure preconfigured discovery pool
preconfiguration.preconfigDisc = sidelinkHelper->GetDefaultSlPreconfigDiscPoolList ();

//-Configure preconfigured UE-to-Network Relay parameters
preconfiguration.preconfigRelay = sidelinkHelper->GetDefaultSlPreconfigRelay ();


preconfiguration.preconfigDisc.pools[0].cpLen.cplen = LteRrcSap::SlCpLen::NORMAL;
preconfiguration.preconfigDisc.pools[0].discPeriod.period = LteRrcSap::SlPeriodDisc::rf32;
preconfiguration.preconfigDisc.pools[0].numRetx = 0;
preconfiguration.preconfigDisc.pools[0].numRepetition = 1;
preconfiguration.preconfigDisc.pools[0].tfResourceConfig.prbNum = 10;
preconfiguration.preconfigDisc.pools[0].tfResourceConfig.prbStart = 10;
preconfiguration.preconfigDisc.pools[0].tfResourceConfig.prbEnd = 49;
preconfiguration.preconfigDisc.pools[0].tfResourceConfig.offsetIndicator.offset = 0;
preconfiguration.preconfigDisc.pools[0].tfResourceConfig.subframeBitmap.bitmap = std::bitset<40> (0x11111);
preconfiguration.preconfigDisc.pools[0].txParameters.txParametersGeneral.alpha = LteRrcSap::SlTxParameters::al09;
preconfiguration.preconfigDisc.pools[0].txParameters.txParametersGeneral.p0 = -40;
preconfiguration.preconfigDisc.pools[0].txParameters.txProbability = SidelinkDiscResourcePool::TxProbabilityFromInt (txProb);

/* Synchronization*/
int16_t syncTxThreshOoC = -60; //dBm
uint16_t syncRefDiffHyst = 0; //dB
uint16_t syncRefMinHyst = 0; //dB
uint16_t filterCoefficient = 0; 
//Synchronization parameters
preconfiguration.preconfigSync.syncOffsetIndicator1 = 18;
preconfiguration.preconfigSync.syncOffsetIndicator2 = 29;
preconfiguration.preconfigSync.syncTxThreshOoC = syncTxThreshOoC;
preconfiguration.preconfigSync.syncRefDiffHyst = syncRefDiffHyst;
preconfiguration.preconfigSync.syncRefMinHyst = syncRefMinHyst;
preconfiguration.preconfigSync.filterCoefficient = filterCoefficient;
  /* END Synchronization*/
  

NS_LOG_INFO ("Install Sidelink discovery configuration in the UEs...");
  
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
pfactory.SetControlPrbEnd(49);
//synchronization

preconfiguration.preconfigSync.syncOffsetIndicator1 = 18;
preconfiguration.preconfigSync.syncOffsetIndicator2 = 29;
preconfiguration.preconfigSync.syncTxThreshOoC = -60; //dBm;
preconfiguration.preconfigSync.syncRefDiffHyst = 0; //dB;
preconfiguration.preconfigSync.syncRefMinHyst = 0; //dB;
preconfiguration.preconfigSync.filterCoefficient = 0;  //k = 4 ==> a = 0.5, k = 0 ==> a = 1 No filter;

preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();
pfactory.SetHaveUeSelectedResourceConfig (true);

  
ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);

uint16_t base_t = 2000; //ms

uint16_t devIt = 1;
ueDevs.Get (devIt)->GetObject<LteUeNetDevice> ()->GetRrc ()->SetSlssid (devIt + 10);
ueDevs.Get (devIt)->GetObject<LteUeNetDevice> ()->GetPhy ()->SetFirstScanningTime (MilliSeconds (base_t + (devIt * base_t)));

NS_LOG_INFO ("Configuring discovery applications");
std::map<Ptr<NetDevice>, std::list<uint32_t> > announcePayloads; 
std::map<Ptr<NetDevice>, std::list<uint32_t> > monitorPayloads; 

for (uint32_t i = 1; i <= nbUes; ++i)
  {
    announcePayloads[ueDevs.Get (i - 1)].push_back (i);

    for (uint32_t j = 1; j <= nbUes; ++j)
      {
        if (i != j)
          {
            monitorPayloads[ueDevs.Get (i - 1)].push_back (j);
          }
      }
  }

for (uint32_t i = 0; i < nbUes; i++)
  {
    Simulator::Schedule (Seconds (2.0), &LteSidelinkHelper::StartDiscoveryApps, sidelinkHelper, ueDevs.Get (i), announcePayloads[ueDevs.Get (i)], LteSlUeRrc::Announcing);
    Simulator::Schedule (Seconds (2.0), &LteSidelinkHelper::StartDiscoveryApps, sidelinkHelper, ueDevs.Get (i), monitorPayloads[ueDevs.Get (i)], LteSlUeRrc::Monitoring);
  }

AsciiTraceHelper ascii;
Ptr<OutputStreamWrapper> streamSyncRef = ascii.CreateFileStream ("SyncRef.txt");
*streamSyncRef->GetStream () << "Time\tIMSI\tprevSLSSID\tprevRxOffset\tprevFrameNo\tprevSframeNo\tcurrSLSSID\tcurrRxOffset\tcurrFrameNo\tcurrSframeNo" << std::endl;

Ptr<OutputStreamWrapper> streamSendOfSlss = ascii.CreateFileStream ("TxSlss.txt");
*streamSendOfSlss->GetStream () << "Time\tIMSI\tSLSSID\ttxOffset\tinCoverage\tFrameNo\tSframeNo" << std::endl;

for (uint32_t i = 0; i < ueDevs.GetN (); ++i)
{
    Ptr<LteUeRrc> ueRrc =  ueDevs.Get (i)->GetObject<LteUeNetDevice> ()->GetRrc ();
    ueRrc->TraceConnectWithoutContext ("ChangeOfSyncRef", MakeBoundCallback (&NotifyChangeOfSyncRef, streamSyncRef));
    ueRrc->TraceConnectWithoutContext ("SendSLSS", MakeBoundCallback (&NotifySendOfSlss, streamSendOfSlss));
}

  /*Synchronization*/
//Set initial SLSSID and start of the first scanning for all UEs
uint32_t baseTime = 2000; //ms
uint32_t firstScanningTime = 0;

for (uint32_t i = 0; i < ueDevs.GetN (); i++)
{
  uint64_t tempSlssid = i + 10;
  ueDevs.Get (i)->GetObject<LteUeNetDevice> ()->GetRrc ()->SetSlssid (tempSlssid);
  firstScanningTime = baseTime + (i * baseTime);
  ueDevs.Get (i)->GetObject<LteUeNetDevice> ()->GetPhy ()->SetFirstScanningTime (MilliSeconds (firstScanningTime));
  uint64_t imsi = ueDevs.Get (i)->GetObject<LteUeNetDevice> ()->GetRrc ()->GetImsi ();
  NS_LOG_INFO ("IMSI " << imsi << " SLSSID " << tempSlssid
                        << " First Scan "  << firstScanningTime << " ms");
}
    
/*NAS layer*******/
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
// ueDevs = wifi.Install (phy, mac, ueNodes);


LteSidelinkHelper ltesidelink;


//installing internet access to all nodes
NS_LOG_INFO ("Installing internet stack on all nodes...");
InternetStackHelper internet;
internet.Install (ueNodes);

// assigning ip addresses to all devices of nodes
NS_LOG_INFO ("Assigning IP addresses to each net device...");
Ipv4AddressHelper ipv4;
ipv4.SetBase ("10.1.1.0", "255.255.255.0");
Ipv4InterfaceContainer i = epcHelper->AssignUeIpv4Address (ueDevs);

Ipv4StaticRoutingHelper ipv4RoutingHelper;
  // Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ueDevs.Get (0)->GetObject<Ipv4> ());
  // staticRouting->SetDefaultRoute (ueDevs.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

for (uint32_t u = 0; u < ueDevs.GetN (); ++u)
  {
    Ptr<Node> ueNode = ueNodes.Get (u);
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

Ipv4Address remoteUeaddress =  i.GetAddress(0);
Ipv4Address relayUeaddress  =  i.GetAddress(1);
// Ipv4Address groupAddress4 = Ipv4Address ("255.255.255.255");
Address remoteAddress;
Address localAddress;
remoteAddress = InetSocketAddress (remoteUeaddress, 8000);
localAddress  = InetSocketAddress (relayUeaddress, 8000);
uint32_t groupL2Address = 255;
Time slBearersActivationTime = Seconds (1.0);
lteHelper->Attach(ueDevs);
Ptr<LteSlTft> tft = Create<LteSlTft> (LteSlTft::TRANSMIT, relayUeaddress, groupL2Address);
sidelinkHelper->ActivateSidelinkBearer (Seconds (1.0), ueDevs.Get(0), tft);
tft = Create<LteSlTft> (LteSlTft::RECEIVE, relayUeaddress, groupL2Address);
sidelinkHelper->ActivateSidelinkBearer (slBearersActivationTime, ueDevs.Get(1), tft);


ns3::PacketMetadata::Enable ();

/*Application layer*************************************************************************************/

NS_LOG_INFO ("Creating applications...");
//creating mcptt application for each device 
Time startTime = Seconds (1);
Time stopTime = Seconds (10);
Ipv4Address peerAddress = Ipv4Address ("255.255.255.255");
uint32_t msgSize = 60; //60 + RTP header = 60 + 12 = 72
DataRate dataRate = DataRate ("24kb/s");
double pushTimeMean = 5.0; // seconds
double pushTimeVariance = 2.0; // seconds
double releaseTimeMean = 5.0; // seconds
double releaseTimeVariance = 2.0; // seconds


ApplicationContainer clientApps;
McpttHelper mcpttHelper;
McpttTimer mcpttTimer;

//creating mcptt service on each node
clientApps.Add (mcpttHelper.Install (ueNodes));

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


ObjectFactory callFac;
callFac.SetTypeId ("ns3::McpttCallMachineGrpBroadcast");

ObjectFactory floorFac;
floorFac.SetTypeId ("ns3::McpttFloorMachineBasic");

Ipv4AddressValue grpAddr;

//creating location to store application info of UEs A, B 
Ptr<McpttPttApp> ueAPttApp = DynamicCast<McpttPttApp, Application> (clientApps.Get (0));
Ptr<McpttPttApp> ueBPttApp = DynamicCast<McpttPttApp, Application> (clientApps.Get (1));
Ptr<McpttPttApp> ueCPttApp = DynamicCast<McpttPttApp, Application> (clientApps.Get (2));

/****added */
uint32_t grpId = 1;
uint16_t callId = 1;

//UE B 

uint16_t floorPort = McpttPttApp::AllocateNextPortNumber ();
uint16_t speechPort = McpttPttApp::AllocateNextPortNumber ();
Ipv4AddressValue grpAddress;

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

//bool m_userAckReq; //!< Indicates if user acknowledgments are required.
Time delayTfb1 = Seconds(10);
Time delayTfb2 = Seconds(5);
Time delayTfb3 = Seconds(5);
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
McpttCallMsg msg;
pkt->AddHeader (msg);
//callChan->Send (pkt);  

NS_LOG_LOGIC (Simulator::Now ().GetSeconds () << "s: PttApp sending " << msg << ".");

// LteRlcSm lteRlc;
// lteRlc.DoTransmitPdcpPdu(pkt);









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


/*udp application*/
  //Set Application in the UEs
  // Ipv4Address groupAddress ("225.0.0.0"); //use multicast address as destination
  // UdpClientHelper udpClient (groupAddress, 8000);
  // udpClient.SetAttribute ("MaxPackets", UintegerValue (500));
  // udpClient.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
  // udpClient.SetAttribute ("PacketSize", UintegerValue (280));

  // clientApps = udpClient.Install (ueNodes.Get (0));
  // clientApps.Get (0)->SetStartTime (Seconds (3.0));
  // clientApps.Stop (Seconds (5.0));

  // ApplicationContainer serverApps;
  // PacketSinkHelper sidelinkSink ("ns3::UdpSocketFactory",Address (InetSocketAddress (Ipv4Address::GetAny (), 8000)));
  // serverApps = sidelinkSink.Install (ueNodes.Get (1));
  // serverApps.Get (0)->SetStartTime (Seconds (3.0));
  // lteHelper->Attach (ueDevs.Get(1));
  OnOffHelper sidelinkClient ("ns3::UdpSocketFactory", remoteAddress);
  sidelinkClient.SetConstantRate (DataRate ("16kb/s"), 200);

  clientApps = sidelinkClient.Install (ueNodes.Get (0));
  //onoff application will send the first packet at :
  //(2.9 (App Start Time) + (1600 (Pkt size in bits) / 16000 (Data rate)) = 3.0 sec
  clientApps.Start (slBearersActivationTime + Seconds (0.9));
  clientApps.Stop (stopTime - slBearersActivationTime + Seconds (1.0));

  ApplicationContainer serverApps;
  PacketSinkHelper sidelinkSink ("ns3::UdpSocketFactory", localAddress);
  serverApps = sidelinkSink.Install (ueNodes.Get (1));
  serverApps.Start (Seconds (2.0));
  // PrintGnuplottableNodeListToFile ("scenario.txt");
sidelinkHelper->ActivateSidelinkBearer (slBearersActivationTime, ueDevs, tft);
  // Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  
  // Ptr<MmWaveHelper> ptr_mmWave = CreateObject<MmWaveHelper> ();
  // ptr_mmWave->ActivateDataRadioBearer (ueDevs, bearer);
/*****************************************************************/

// assigning ip addresses to all devices of nodes
NS_LOG_INFO ("Assigning IP addresses to each net device...");


 Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("discovery.tr");
//   wifiPhy.EnableAsciiAll (stream);

internet.EnableAsciiIpv4All (stream);
*stream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;

  std::ostringstream oss;

//   AsciiTraceHelper ascii_wifi;
//   wifiPhy.EnableAsciiAll (ascii_wifi.CreateFileStream ("wifi-packet-socket.tr"));
      // Set Tx traces
for (uint16_t i=0; i< ueNodes.GetN()  ; i++){
for (uint16_t ac = 0; ac < clientApps.GetN (); ac++)
  {
    Ipv4Address localAddrs =  clientApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
    std::cout << "Tx address: " << localAddrs << std::endl;
    oss << "tx\t" << ueNodes.Get (i)->GetId () << "\t" << ueNodes.Get (0)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
    clientApps.Get (ac)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
    oss.str ("");
  }

// Set Rx traces
for (uint16_t ac = 0; ac < serverApps.GetN (); ac++)
  {
    Ipv4Address localAddrs =  serverApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
    std::cout << "Rx address: " << localAddrs << std::endl;
    oss << "rx\t" << ueNodes.Get (i)->GetId () << "\t" << ueNodes.Get (1)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
    serverApps.Get (ac)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
    oss.str ("");
  }
}
AsciiTraceHelper ascii_trace;


Ptr<OutputStreamWrapper> packetOutputStream = ascii_trace.CreateFileStream ("discovery_trace.txt");
*packetOutputStream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;


std::ostringstream txWithAddresses;
txWithAddresses << "/NodeList/" << ueDevs.Get (0)->GetNode ()->GetId () << "/ApplicationList/0/TxWithAddresses";


std::ostringstream rxWithAddresses;
rxWithAddresses << "/NodeList/" << ueDevs.Get (0)->GetNode ()->GetId () << "/ApplicationList/0/RxWithAddresses";

//Trace file table header
*stream->GetStream () << "time(sec)\ttx/rx\tNodeID\tIMSI\tPktSize(bytes)\tIP[src]\tIP[dst]" << std::endl;


  ///*** End of application configuration ***///
  //other traces
 
// Set Discovery Traces
//Enable traces

lteHelper->EnablePhyTraces ();
lteHelper->EnableMacTraces ();
lteHelper->EnableRlcTraces ();
lteHelper->EnablePdcpTraces ();

lteHelper->EnableSlPscchMacTraces ();

lteHelper->EnableSlPscchRxPhyTraces ();
lteHelper->EnableSlPsschMacTraces ();
lteHelper->EnableSlRxPhyTraces ();
lteHelper->EnableSlPsdchMacTraces ();
lteHelper->EnableDiscoveryMonitoringRrcTraces ();

mcpttHelper.EnableMsgTraces ();
mcpttHelper.EnableStateMachineTraces ();

NS_LOG_INFO ("Starting simulation...");
Simulator::Stop (Seconds (simTime));

AnimationInterface anim ("discovery.xml");
anim.SetMaxPktsPerTraceFile(500000);

Simulator::Run ();
Simulator::Destroy ();
return 0;


}