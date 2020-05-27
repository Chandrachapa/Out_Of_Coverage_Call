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
#include <ns3/netanim-module.h>
#include <ns3/udp-socket-factory.h>
#include <ns3/udp-echo-client.h>

#include <cfloat>
#include <sstream>

using namespace ns3;

// This trace will log packet transmissions and receptions from the application
// layer.  The parameter 'localAddrs' is passed to this trace in case the
// address passed by the trace is not set (i.e., is '0.0.0.0' or '::').  The
// trace writes to a file stream provided by the first argument; by default,
// this trace file is 'UePacketTrace.tr'

 
void
ChangeUdpEchoClientRemote (Ptr<Node> newRemoteNode, Ptr<UdpEchoClient> app, uint16_t port, Ipv6Address network, Ipv6Prefix prefix)
{
  Ptr<Ipv6> ipv6 = newRemoteNode->GetObject<Ipv6> ();
  //Get the interface used for UE-to-Network Relay (LteSlUeNetDevices)
  int32_t ipInterfaceIndex = ipv6->GetInterfaceForPrefix (network, prefix);

  Ipv6Address remoteNodeSlIpAddress = newRemoteNode->GetObject<Ipv6L3Protocol> ()->GetAddress (ipInterfaceIndex,1).GetAddress ();
  // NS_LOG_INFO (" Node id = [" << app->GetNode ()->GetId ()
  //                             << "] changed the UdpEchoClient Remote Ip Address to " << remoteNodeSlIpAddress);
  app->SetRemote (remoteNodeSlIpAddress, port);
}


void
TraceSinkPC5SignalingPacketTrace (Ptr<OutputStreamWrapper> stream, uint32_t srcL2Id, uint32_t dstL2Id, Ptr<Packet> p)
{
  LteSlPc5SignallingMessageType lpc5smt;
  p->PeekHeader (lpc5smt);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << srcL2Id << "\t" << dstL2Id << "\t" << lpc5smt.GetMessageName () << std::endl;
}


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
  else
   if (Inet6SocketAddress::IsMatchingType (srcAddrs))
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


void StartFlow (Ptr<Socket> localSocket,
                Ipv4Address servAddress,
                uint16_t servPort)
 {
  // static uint32_t currentTxBytes = 0;
  // NS_LOG_INFO ("Starting flow at time " <<  Simulator::Now ().GetSeconds ());
  // currentTxBytes = 0;
  localSocket->Bind ();
  localSocket->Connect (InetSocketAddress (servAddress, servPort)); //connect

  // tell the tcp implementation to call WriteUntilBufferFull again
  // if we blocked and new tx buffer space becomes available
 
}

/*
 * The topology is the following:
 *
 *          UE1..........(20 m)..........UE2
 *   (0.0, 0.0, 1.5)            (20.0, 0.0, 1.5)
 *
 * Please refer to the Sidelink section of the LTE user documentation for more details.
 *
 */

NS_LOG_COMPONENT_DEFINE ("LteSlOutOfCovrg");

int main (int argc, char *argv[])
{
  Time simTime = Seconds (6);
  bool enableNsLogs = false;
  bool useIPv6 = true;

  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("enableNsLogs", "Enable ns-3 logging (debug builds)", enableNsLogs);
  cmd.AddValue ("useIPv6", "Use IPv6 instead of IPv4", useIPv6);
  cmd.Parse (argc, argv);

    /* Synchronization*/
  //Configure synchronization protocol
  Config::SetDefault ("ns3::LteUePhy::UeRandomInitialSubframeIndication", BooleanValue (true));
  Config::SetDefault ("ns3::LteUeRrc::UeSlssTransmissionEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteUePhy::UeSlssInterScanningPeriodMax", TimeValue (MilliSeconds (5000)));
  Config::SetDefault ("ns3::LteUePhy::UeSlssInterScanningPeriodMin", TimeValue (MilliSeconds (5000)));
  /* END Synchronization*/


  //Configure the UE for UE_SELECTED scenario
  Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (16));
  Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (5)); //The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1));
  Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (true)); //use default Trp index of 0

  //Set the frequency
  uint32_t ulEarfcn = 18100;
  uint16_t ulBandwidth = 50;

  // Set error models
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (false));

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  // parse again so we can override input file default values via command line
  cmd.Parse (argc, argv);

  if (enableNsLogs)
    {
      LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);

      LogComponentEnable ("LteUeRrc", logLevel);
      LogComponentEnable ("LteUeMac", logLevel);
      LogComponentEnable ("LteSpectrumPhy", logLevel);
      LogComponentEnable ("LteUePhy", logLevel);
    }

  //Set the UEs power in dBm
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));

  //Sidelink bearers activation time
  Time slBearersActivationTime = Seconds (2.0);

  //Create the helpers
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  //Create and set the EPC helper
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ////Create Sidelink helper and set lteHelper
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);

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
  NS_ABORT_MSG_IF (lossModel == nullptr, "No PathLossModel");
  bool ulFreqOk = uplinkPathlossModel->SetAttributeFailSafe ("Frequency", DoubleValue (ulFreq));
  if (!ulFreqOk)
    {
      NS_LOG_WARN ("UL propagation model does not have a Frequency attribute");
    }

  NS_LOG_INFO ("Deploying UE's...");

  //Create nodes (UEs)
  // NodeContainer ueNodes;
  // ueNodes.Create (3);
  // NS_LOG_INFO ("UE 1 node id = [" << ueNodes.Get (0)->GetId () << "]");
  // NS_LOG_INFO ("UE 2 node id = [" << ueNodes.Get (1)->GetId () << "]");

  // //Position of the nodes
  // Ptr<ListPositionAllocator> positionAllocUe1 = CreateObject<ListPositionAllocator> ();
  // positionAllocUe1->Add (Vector (0.0, 0.0, 1.5));
  // Ptr<ListPositionAllocator> positionAllocUe2 = CreateObject<ListPositionAllocator> ();
  // positionAllocUe2->Add (Vector (20.0, 0.0, 1.5));
  //   Ptr<ListPositionAllocator> positionAllocUe3 = CreateObject<ListPositionAllocator> ();
  // positionAllocUe2->Add (Vector (10.0, 1.0, 1.5));

  // //Install mobility

  // MobilityHelper mobilityUe1;
  // mobilityUe1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // mobilityUe1.SetPositionAllocator (positionAllocUe1);
  // mobilityUe1.Install (ueNodes.Get (0));

  // MobilityHelper mobilityUe2;
  // mobilityUe2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // mobilityUe2.SetPositionAllocator (positionAllocUe2);
  // mobilityUe2.Install (ueNodes.Get (1));
  
  // MobilityHelper mobilityUe3;
  // mobilityUe3.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // mobilityUe3.SetPositionAllocator (positionAllocUe3);
  // mobilityUe3.Install (ueNodes.Get (2));


  NodeContainer ueNodes;
  ueNodes.Create (3);

  NodeContainer remoteUeNodes;
  remoteUeNodes.Create (1);
  
  NodeContainer relayUeNodes;
  relayUeNodes.Create (2);

  NodeContainer allUeNodes = NodeContainer (relayUeNodes,remoteUeNodes);

    //Position of the UE nodes
  Ptr<ListPositionAllocator> positionAllocRelays = CreateObject<ListPositionAllocator> ();
  positionAllocRelays->Add (Vector (10.0, 0.0, 1.5));
  positionAllocRelays->Add (Vector (-10.0, 0.0, 1.5));

  Ptr<ListPositionAllocator> positionAllocRemotes = CreateObject<ListPositionAllocator> ();
  positionAllocRemotes ->Add (Vector (0.0, 17.32, 1.5));

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

  NetDeviceContainer relayUeDevs = lteHelper->InstallUeDevice (relayUeNodes);
  NetDeviceContainer remoteUeDevs = lteHelper->InstallUeDevice (remoteUeNodes);
  NetDeviceContainer allUeDevs = NetDeviceContainer (relayUeDevs, remoteUeDevs);

  NS_LOG_INFO ("UE 1 node id = [" << ueNodes.Get (0)->GetId () << "]");
  NS_LOG_INFO ("UE 2 node id = [" << ueNodes.Get (1)->GetId () << "]");
  NS_LOG_INFO ("UE 3 node id = [" << ueNodes.Get (2)->GetId () << "]");

  //Position of the UE nodes
  Ptr<ListPositionAllocator> positionAllocUes = CreateObject<ListPositionAllocator> ();
  positionAllocUes->Add (Vector (10.0, 0.0, 1.5));
  positionAllocUes->Add (Vector (-10.0, 0.0, 1.5));
  positionAllocUes->Add (Vector (0.0, 17.32, 1.5));

  //Install mobility
  MobilityHelper mobilityUes;
  mobilityUes.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUes.SetPositionAllocator (positionAllocUes);

  for (uint32_t i = 0; i < ueNodes.GetN (); ++i)
    {
      mobilityUes.Install (ueNodes.Get (i));
    }

  //Install LTE UE devices to the nodes
  // NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);

  //Fix the random number stream
  uint16_t randomStream = 1;
  // randomStream += lteHelper->AssignStreams (ueDevs, randomStream);

randomStream += lteHelper->AssignStreams (allUeDevs, randomStream);

  //Sidelink pre-configuration for the UEs
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  ueSidelinkConfiguration->SetSlEnabled (true);
  ueSidelinkConfiguration->SetDiscEnabled (true);

  LteRrcSap::SlPreconfiguration preconfiguration;

  preconfiguration.preconfigGeneral.carrierFreq = ulEarfcn;
  preconfiguration.preconfigGeneral.slBandwidth = ulBandwidth;
  preconfiguration.preconfigComm.nbPools = 1;

  LteSlDiscPreconfigPoolFactory pfactory_disc;
  //discovery
  pfactory_disc.SetDiscCpLen ("NORMAL");
  pfactory_disc.SetDiscPeriod ("rf32");
  pfactory_disc.SetNumRetx (0);
  pfactory_disc.SetNumRepetition (1);

  pfactory_disc.SetDiscPrbNum (2);
  pfactory_disc.SetDiscPrbStart (1);
  pfactory_disc.SetDiscPrbEnd (2);
  pfactory_disc.SetDiscOffset (0);
  pfactory_disc.SetDiscBitmap (0x00001);
  
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

  preconfiguration.preconfigDisc.pools[0] = pfactory_disc.CreatePool ();
  preconfiguration.preconfigDisc.nbPools = 1;

  preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();
  preconfiguration.preconfigComm.nbPools = 1;

   /* Synchronization*/
  //Synchronization parameters
  preconfiguration.preconfigSync.syncOffsetIndicator1 = 18;
  preconfiguration.preconfigSync.syncOffsetIndicator2 = 29;
  preconfiguration.preconfigSync.syncTxThreshOoC = -60; //dBm;
  preconfiguration.preconfigSync.syncRefDiffHyst = 0; //dB;
  preconfiguration.preconfigSync.syncRefMinHyst = 0; //dB;
  preconfiguration.preconfigSync.filterCoefficient = 0;  //k = 4 ==> a = 0.5, k = 0 ==> a = 1 No filter;
  /* END Synchronization*/

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
  lteHelper->InstallSidelinkConfiguration (allUeDevs, ueSidelinkConfiguration);

  /*Synchronization*/
  //Set initial SLSSID and start of the first scanning for all UEs
  uint16_t base_t = 2000; //ms
  for (uint16_t devIt = 0; devIt < allUeDevs.GetN (); devIt++)
    {
      allUeDevs.Get (devIt)->GetObject<LteUeNetDevice> ()->GetRrc ()->SetSlssid (devIt + 10);
      allUeDevs.Get (devIt)->GetObject<LteUeNetDevice> ()->GetPhy ()->SetFirstScanningTime (MilliSeconds (base_t + (devIt * base_t)));
      std::cout << "node ID :"  << allUeDevs.Get(devIt)->GetNode()->GetId() << " IMSI" << allUeDevs.Get (devIt)->GetObject<LteUeNetDevice> ()->GetRrc ()->GetImsi ()
                << " SLSSID " << devIt + 10
                << " First Scan "  << base_t + (devIt * base_t) << " ms" << std::endl;

    }

  
  //Tracing synchronization stuffs
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> streamSyncRef = ascii.CreateFileStream (" nyncRef.txt");
  *streamSyncRef->GetStream () << "Time\tIMSI\tprevSLSSID\tprevRxOffset\tprevFrameNo\tprevSframeNo\tcurrSLSSID\tcurrRxOffset\tcurrFrameNo\tcurrSframeNo" << std::endl;
  Ptr<OutputStreamWrapper> streamSendOfSlss = ascii.CreateFileStream ("TxSlss.txt");
  *streamSendOfSlss->GetStream () << "Time\tIMSI\tSLSSID\ttxOffset\tinCoverage\tFrameNo\tSframeNo" << std::endl;
  for (uint32_t i = 0; i < allUeDevs.GetN (); ++i)
    {
      Ptr<LteUeRrc> ueRrc =  allUeDevs.Get (i)->GetObject<LteUeNetDevice> ()->GetRrc ();
      ueRrc->TraceConnectWithoutContext ("ChangeOfSyncRef", MakeBoundCallback (&NotifyChangeOfSyncRef, streamSyncRef));
      ueRrc->TraceConnectWithoutContext ("SendSLSS", MakeBoundCallback (&NotifySendOfSlss, streamSendOfSlss));

    }

      std::cout << "oki 1" << std ::endl;

  InternetStackHelper internet;
  internet.Install (ueNodes);
  internet.Install (relayUeNodes);
  internet.Install (remoteUeNodes);
  uint32_t groupL2Address = 255;
  Ipv4Address groupAddress4 ("225.0.0.0");     //use multicast address as destination
  Ipv6Address groupAddress6 ("ff0e::1");     //use multicast address as destination
  Address remoteAddress;
  Address localAddress;
  Ptr<LteSlTft> tft;
  
  if (!useIPv6)
    {
      Ipv4InterfaceContainer ueIpIface;
      ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (allUeDevs));

      // set the default gateway for the UE
      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      for (uint32_t u = 0; u < allUeNodes.GetN (); ++u)
        {
          Ptr<Node> ueNode = allUeNodes.Get (u);
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
      ueIpIface = epcHelper->AssignUeIpv6Address (NetDeviceContainer (allUeDevs));

      // set the default gateway for the UE
      Ipv6StaticRoutingHelper ipv6RoutingHelper;
      for (uint32_t u = 0; u < allUeNodes.GetN (); ++u)
        {
          Ptr<Node> ueNode = allUeNodes.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv6StaticRouting> ueStaticRouting = ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
        }
      remoteAddress = Inet6SocketAddress (groupAddress6, 8000);
      localAddress = Inet6SocketAddress (Ipv6Address::GetAny (), 8000);
      tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupAddress6, groupL2Address);
    }

    //Configure UE-to-Network Relay network
  proseHelper->SetIpv6BaseForRelayCommunication ("7777:f00e::", Ipv6Prefix (64));

  
  ///*** Configure applications ***///
  NS_LOG_INFO ("Configuring discovery applications");
   std::map<Ptr<NetDevice>, std::list<uint32_t> > announceApps;
   std::map<Ptr<NetDevice>, std::list<uint32_t> > monitorApps;
   
   uint32_t i = 0;
  
         announceApps[allUeDevs.Get (i)].push_back(i);
  
      for (uint32_t j = 1; j < allUeNodes.GetN (); ++j)
      {
       
            monitorApps[allUeDevs.Get (j)].push_back(j);
       
      }


   for (auto itAnnounceApps : announceApps)
   {
      Ptr<LteUeNetDevice> ueNetDevice = DynamicCast<LteUeNetDevice> (itAnnounceApps.first);
      std::list<uint32_t> apps = itAnnounceApps.second;
      std::cout << "Scheduling " << apps.size () << " announce apps for UE with IMSI = " << ueNetDevice->GetImsi () << std::endl;
      std::list<uint32_t>::iterator itAppList;
      for (auto itAppList : apps)
        {
          std::cout << "Announcing App code = " << itAppList << std::endl;
        }

      Simulator::Schedule (Seconds (2.0), &LteSidelinkHelper::StartDiscoveryApps, proseHelper, ueNetDevice, apps, LteSlUeRrc::Announcing);
    }

  for (auto itMonitorApps : monitorApps)
    {
      Ptr<LteUeNetDevice> ueNetDevice = DynamicCast<LteUeNetDevice> (itMonitorApps.first);
      std::list<uint32_t> apps = itMonitorApps.second;
      std::cout << "Scheduling " << apps.size () << " monitor apps for UE with IMSI = " << ueNetDevice->GetImsi () << std::endl;
      std::list<uint32_t>::iterator itAppList;
      for (auto itAppList : apps)
      {
         std::cout << "Monitoring App code = " << itAppList << std::endl;
      }

      Simulator::Schedule (Seconds (2.0),&LteSidelinkHelper::StartDiscoveryApps, proseHelper, ueNetDevice, apps, LteSlUeRrc::Monitoring);
    }

  // //Set Application in the UEs
  // std::string echoServerNode ("RemoteUE");
  // Ipv6Address echoServerAddr;

 
  //     std::cout << "oki 2" << std ::endl;

  //     // We use a dummy IP address for initial configuration as we don't know the
  //     // IP address of the 'Remote UE (0)' before it connects to the Relay UE
  // echoServerAddr = Ipv6Address::GetOnes ();
    
  //     std::cout << "oki 3" << std ::endl;

  //  for (uint16_t remUeIdx = 0; remUeIdx < remoteUeNodes.GetN (); remUeIdx++)
  //   {std::cout << "oki 4" << std ::endl; 
  //     // if (echoServerNode.compare ("RemoteUE") == 0 && remUeIdx == 0)
  //     //   {std::cout << "oki 5" << std ::endl; 
  //     //     //No own traffic applications for Remote UE (0) as it is the echoServerNode
  //     //     continue;
  //     //   }
         
  //     std::cout << "oki 6" << std ::endl; 
  //     ApplicationContainer serverApps;
  //     ApplicationContainer clientApps;
  //     std::ostringstream oss;
  //     uint16_t echoPortBase = 50000;
  //     uint16_t remUePort = echoPortBase + remUeIdx;
  //     uint32_t echoServerNodeId = 0;
  //     //UdpEchoServer listening in the Remote UE port
  //     UdpEchoServerHelper echoServerHelper (remUePort);
  //     ApplicationContainer singleServerApp;

  //     std::cout << "oki 7" << std ::endl;

  //     Ptr<OutputStreamWrapper> packetOutputStream = ascii.CreateFileStream ("AppPacketTrace.txt");
  //      std::cout << "oki 8" << std ::endl;
  //     *packetOutputStream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;

  //     // if (echoServerNode.compare ("RemoteHost") == 0)
  //     //   {
  //     //     singleServerApp.Add (echoServerHelper.Install (remoteHost));
  //     //     echoServerNodeId = remoteHost->GetId ();
  //     //   }
  //     // else 
  //     if (echoServerNode.compare ("RemoteUE") == 0)
  //       {
  //         singleServerApp.Add (echoServerHelper.Install (remoteUeNodes.Get (0)));
  //         echoServerNodeId = remoteUeNodes.Get (0)->GetId ();

  //       }
  //     singleServerApp.Start (Seconds (1.0));
  //     singleServerApp.Stop (Seconds (simTime));
      
      

  //     //Tracing packets on the UdpEchoServer (S)
  //     oss << "rx\tS\t" << echoServerNodeId;
  //     singleServerApp.Get (0)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
  //     oss.str ("");
  //     oss << "tx\tS\t" << echoServerNodeId;
  //     singleServerApp.Get (0)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
  //     oss.str ("");

  //     serverApps.Add (singleServerApp);

  //     //UdpEchoClient in the Remote UE
  //     UdpEchoClientHelper echoClientHelper (echoServerAddr);
  //     echoClientHelper.SetAttribute ("MaxPackets", UintegerValue (20));
  //     echoClientHelper.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
  //     echoClientHelper.SetAttribute ("PacketSize", UintegerValue (150));
  //     echoClientHelper.SetAttribute ("RemotePort", UintegerValue (remUePort));

  //     ApplicationContainer singleClientApp = echoClientHelper.Install (remoteUeNodes.Get (remUeIdx));
  //     //Start the application 3.0 s after the remote UE started the relay service
  //     //normally it should be enough time to connect
  //     singleClientApp.Start (Seconds (3.0 ) );
  //     //Stop the application after 10.0 s
  //     singleClientApp.Stop (Seconds (3.0 + 10.0));

  //     //Tracing packets on the UdpEchoClient (C)
  //     oss << "tx\tC\t" << remoteUeNodes.Get (remUeIdx)->GetId ();
  //     singleClientApp.Get (0)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
  //     oss.str ("");
  //     oss << "rx\tC\t" << remoteUeNodes.Get (remUeIdx)->GetId ();
  //     singleClientApp.Get (0)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
  //     oss.str ("");

  //     clientApps.Add (singleClientApp);

  //     if (echoServerNode.compare ("RemoteUE") == 0 && remUeIdx != 0)
  //       {
  //         //Schedule the change of the RemoteAddress to 100 ms before the start of the application
  //         //normally 'Remote UE (0)' should be already connected to its Relay UE so we can
  //         //assign its address as RemoteAddress
  //         Simulator::Schedule (Seconds (3.0 +  - 0.100),
  //                              &ChangeUdpEchoClientRemote, remoteUeNodes.Get (0),
  //                              singleClientApp.Get (0)->GetObject<UdpEchoClient> (),
  //                              remUePort,
  //                              proseHelper->GetIpv6NetworkForRelayCommunication (),
  //                              proseHelper->GetIpv6PrefixForRelayCommunication ());
  //       }
  //   }


  OnOffHelper sidelinkClient ("ns3::UdpSocketFactory", remoteAddress);
  sidelinkClient.SetConstantRate (DataRate ("16kb/s"), 200);

  ApplicationContainer clientApps = sidelinkClient.Install (allUeNodes.Get (0));
  //onoff application will send the first packet at :
  //(2.9 (App Start Time) + (1600 (Pkt size in bits) / 16000 (Data rate)) = 3.0 sec
  clientApps.Start (slBearersActivationTime + Seconds (0.9));
  clientApps.Stop (simTime - slBearersActivationTime + Seconds (1.0));

  ApplicationContainer serverApps;
  PacketSinkHelper sidelinkSink ("ns3::UdpSocketFactory", localAddress);
  serverApps = sidelinkSink.Install (allUeNodes.Get (1));
  serverApps.Start (Seconds (2.0));


  //Set Sidelink bearers
  proseHelper->ActivateSidelinkBearer (slBearersActivationTime, allUeDevs, tft);
  ///*** End of application configuration ***///

  // AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("UePacketTrace.tr");

  //Trace file table header
  *stream->GetStream () << "time(sec)\ttx/rx\tNodeID\tIMSI\tPktSize(bytes)\tIP[src]\tIP[dst]" << std::endl;

  std::ostringstream oss;

  if (!useIPv6)
    {
      // Set Tx traces
      for (uint16_t ac = 0; ac < clientApps.GetN (); ac++)
        {
          Ipv4Address localAddrs =  clientApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
          std::cout << "Tx address: " << localAddrs << std::endl;
          oss << "tx\t" << allUeNodes.Get (0)->GetId () << "\t" << allUeNodes.Get (0)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
          clientApps.Get (ac)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
          oss.str ("");
        }

      // Set Rx traces
      for (uint16_t ac = 0; ac < serverApps.GetN (); ac++)
        {
          Ipv4Address localAddrs =  serverApps.Get (ac)->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
          std::cout << "Rx address: " << localAddrs << std::endl;
          oss << "rx\t" << allUeNodes.Get (1)->GetId () << "\t" << allUeNodes.Get (1)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
          serverApps.Get (ac)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
          oss.str ("");
        }
    }
  else
    {
      // Set Tx traces
      for (uint16_t ac = 0; ac < clientApps.GetN (); ac++)
        {
          clientApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->AddMulticastAddress (groupAddress6);
          Ipv6Address localAddrs =  clientApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->GetAddress (1,1).GetAddress ();
          std::cout << "Tx address: " << localAddrs << std::endl;
          oss << "tx\t" << allUeNodes.Get (0)->GetId () << "\t" << allUeNodes.Get (0)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
          clientApps.Get (ac)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
          oss.str ("");
        }

      // Set Rx traces
      for (uint16_t ac = 0; ac < serverApps.GetN (); ac++)
        {
          serverApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->AddMulticastAddress (groupAddress6);
          Ipv6Address localAddrs =  serverApps.Get (ac)->GetNode ()->GetObject<Ipv6L3Protocol> ()->GetAddress (1,1).GetAddress ();
          std::cout << "Rx address: " << localAddrs << std::endl;
          oss << "rx\t" << allUeNodes.Get (1)->GetId () << "\t" << allUeNodes.Get (1)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
          serverApps.Get (ac)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
          oss.str ("");
        }
    }
  
  
  // //Schedule the start of the relay service in each UE with their corresponding
  // //roles and service codes
  // for (uint32_t ryDevIdx = 0; ryDevIdx < relayUeDevs.GetN (); ryDevIdx++)
  //   {
  //     uint32_t serviceCode = relayUeDevs.Get (ryDevIdx)->GetObject<LteUeNetDevice> ()->GetImsi ();

  //     Simulator::Schedule (slBearersActivationTime, &LteSidelinkHelper::StartRelayService, proseHelper, relayUeDevs.Get (ryDevIdx), serviceCode, LteSlUeRrc::ModelA, LteSlUeRrc::RelayUE);
  //     // NS_LOG_INFO ("Relay UE " << ryDevIdx << " node id = [" << relayUeNodes.Get (ryDevIdx)->GetId () << "] provides Service Code " << serviceCode << " and start service at " << startTimeRelay [ryDevIdx] << " s");
  //     //Remote UEs
  //     for (uint32_t rm = 0; rm < 1; ++rm)
  //       {
  //         uint32_t rmDevIdx = ryDevIdx * 1 + rm;
  //         Simulator::Schedule (slBearersActivationTime, &LteSidelinkHelper::StartRelayService, proseHelper, remoteUeDevs.Get (rmDevIdx), serviceCode, LteSlUeRrc::ModelA, LteSlUeRrc::RemoteUE);
  //         // NS_LOG_INFO ("Remote UE " << rmDevIdx << " node id = [" << remoteUeNodes.Get (rmDevIdx)->GetId () << "] interested in Service Code " << serviceCode << " and start service at " << startTimeRemote [rmDevIdx] << " s");
  //       }
  //   }


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

  // First packet as normal (goes via Rtr1)
  // Simulator::Schedule (Seconds (0.1),&StartFlow, srcSocket1, dstaddr, dstport);

  // Ptr<Socket> dst = Socket::CreateSocket(dest_node->GetDevice(1), TypeId::LookupByName("ns3::UdpSocketFactory"));


  // InetSocketAddress addr = InetSocketAddress(localAddress, echoport );
  // dst->Bind(addr);
  // dst->SetRecvCallback(MakeCallback(&BNode::recv_pkg, dest_node.get()));
  
  NS_LOG_INFO ("Enabling Sidelink traces...");

  lteHelper->EnableSlPscchMacTraces ();
  lteHelper->EnableSlPsschMacTraces ();
  lteHelper->EnableSlPsdchMacTraces ();

  lteHelper->EnableSlRxPhyTraces ();
  lteHelper->EnableSlPscchRxPhyTraces ();

  lteHelper->EnableDiscoveryMonitoringRrcTraces ();
  lteHelper->EnablePdcpTraces ();
  NS_LOG_INFO ("Starting simulation...");

  Simulator::Stop (simTime);
  AnimationInterface anim("test_sl.xml");
  anim.SetMaxPktsPerTraceFile(500000);
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;

}
