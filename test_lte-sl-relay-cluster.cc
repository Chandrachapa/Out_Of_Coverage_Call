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
#include <math.h>
#include "ns3/gnuplot.h"
#include "ns3/netanim-module.h"
#include <ns3/mcptt-helper.h>
#include <ns3/wifi-module.h>


using namespace ns3;

/*
 * Scenario with configurable number of Relay UEs and Remote UEs organized in
 * a cluster topology. The Remote UEs can be configured to be in-coverage or
 * out-of-coverage, and the traffic can be configured to flow either between
 * Remote UEs and a RemoteHost in the Internet, or between Remote UEs.
 *
 * Topology:
 * The 'nRelayUes' Relay UEs are deployed around the eNB uniformly on a
 * circle of radius 'relayRadius' meters.
 * Each Relay UE has a cluster of 'nRemoteUesPerRelay' Remote UEs deployed
 * around itself uniformly on a circle of radius 'remoteRadius' meters.
 *
 * The parameter 'remoteUesOoc' set to true indicates the Remote UEs are
 * out-of-coverage (not attached to the eNodeB and use SL preconfiguration).
 *
 * One-to-one connection:
 * The cluster of Remote UEs around a given Relay UE are interested only in its
 * Relay Service Code and thus will connect only to that Relay UE.
 * The UEs start their relay service sequentially in time. First the Relay UE,
 * then the cluster of Remote UEs associated to that Relay UE (sequentially as
 * well), then the next Relay UE, and so on.
 *
 * Traffic:
 * Each Remote UE sends packets to a given node in the network, which echoes
 * back each packet to the Remote UE, showcasing both upward and downward
 * traffic through the Relay UE.
 * The parameter 'echoServerNode' determines the node towards which the Remote
 * UEs send their traffic: either a Remote Host in the Internet (when set to
 * 'RemoteHost') or the first Remote UE connected to the first Relay UE (when
 * set to 'RemoteUE').
 * Each transmitting Remote UE starts sending traffic 3.00 s after the start
 * of the one-to-one connection procedure with its Relay UE and remain active
 * during 10.0 s. The simulation time is calculated so that the last Remote UE
 * can have its 10.0s of traffic activity.
 *
 * Scenario outputs:
 * - AppPacketTrace.txt: Log of application layer transmitted and received
 *                       packets
 * - topology.plt: gnuplot script to plot the topology of the scenario.
 * - PC5SignalingPacketTrace.txt: Log of the received PC5 signaling messages
 * - DlPdcpStats.txt: PDCP layer statistics for the DL
 * - UlPdcpStats.txt: PDCP layer statistics for the UL
 *
 * Users can generate the topology plot by running:
 * $ gnuplot topology.plt
 * It will generate the file 'topology.png' with a plot of the eNodeB,
 * Relay UEs, and Remote UEs positions.
 * Users can track the one-to-one communication setup procedure between Remote
 * and Relay UEs by checking the corresponding PC5 signaling messages exchange
 * in the file 'PC5SignalingPacketTrace.txt'.
 * Users can verify the traffic flows on the file 'AppPacketTrace.txt' and also
 * can verify which nodes are using the network UL/DL in 'DlPdcpStats.txt' and
 * 'UlPdcpStats.txt'. If the one-to-one connections were successfully
 * established and maintained, only the Relay UEs should be using the UL/DL
 * to transmit and receive the Remote UEs relayed traffic.
 */

NS_LOG_COMPONENT_DEFINE ("lte-sl-relay-cluster");

/*
 * Trace sink function for logging when a packet is transmitted or received
 * at the application layer
 */
void
UePacketTrace (Ptr<OutputStreamWrapper> stream, std::string context, Ptr<const Packet> p, const Address &srcAddrs, const Address &dstAddrs)
{
  std::ostringstream oss;
  stream->GetStream ()->precision (6);

  *stream->GetStream () << Simulator::Now ().GetNanoSeconds () / (double) 1e9 << "\t"
                        << context << "\t"
                        << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 () << ":"
                        << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t"
                        << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 () << ":"
                        << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << "\t"
                        << p->GetSize () << "\t"
                        << std::endl;

}

/**
 * Function that generates a gnuplot script file that can be used to plot the
 * topology of the scenario access network (eNBs, Relay UEs and Remote UEs)
 */
void
GenerateTopologyPlotFile (NodeContainer enbNode, NodeContainer relayUeNodes, NodeContainer remoteUeNodes,
                          double relayRadius, double remoteRadius )
{
  std::string fileNameWithNoExtension = "topology";
  std::string graphicsFileName = fileNameWithNoExtension + ".png";
  std::string gnuplotFileName = fileNameWithNoExtension + ".plt";
  std::string plotTitle = "Topology (Labels = Node IDs)";

  Gnuplot plot (graphicsFileName);
  plot.SetTitle (plotTitle);
  plot.SetTerminal ("png size 1280,1024");
  plot.SetLegend ("X", "Y"); //These are the axis, not the legend
  std::ostringstream plotExtras;
  plotExtras << "set xrange [-" << 1.1 * (relayRadius + remoteRadius) << ":+" << 1.1 * (relayRadius + remoteRadius) << "]" << std::endl;
  plotExtras << "set yrange [-" << 1.1 * (relayRadius + remoteRadius) << ":+" << 1.1 * (relayRadius + remoteRadius) << "]" << std::endl;
  plotExtras << "set linetype 1 pt 3 ps 2 " << std::endl;
  plotExtras << "set linetype 2 lc rgb \"green\" pt 2 ps 2" << std::endl;
  plotExtras << "set linetype 3 pt 1 ps 2" << std::endl;
  plot.AppendExtra (plotExtras.str ());

  //eNB
  Gnuplot2dDataset datasetEnodeB;
  datasetEnodeB.SetTitle ("eNodeB");
  datasetEnodeB.SetStyle (Gnuplot2dDataset::POINTS);

  double x = enbNode.Get (0)->GetObject<MobilityModel> ()->GetPosition ().x;
  double y = enbNode.Get (0)->GetObject<MobilityModel> ()->GetPosition ().y;
  std::ostringstream strForLabel;
  strForLabel << "set label \"" << enbNode.Get (0)->GetId () << "\" at " << x << "," << y << " textcolor rgb \"grey\" center front offset 0,1";
  plot.AppendExtra (strForLabel.str ());
  datasetEnodeB.Add (x, y);
  plot.AddDataset (datasetEnodeB);

  //Relay UEs
  Gnuplot2dDataset datasetRelays;
  datasetRelays.SetTitle ("Relay UEs");
  datasetRelays.SetStyle (Gnuplot2dDataset::POINTS);
  for (uint32_t ry = 0; ry < relayUeNodes.GetN (); ry++)
    {
      double x = relayUeNodes.Get (ry)->GetObject<MobilityModel> ()->GetPosition ().x;
      double y = relayUeNodes.Get (ry)->GetObject<MobilityModel> ()->GetPosition ().y;
      std::ostringstream strForLabel;
      strForLabel << "set label \"" << relayUeNodes.Get (ry)->GetId () << "\" at " << x << "," << y << " textcolor rgb \"grey\" center front offset 0,1";
      plot.AppendExtra (strForLabel.str ());
      datasetRelays.Add (x, y);
    }
  plot.AddDataset (datasetRelays);

  //Remote UEs
  Gnuplot2dDataset datasetRemotes;
  datasetRemotes.SetTitle ("Remote UEs");
  datasetRemotes.SetStyle (Gnuplot2dDataset::POINTS);
  for (uint32_t rm = 0; rm < remoteUeNodes.GetN (); rm++)
    {
      double x = remoteUeNodes.Get (rm)->GetObject<MobilityModel> ()->GetPosition ().x;
      double y = remoteUeNodes.Get (rm)->GetObject<MobilityModel> ()->GetPosition ().y;
      std::ostringstream strForLabel;
      strForLabel << "set label \"" << remoteUeNodes.Get (rm)->GetId () << "\" at " << x << "," << y << " textcolor rgb \"grey\" center front offset 0,1";
      plot.AppendExtra (strForLabel.str ());
      datasetRemotes.Add (x, y);
    }
  plot.AddDataset (datasetRemotes);

  std::ofstream plotFile (gnuplotFileName.c_str ());
  plot.GenerateOutput (plotFile);
  plotFile.close ();
}

/*
 * Trace sink function for logging when PC5 signaling messages are received
 */
void
TraceSinkPC5SignalingPacketTrace (Ptr<OutputStreamWrapper> stream, uint32_t srcL2Id, uint32_t dstL2Id, Ptr<Packet> p)
{
  LteSlPc5SignallingMessageType lpc5smt;
  p->PeekHeader (lpc5smt);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << srcL2Id << "\t" << dstL2Id << "\t" << lpc5smt.GetMessageName () << std::endl;
}

/**
 * Function to update the remote address of a UdpEchoClient application to
 * the UE-to-Network Relay network address of a node.
 * Used when the UdpEchoServer application is installed in a Remote UE.
 * The Remote UE will use an IPv6 address from the UE-to-Network Relay network
 * once it connects to a Relay UE. Thus, the UdpEchoClient applications
 * sending to the UdpEchoServer application in the Remote UE need to update
 * their remote address to this new address to be able to reach it.
 */
void
ChangeUdpEchoClientRemote (Ptr<Node> newRemoteNode, Ptr<UdpEchoClient> app, uint16_t port, Ipv6Address network, Ipv6Prefix prefix)
{
  Ptr<Ipv6> ipv6 = newRemoteNode->GetObject<Ipv6> ();
  //Get the interface used for UE-to-Network Relay (LteSlUeNetDevices)
  int32_t ipInterfaceIndex = ipv6->GetInterfaceForPrefix (network, prefix);

  Ipv6Address remoteNodeSlIpAddress = newRemoteNode->GetObject<Ipv6L3Protocol> ()->GetAddress (ipInterfaceIndex,1).GetAddress ();
  NS_LOG_INFO (" Node id = [" << app->GetNode ()->GetId ()
                              << "] changed the UdpEchoClient Remote Ip Address to " << remoteNodeSlIpAddress);
  app->SetRemote (remoteNodeSlIpAddress, port);
}


int main (int argc, char *argv[])
{
  // relay and remote ue  declaration
  double simTime = 1.0; //s //Simulation time (in seconds) updated automatically based on number of nodes
  double relayRadius = 300.0; //m
  double remoteRadius = 50.0; //m
  uint32_t nRelayUes = 2;
  uint32_t nRemoteUesPerRelay = 5;
  bool remoteUesOoc = true;
  std::string echoServerNode ("RemoteUE");

  CommandLine cmd;

  cmd.AddValue ("relayRadius", "The radius of the circle (with center on the eNB) where the Relay UEs are positioned", relayRadius);
  cmd.AddValue ("remoteRadius", "The radius of the circle (with center on the Relay UE) where the Remote UEs are positioned", remoteRadius);
  cmd.AddValue ("nRelayUes", "Number of Relay UEs in the scenario", nRelayUes);
  cmd.AddValue ("nRemoteUesPerRelay", "Number of Remote UEs per deployed Relay UE", nRemoteUesPerRelay);
  cmd.AddValue ("remoteUesOoc", "The Remote UEs are out-of-coverage", remoteUesOoc);
  cmd.AddValue ("echoServerNode", "The node towards which the Remote UE traffic is directed to (RemoteHost|RemoteUE)", echoServerNode);
  cmd.Parse (argc, argv);


 
 
  if (echoServerNode.compare ("RemoteHost") != 0 && echoServerNode.compare ("RemoteUE") != 0)
    {
      NS_FATAL_ERROR ("Wrong echoServerNode!. Options are (RemoteHost|RemoteUE).");
      return 1;
    }
  if (echoServerNode.compare ("RemoteUE") == 0 && nRelayUes * nRemoteUesPerRelay < 2)
    {
      NS_FATAL_ERROR ("At least 2 Remote UEs are needed when echoServerNode is a RemoteUE !");
      return 1;
    }

  //Calculate the start time of the relay service for Relay UEs and Remote UEs
  
  //Do it sequentially for easy of tractability
  double startTimeRelay [nRelayUes];
  double startTimeRemote [nRelayUes * nRemoteUesPerRelay];
  //The time between Remote UE's start of service
  //Four discovery periods (4 * 0.32 s) to ensure relay selection (measurement report every 4 periods)
  //One discovery period (0.32 s) to avoid some of the congestion for the connection messages
  double timeBetweenRemoteStarts = 4 * 0.32 + 0.32; //s
  // The time between Relay UE's start of service
  // 1.0 s of baseline
  // 0.320 s to ensure sending the 1st discovery message
  // plus the time needed to all Remote UEs to connect to it:
  // (2+2*nRemoteUesPerRelay)*0.04 is the time in the worst case for all connection messages to go through between a Remote UE and a Relay UE:
  // 2 msgs from the Remote UE, each of them in 1 single SL period (0.04 s) and
  // 2 msgs from the Relay UE, which given the SL period RR scheduling, it can take in the worst case up to
  // nRemoteUesPerRelay SL periods to be sent
  // timeBetweenRemoteStarts to ensure sequentiality of activation
  double timeBetweenRelayStarts = 1.0 + nRemoteUesPerRelay * ((2 + 2 * nRemoteUesPerRelay) * 0.04 + timeBetweenRemoteStarts); //s

  for (uint32_t ryIdx = 0; ryIdx < nRelayUes; ryIdx++)
    {
      startTimeRelay [ryIdx] = 2.0 + 0.320 + timeBetweenRelayStarts * ryIdx;

      NS_LOG_INFO ("Relay UE Idx " << ryIdx << " start time " << startTimeRelay [ryIdx] << "s");

      for (uint32_t rm = 0; rm < nRemoteUesPerRelay; ++rm)
        {
          uint32_t rmIdx = ryIdx * nRemoteUesPerRelay + rm;
          startTimeRemote [rmIdx] = startTimeRelay [ryIdx] + timeBetweenRemoteStarts * (rm + 1);
          NS_LOG_INFO ("Remote UE Idx " << rmIdx << " start time " << startTimeRemote [rmIdx] << "s");
        }
    }

  //Calculate simTime based on relay service starts and give 10 s of traffic for the last one
  simTime = startTimeRemote [(nRelayUes * nRemoteUesPerRelay - 1)] + 3.0 + 10.0; //s
  NS_LOG_INFO ("Simulation time = " << simTime << " s");

  NS_LOG_INFO ("Configuring default parameters...");

  //defaults required to set for out of coverage scenario 

  //Configure the UE for UE_SELECTED scenario
  Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (16));
  Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (6)); //The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1));
  Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (false));
  Config::SetDefault ("ns3::LteUeMac::SlScheduler", StringValue ("Random")); //Values include Fixed, Random, MinPrb, MaxCoverage

  //for tracing
  Config::SetDefault ("ns3::McpttMsgStats::CallControl", BooleanValue (true));
  Config::SetDefault ("ns3::McpttMsgStats::FloorControl", BooleanValue (true));
  Config::SetDefault ("ns3::McpttMsgStats::Media", BooleanValue (true));
  Config::SetDefault ("ns3::McpttMsgStats::IncludeMessageContent", BooleanValue (true));
  
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
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlFullDuplexEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::LteUePhy::DownlinkCqiPeriodicity", TimeValue (MilliSeconds (79)));

  //Reduce frequency of CQI report to allow for transmissions
  Config::SetDefault ("ns3::LteUePhy::DownlinkCqiPeriodicity", TimeValue (MilliSeconds (79)));

  //Increase SRS periodicity to allow larger number of UEs in the system
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));


  //Set the eNBs power in dBm
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46.0));
  
  // Instantiating LTE, EPC and Sidelink helpers and connecting them
  //Create the lte helper
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  //Create and set the EPC helper
  Ptr<NoBackhaulEpcHelper> epcHelper = CreateObject<NoBackhaulEpcHelper> ();
  // Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  
  //core network
  // Ptr<Node> pgw = epcHelper->GetPgwNode ();

  //setup sidelink 

  //Set the UEs power in dBm
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));

  //Create helper and set lteHelper
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  //Connect controller and Helper
  Config::SetDefault ("ns3::LteSlBasicUeController::ProseHelper", PointerValue (proseHelper));

  //Set pathloss model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Hybrid3gppPropagationLossModel"));

  // channel model initialization
lteHelper->Initialize ();
// // //Set the frequency
uint32_t ulEarfcn = 18300;
// uint16_t ulBandwidth = 50;

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

  

  //Enable Sidelink
  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));

/*****************************************/
  //setup physical layer
  //create nodes and specify location and mobility
  
  //Create a single RemoteHost
  // NodeContainer remoteHostContainer;
  // remoteHostContainer.Create (1);
  
  // std::cout << "remote host id :" <<remoteHostContainer.Get(0)->GetId() << std::endl;

  // Ptr<Node> remoteHost = remoteHostContainer.Get (0);
 
  // internet.Install (remoteHostContainer);

  // // Create the Internet
  // PointToPointHelper p2ph;
  // p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  // p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  // p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  

  //Create nodes (eNb + UEs)
  NodeContainer enbNode;
  enbNode.Create (1);
  NS_LOG_INFO ("eNb node id = [" << enbNode.Get (0)->GetId () << "]");
  std::cout << "gnb id :" << enbNode.Get(0)->GetId() << std::endl;

  NodeContainer relayUeNodes;
  relayUeNodes.Create (nRelayUes);

  std::cout << "relay 1 id :" << relayUeNodes.Get(0)->GetId() << std::endl;
  std::cout << "relay 2 id :" << relayUeNodes.Get(1)->GetId() << std::endl;


  NodeContainer remoteUeNodes;
  remoteUeNodes.Create (nRelayUes * nRemoteUesPerRelay);
  std::cout << "remote 1 id :" << remoteUeNodes.Get(0)->GetId() << std::endl;
  std::cout << "remote 2 id :" << remoteUeNodes.Get(1)->GetId() << std::endl;

  Ptr<Node> relayUe1 = relayUeNodes.Get(0);
  Ptr<Node> relayUe2 = relayUeNodes.Get(1);
  InternetStackHelper internet;
  // NetDeviceContainer internetDevices = p2ph.Install (relayUe1,relayUe2);
  

  for (uint32_t ry = 0; ry < relayUeNodes.GetN (); ry++)
    {
      NS_LOG_INFO ("Relay UE " << ry + 1 << " node id = [" << relayUeNodes.Get (ry)->GetId () << "]");
    }
  for (uint32_t rm = 0; rm < remoteUeNodes.GetN (); rm++)
    {
      NS_LOG_INFO ("Remote UE " << rm + 1 << " node id = [" << remoteUeNodes.Get (rm)->GetId () << "]");
    }
  NodeContainer allUeNodes = NodeContainer (relayUeNodes,remoteUeNodes);

   std::cout << "node 0 id :" << allUeNodes.Get(0)->GetId() << std::endl;
  std::cout << "node 1 id :" <<  allUeNodes.Get(1)->GetId() << std::endl;

   std::cout << "node 2 id :" << allUeNodes.Get(2)->GetId() << std::endl;
  std::cout << "node 3 id :" <<  allUeNodes.Get(3)->GetId() << std::endl;



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

  //Generate gnuplot file with the script to generate the topology plot
  GenerateTopologyPlotFile (enbNode, relayUeNodes, remoteUeNodes, relayRadius, remoteRadius);
   
  std::cout << "ok, 1 " << std::endl; 


  //Install LTE devices to the nodes
  NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNode);
  NetDeviceContainer relayUeDevs = lteHelper->InstallUeDevice (relayUeNodes);
  NetDeviceContainer remoteUeDevs; 
  // = lteHelper->InstallUeDevice (remoteUeNodes);
  NetDeviceContainer allUeDevs = NetDeviceContainer (relayUeDevs, remoteUeDevs);
  std::cout << "ok, 2 " << std::endl; 

  //connecting sidelink and gnB
  //Configure eNodeB for Sidelink
  Ptr<LteSlEnbRrc> enbSidelinkConfiguration = CreateObject<LteSlEnbRrc> ();
  enbSidelinkConfiguration->SetSlEnabled (true);
  std::cout << "ok, 3 " << std::endl; 

  //-Configure communication pool
  //resource setup : UE selected
  enbSidelinkConfiguration->SetDefaultPool (proseHelper->GetDefaultSlCommTxResourcesSetupUeSelected ());
  std::cout << "ok, 4 " << std::endl; 


  //-Enable discovery
  enbSidelinkConfiguration->SetDiscEnabled (true);
  std::cout << "ok, 5 " << std::endl; 

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
  
  std::cout << "ok, 6 " << std::endl; 

  pool.ueSelected.poolToAddModList.pools[0].pool =  pDiscFactory.CreatePool ();
  std::cout << "ok, 7 " << std::endl; 

  LteRrcSap::SlCommTxResourcesSetup commpool;

  LteSlResourcePoolFactory pfactory;
  std::cout << "ok, 8 " << std::endl;

  //Control
  pfactory.SetControlPeriod ("sf40");
  pfactory.SetControlBitmap (0x00000000FF); //8 subframes for PSCCH
  pfactory.SetControlOffset (0);
  pfactory.SetControlPrbNum (22);
  pfactory.SetControlPrbStart (0);
  pfactory.SetControlPrbEnd (49);
  
  std::cout << "ok, 9 " << std::endl; 

  //Data
  pfactory.SetDataBitmap (0xFFFFFFFFFF);
  pfactory.SetDataOffset (8); //After 8 subframes of PSCCH
  pfactory.SetDataPrbNum (25);
  pfactory.SetDataPrbStart (0);
  pfactory.SetDataPrbEnd (49);
  std::cout << "ok, 10 " << std::endl; 

  commpool.ueSelected.poolToAddModList.pools[0].pool =  pfactory.CreatePool ();
  pfactory.SetHaveUeSelectedResourceConfig (true);
  std::cout << "ok, 11 " << std::endl; 

  //-Configure UE-to-Network Relay parameters
  //The parameters for UE-to-Network Relay (re)selection are broadcasted in the SIB19 to the UEs attached to the eNB.
  //they are required by the
  // LteUeRrc for SD-RSRP measurement and Relay Selection
  enbSidelinkConfiguration->SetDiscConfigRelay (proseHelper->GetDefaultSib19DiscConfigRelay ());
  std::cout << "ok, 12 " << std::endl; 

  //Install eNodeB configuration on the eNodeB devices
  lteHelper->InstallSidelinkConfiguration (enbDevs, enbSidelinkConfiguration);
   std::cout << "ok, 13 " << std::endl; 


  //Preconfigure UEs for Sidelink
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  ueSidelinkConfiguration->SetSlEnabled (true);
  std::cout << "ok, 14 " << std::endl; 

  //-Configure preconfiguration
  // Relay UEs: Empty configuration
  LteRrcSap::SlPreconfiguration preconfigurationRelay;
  // Remote UEs: Empty configuration if in-coverage
  //             Custom configuration (see below) if out-of-coverage
  LteRrcSap::SlPreconfiguration preconfigurationRemote;
  std::cout << "ok, 15 " << std::endl; 


  if (remoteUesOoc)
     { 
      //  uint32_t carrierFreq_r = 18300;
    // uint8_t slBandwidth_=50;
      //Configure general preconfiguration parameters
      preconfigurationRemote.preconfigGeneral.carrierFreq = enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlEarfcn ();
      preconfigurationRemote.preconfigGeneral.slBandwidth = enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlBandwidth ();
      std::cout << "ok, 16 " << std::endl; 

      //-Configure preconfigured communication pool
      preconfigurationRemote.preconfigComm = proseHelper->GetDefaultSlPreconfigCommPoolList ();
      std::cout << "ok, 17 " << std::endl; 
//        LteRrcSap::SlPreconfigCommPoolList preconfigComm;
//   // preconfigComm.nbPools = 1;
//         std::cout << 1 << std::endl;
//   // LteSlPreconfigPoolFactory preconfCommPoolFactory;
  
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
      std::cout << "ok, 18 " << std::endl; 

      preconfigurationRemote.preconfigDisc.pools[0] = preconfDiscPoolFactory.CreatePool ();
      
      std::cout << "ok, 19 " << std::endl; 


       //Communication
      preconfigurationRemote.preconfigComm.nbPools = 1;
      LteSlPreconfigPoolFactory preconfCommPoolFactory;
      std::cout << "ok, 20 " << std::endl; 

      //-Control
      preconfCommPoolFactory.SetControlPeriod ("sf40");
      preconfCommPoolFactory.SetControlBitmap (0x00000000FF); //8 subframes for PSCCH
      preconfCommPoolFactory.SetControlOffset (0);
      preconfCommPoolFactory.SetControlPrbNum (22);
      preconfCommPoolFactory.SetControlPrbStart (0);
      preconfCommPoolFactory.SetControlPrbEnd (49);
      
      std::cout << "ok, 21 " << std::endl; 

      //-Data
      preconfCommPoolFactory.SetDataBitmap (0xFFFFFFFFFF);
      preconfCommPoolFactory.SetDataOffset (8); //After 8 subframes of PSCCH
      preconfCommPoolFactory.SetDataPrbNum (25);
      preconfCommPoolFactory.SetDataPrbStart (0);
      preconfCommPoolFactory.SetDataPrbEnd (49);

      preconfigurationRemote.preconfigComm.pools[0] = preconfCommPoolFactory.CreatePool ();
      
      std::cout << "ok, 22 " << std::endl; 

      //-Relay UE (re)selection
      preconfigurationRemote.preconfigRelay.haveReselectionInfoOoc = true;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.filterCoefficient = 0;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.minHyst = 0;
      preconfigurationRemote.preconfigRelay.reselectionInfoOoc.qRxLevMin = -125;

      std::cout << "ok, 23 " << std::endl; 

// preconfigurationRemote.preconfigComm.nbPools = 1;
//    std::cout << 1 << std::endl;
//   LteSlPreconfigPoolFactory pfactory;
//    std::cout << 2 << std::endl;

//   //Control
//   pfactory.SetControlPeriod ("sf40");
//   pfactory.SetControlBitmap (0x00000000FF); //8 subframes for PSCCH
//   pfactory.SetControlOffset (0);
//   pfactory.SetControlPrbNum (22);
//   pfactory.SetControlPrbStart (0);
//   pfactory.SetControlPrbEnd (49);

//   //Data
//   pfactory.SetDataBitmap (0xFFFFFFFFFF);
//   pfactory.SetDataOffset (8); //After 8 subframes of PSCCH
//   pfactory.SetDataPrbNum (25);
//   pfactory.SetDataPrbStart (0);
//   pfactory.SetDataPrbEnd (49);
//    std::cout << 3 << std::endl;
//    preconfigComm.pools[0] = pfactory.CreatePool ();
//     std::cout << 4 << std::endl;      


      //-Configure preconfigured discovery pool
    //   preconfigurationRemote.preconfigDisc = proseHelper->GetDefaultSlPreconfigDiscPoolList ();
      //  std::cout << 5 << std::endl;
      //-Configure preconfigured UE-to-Network Relay parameters
    //   preconfigurationRemote.preconfigRelay = proseHelper->GetDefaultSlPreconfigRelay ();
      //  std::cout << 6 << std::endl;

    }


  uint8_t nb = 3;
  ueSidelinkConfiguration->SetDiscTxResources (nb);
  std::cout << "ok, 24 " << std::endl; 
  //-Enable discovery
  ueSidelinkConfiguration->SetDiscEnabled (true);
  std::cout << "ok, 25 " << std::endl; 
  //-Set frequency for discovery messages monitoring
  // ueSidelinkConfiguration->SetDiscInterFreq (enbDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetUlEarfcn ());
  std::cout << "ok, 26 " << std::endl; 
  //Install UE configuration on the UE devices with the corresponding preconfiguration
  ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRelay);
  lteHelper->InstallSidelinkConfiguration (relayUeDevs, ueSidelinkConfiguration);
  std::cout << "ok, 27 " << std::endl; 

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfigurationRemote);
  lteHelper->InstallSidelinkConfiguration (remoteUeDevs, ueSidelinkConfiguration);
  std::cout << "ok, 28 " << std::endl; 

  //NAS layer 
  WifiHelper wifi;
wifi.SetStandard (WIFI_PHY_STANDARD_80211g); //2.4Ghz
wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("ErpOfdmRate54Mbps"));
std::cout << "ok, 29 " << std::endl; 

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
std::cout << "ok, 30 " << std::endl; 

  //Install the IP stack on the UEs and assign network IP addresses
  
  internet.Install (relayUeNodes);
  internet.Install (remoteUeNodes);
  Ipv6InterfaceContainer ueIpIfaceRelays;
  Ipv6InterfaceContainer ueIpIfaceRemotes;
  ueIpIfaceRelays = epcHelper->AssignUeIpv6Address (relayUeDevs);
  ueIpIfaceRemotes = epcHelper->AssignUeIpv6Address (remoteUeDevs);
  std::cout << "ok, 31 " << std::endl; 
  //Set the default gateway for the UEs
  Ipv6StaticRoutingHelper Ipv6RoutingHelper;
  for (uint32_t u = 0; u < allUeNodes.GetN (); ++u)
    {  
      Ptr<Node> ueNode = allUeNodes.Get (u);
      Ptr<Ipv6StaticRouting> ueStaticRouting = Ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
      std::cout << "ok, 32 " << std::endl; 
    }

  //Configure IP for the nodes in the Internet (PGW and RemoteHost)
  Ipv6AddressHelper ipv6h;
  ipv6h.SetBase (Ipv6Address ("6001:db80::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer internetIpIfaces = ipv6h.Assign (allUeDevs);
  internetIpIfaces.SetForwarding (0, true);
  internetIpIfaces.SetDefaultRouteInAllNodes (0);
  std::cout << "ok, 33 " << std::endl; 
  ns3::PacketMetadata::Enable ();
  std::cout << "ok, 34 " << std::endl;  
  //Set route for the Remote Host to join the LTE network nodes
  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  // Ptr<Ipv6StaticRouting> remoteHostStaticRouting = ipv6RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv6> ());
  // remoteHostStaticRouting->AddNetworkRouteTo ("7777:f000::", Ipv6Prefix (60), internetIpIfaces.GetAddress (0, 1), 1, 0);
  std::cout << "ok, 35 " << std::endl; 
  //Configure UE-to-Network Relay network
  proseHelper->SetIpv6BaseForRelayCommunication ("7777:f00e::", Ipv6Prefix (48));

  //Configure route between PGW and UE-to-Network Relay network
  // proseHelper->ConfigurePgwToUeToNetworkRelayRoute (pgw);

  //Attach Relay UEs to the eNB
  lteHelper->Attach (relayUeDevs);
  //If the Remote UEs are not OOC attach them to the eNodeB as well
  if (!remoteUesOoc)
    {
      lteHelper->Attach (remoteUeDevs);
    }

  ///*** Configure applications ***///
  //For each Remote UE, we have a pair (UpdEchoClient, UdpEchoServer)
  //Each Remote UE has an assigned port
  //UdpEchoClient installed in the Remote UE, sending to the echoServerAddr
  //and to the corresponding Remote UE port
  //UdpEchoServer installed in the echoServerNode, listening to the
  //corresponding Remote UE port
  std::cout << "ok, 36 " << std::endl; 
  Ipv6Address echoServerAddr;
  if (echoServerNode.compare ("RelayUE") == 0)
    {
      echoServerAddr = internetIpIfaces.GetAddress (1, 1);
    }
  else if (echoServerNode.compare ("RemoteUE") == 0)
    {
      // We use a dummy IP address for initial configuration as we don't know the
      // IP address of the 'Remote UE (0)' before it connects to the Relay UE
      echoServerAddr = Ipv6Address::GetOnes ();
    }
  
  std::cout << "ok, 37 " << std::endl;

  uint16_t echoPortBase = 50000;
  ApplicationContainer serverApps;
  ApplicationContainer clientApps;
  AsciiTraceHelper ascii;
  std::cout << "ok, 38 " << std::endl;  
  std::ostringstream oss;
  Ptr<OutputStreamWrapper> packetOutputStream = ascii.CreateFileStream ("AppPacketTrace.txt");
  *packetOutputStream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;
  std::cout << "ok, 39 " << std::endl; 
  for (uint16_t remUeIdx = 0; remUeIdx < remoteUeNodes.GetN (); remUeIdx++)
    {
      if (echoServerNode.compare ("RemoteUE") == 0 && remUeIdx == 0)
        {
          //No own traffic applications for Remote UE (0) as it is the echoServerNode
          continue;
        }
      std::cout << "ok, 40 " << std::endl; 
      uint16_t remUePort = echoPortBase + remUeIdx;
      uint32_t echoServerNodeId = 0;
      //UdpEchoServer listening in the Remote UE port
      UdpEchoServerHelper echoServerHelper (remUePort);
      ApplicationContainer singleServerApp;
      std::cout << "ok, 41 " << std::endl; 
      if (echoServerNode.compare ("RelayUE") == 0)
        {
          singleServerApp.Add (echoServerHelper.Install (relayUeNodes));
          echoServerNodeId = relayUeNodes.Get(0)->GetId ();
          std::cout << "ok,42 " << std::endl; 
        }
      else if (echoServerNode.compare ("RemoteUE") == 0)
        {
          singleServerApp.Add (echoServerHelper.Install (remoteUeNodes.Get (0)));
          echoServerNodeId = remoteUeNodes.Get (0)->GetId ();
          std::cout << "ok, 43 " << std::endl;

        }
      std::cout << "ok, 44 " << std::endl; 
      
      singleServerApp.Start (Seconds (1.0));
      singleServerApp.Stop (Seconds (simTime));
      std::cout << "ok, 45 " << std::endl; 

      //Tracing packets on the UdpEchoServer (S)
      oss << "rx\tS\t" << echoServerNodeId;
      singleServerApp.Get (0)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
      std::cout << "ok, 46 " << std::endl; 
      oss.str ("");
      oss << "tx\tS\t" << echoServerNodeId;
      singleServerApp.Get (0)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
      oss.str ("");
      std::cout << "ok, 47 " << std::endl; 
      serverApps.Add (singleServerApp);
      std::cout << "ok, 48 " << std::endl;

      //UdpEchoClient in the Remote UE
      UdpEchoClientHelper echoClientHelper (echoServerAddr);
      echoClientHelper.SetAttribute ("MaxPackets", UintegerValue (20));
      echoClientHelper.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
      echoClientHelper.SetAttribute ("PacketSize", UintegerValue (150));
      echoClientHelper.SetAttribute ("RemotePort", UintegerValue (remUePort));
      std::cout << "ok, 49 " << std::endl; 
      
      ApplicationContainer singleClientApp = echoClientHelper.Install (remoteUeNodes.Get (remUeIdx));
      //Start the application 3.0 s after the remote UE started the relay service
      //normally it should be enough time to connect
      std::cout << "ok, 50 " << std::endl; 
      singleClientApp.Start (Seconds (3.0 + startTimeRemote [remUeIdx]) );
      std::cout << "ok, 51 " << std::endl; 
      //Stop the application after 10.0 s
      singleClientApp.Stop (Seconds (3.0 + startTimeRemote [remUeIdx] + 10.0));
      std::cout << "ok, 52 " << std::endl; 

    /* mcptt app**************************************/ 
    //creating mcptt application for each device 

  McpttHelper mcpttHelper;
  ApplicationContainer mcpttclientApps;
  McpttTimer mcpttTimer;
  std::cout << "ok, 53 " << std::endl; 


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
  uint32_t msgSize = 60; //60 + RTP header = 60 + 12 = 72
  //creating mcptt service on each node
  std::cout << "ok, 54 " << std::endl; 

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
  std::cout << "ok, 55 " << std::endl;

  mcpttclientApps.Add (mcpttHelper.Install (allUeNodes));
  std::cout << "ok, 56 " << std::endl; 

  mcpttclientApps.Start (startTime);
  std::cout << "ok, 57 " << std::endl; 
  mcpttclientApps.Stop (stopTime);
  std::cout << "ok, 58 " << std::endl; 

  //creating a call initiated by the press of push button 
  ObjectFactory callFac;
  callFac.SetTypeId ("ns3::McpttCallMachineGrpBroadcast");
  std::cout << "ok, 59 " << std::endl; 
  ObjectFactory floorFac;
  floorFac.SetTypeId ("ns3::McpttFloorMachineBasic");
  std::cout << "ok, 60 " << std::endl; 


  //creating location to store application info of UEs A, B 
  Ptr<McpttPttApp> ueAPttApp = DynamicCast<McpttPttApp, Application> (mcpttclientApps.Get (0));

  //floor and call machines generate*******************************
  // uint16_t callId = 1;
  ueAPttApp->CreateCall (callFac, floorFac);
  ueAPttApp->SelectLastCall ();
  std::cout << "ok, 61 " << std::endl;

  Ipv4AddressValue grpAddr;
  std::cout << "ok, 62 " << std::endl; 

  Simulator::Schedule (Seconds (2.2), &McpttPttApp::TakePushNotification, ueAPttApp);
  std::cout << "ok, 63 " << std::endl; 

  uint16_t floorPort = McpttPttApp::AllocateNextPortNumber ();
  uint16_t speechPort = McpttPttApp::AllocateNextPortNumber ();
  std::cout << "ok, 64 " << std::endl;

  Ptr<McpttCall> ueACall = ueAPttApp->GetSelectedCall ();
  std::cout << "ok, 65 " << std::endl; 

  Simulator::Schedule (Seconds (2.15), &McpttCall::OpenFloorChan, ueACall, grpAddr.Get (), floorPort);
  std::cout << "ok, 66 " << std::endl; 
  Simulator::Schedule (Seconds (2.15), &McpttCall::OpenFloorChan, ueACall, grpAddr.Get (), speechPort);
  std::cout << "ok, 67 " << std::endl; 
  Simulator::Schedule (Seconds (5.25), &McpttPttApp::ReleaseCall, ueAPttApp);
  std::cout << "ok, 68 " << std::endl;

/** tracing ***************/
  uint32_t mcpttClientAppNodeId = 0;
  
  mcpttClientAppNodeId = remoteUeNodes.Get(0)->GetId ();
        //Tracing packets on the UdpEchoServer (S)
  oss << "rx\tS\t" << mcpttClientAppNodeId;
  mcpttclientApps.Get (0)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
  oss.str ("");
  std::cout << "ok, 69 " << std::endl; 
  oss << "tx\tS\t" << mcpttClientAppNodeId;
  mcpttclientApps.Get (0)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
  oss.str ("");
  std::cout << "ok, 70 " << std::endl; 

  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("b20.tr");
  std::cout << "ok, 71 " << std::endl; 
  wifiPhy.EnableAsciiAll (stream);
  std::cout << "ok, 72 " << std::endl; 
  internet.EnableAsciiIpv4All (stream);
  std::cout << "ok, 73 " << std::endl; 
  *stream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;
  std::cout << "ok, 74 " << std::endl; 
  AsciiTraceHelper ascii_wifi;
  std::cout << "ok, 75 " << std::endl; 
  wifiPhy.EnableAsciiAll (ascii_wifi.CreateFileStream ("wifi-packet-socket.tr"));
  std::cout << "ok, 76 " << std::endl; 
 
  AsciiTraceHelper ascii_r;

  Ptr<OutputStreamWrapper> rtw = ascii_r.CreateFileStream ("routing_table");
  std::cout << "ok, 77 " << std::endl; 
  // AsciiTraceHelper ascii_trace;

  // std::ostringstream oss;
  // Ptr<OutputStreamWrapper> packetOutputStream = ascii_trace.CreateFileStream ("b20_trace.txt");
  // *packetOutputStream->GetStream () << "time(sec)\ttx/rx\tC/S\tNodeID\tIP[src]\tIP[dst]\tPktSize(bytes)" << std::endl;


  NS_LOG_INFO ("Enabling MCPTT traces...");
  mcpttHelper.EnableMsgTraces ();
  mcpttHelper.EnableStateMachineTraces ();
  std::cout << "ok, 78 " << std::endl; 

      //Tracing packets on the UdpEchoClient (C)
      oss << "tx\tC\t" << remoteUeNodes.Get (remUeIdx)->GetId ();

      std::cout << "remote ue id: " << remoteUeNodes.Get (remUeIdx)->GetId () << std::endl;

      singleClientApp.Get (0)->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
      oss.str ("");
      std::cout << "ok, 79 " << std::endl; 
      oss << "rx\tC\t" << remoteUeNodes.Get (remUeIdx)->GetId ();
      singleClientApp.Get (0)->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, packetOutputStream));
      oss.str ("");
      std::cout << "ok, 80 " << std::endl; 
      clientApps.Add (singleClientApp);
      std::cout << "ok, 81 " << std::endl; 

      if (echoServerNode.compare ("RemoteUE") == 0 && remUeIdx != 0)
        {
          //Schedule the change of the RemoteAddress to 100 ms before the start of the application
          //normally 'Remote UE (0)' should be already connected to its Relay UE so we can
          //assign its address as RemoteAddress
          Simulator::Schedule (Seconds (3.0 + startTimeRemote [remUeIdx] - 0.100),
                               &ChangeUdpEchoClientRemote, remoteUeNodes.Get (0),
                               singleClientApp.Get (0)->GetObject<UdpEchoClient> (),
                               remUePort,
                               proseHelper->GetIpv6NetworkForRelayCommunication (),
                               proseHelper->GetIpv6PrefixForRelayCommunication ());
                               std::cout << "ok, 82 " << std::endl; 
        }
    }

  ///*** Configure Relaying ***///

  //Setup dedicated bearer for the Relay UEs
  Ptr<EpcTft> tft = Create<EpcTft> ();
  EpcTft::PacketFilter dlpf;
  std::cout << "ok, 83 " << std::endl; 
  dlpf.localIpv6Address = proseHelper->GetIpv6NetworkForRelayCommunication ();
  std::cout << "ok, 84 " << std::endl; 
  dlpf.localIpv6Prefix = proseHelper->GetIpv6PrefixForRelayCommunication ();
  std::cout << "ok, 85 " << std::endl; 
  tft->Add (dlpf);
  std::cout << "ok, 86 " << std::endl; 
  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
  // lteHelper->ActivateDedicatedEpsBearer (relayUeDevs, bearer, tft);
  std::cout << "ok, 87 " << std::endl; 
  // Ptr<EpcTft> remote_tft = Create<EpcTft> ();
  //     std::cout << "ok, 88 " << std::endl; 
  //     EpcTft::PacketFilter rem_dlpf;
  //     std::cout << "ok, 89 " << std::endl;  
  //     rem_dlpf.localIpv6Address.Set ("7777:f00e::");
  //     std::cout << "ok, 90 " << std::endl; 
  //     rem_dlpf.localIpv6Prefix = Ipv6Prefix (64);
  //     std::cout << "ok, 91 " << std::endl; 
  //     remote_tft->Add (rem_dlpf);
  //     std::cout << "ok, 92 " << std::endl;

  //     EpsBearer remote_bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
  //     lteHelper->ActivateDedicatedEpsBearer (remoteUeDevs.Get (0), remote_bearer, remote_tft);
      std::cout << "ok, 93 " << std::endl; 

  //Schedule the start of the relay service in each UE with their corresponding
  //roles and service codes
  for (uint32_t ryDevIdx = 0; ryDevIdx < relayUeDevs.GetN (); ryDevIdx++)
    {
      uint32_t serviceCode = relayUeDevs.Get (ryDevIdx)->GetObject<LteUeNetDevice> ()->GetImsi ();
      std::cout << "ok, 94 " << std::endl;

      Simulator::Schedule (Seconds (startTimeRelay [ryDevIdx]), &LteSidelinkHelper::StartRelayService, proseHelper, relayUeDevs.Get (ryDevIdx), serviceCode, LteSlUeRrc::ModelA, LteSlUeRrc::RelayUE);
      NS_LOG_INFO ("Relay UE " << ryDevIdx << " node id = [" << relayUeNodes.Get (ryDevIdx)->GetId () << "] provides Service Code " << serviceCode << " and start service at " << startTimeRelay [ryDevIdx] << " s");
      //Remote UEs
      for (uint32_t rm = 0; rm < nRemoteUesPerRelay; ++rm)
        {
         
          uint32_t rmDevIdx = ryDevIdx * nRemoteUesPerRelay + rm;
          std::cout << "ok, 95 " << std::endl; 
          // Simulator::Schedule ((Seconds (startTimeRemote [rmDevIdx])), &LteSidelinkHelper::StartRelayService, proseHelper, remoteUeDevs.Get (rmDevIdx), serviceCode, LteSlUeRrc::ModelA, LteSlUeRrc::RemoteUE);
          std::cout << "ok, 95.5 " << std::endl; 
          NS_LOG_INFO ("Remote UE " << rmDevIdx << " node id = [" << remoteUeNodes.Get (rmDevIdx)->GetId () << "] interested in Service Code " << serviceCode << " and start service at " << startTimeRemote [rmDevIdx] << " s");
          std::cout << "ok, 96 " << std::endl; 
        }
    }
  std::cout << "ok, 97 " << std::endl; 

  //Tracing PC5 signaling messages
  Ptr<OutputStreamWrapper> PC5SignalingPacketTraceStream = ascii.CreateFileStream ("PC5SignalingPacketTrace.txt");
  *PC5SignalingPacketTraceStream->GetStream () << "time(s)\ttxId\tRxId\tmsgType" << std::endl;
  std::cout << "ok, 98 " << std::endl; 

  for (uint32_t ueDevIdx = 0; ueDevIdx < relayUeDevs.GetN (); ueDevIdx++)
    {
      Ptr<LteUeRrc> rrc = relayUeDevs.Get (ueDevIdx)->GetObject<LteUeNetDevice> ()->GetRrc ();
      std::cout << "ok, 99 " << std::endl; 
      PointerValue ptrOne;
      rrc->GetAttribute ("SidelinkConfiguration", ptrOne);
      std::cout << "ok, 100 " << std::endl; 
      Ptr<LteSlUeRrc> slUeRrc = ptrOne.Get<LteSlUeRrc> ();
      std::cout << "ok, 101 " << std::endl; 
      slUeRrc->TraceConnectWithoutContext ("PC5SignalingPacketTrace",
                                           MakeBoundCallback (&TraceSinkPC5SignalingPacketTrace,
                                                              PC5SignalingPacketTraceStream));
                                                              std::cout << "ok, 102 " << std::endl; 
    }
  std::cout << "ok, 103 " << std::endl; 
  for (uint32_t ueDevIdx = 0; ueDevIdx < remoteUeDevs.GetN (); ueDevIdx++)
    {
      std::cout << "ok, 104 " << std::endl; 
      Ptr<LteUeRrc> rrc = remoteUeDevs.Get (ueDevIdx)->GetObject<LteUeNetDevice> ()->GetRrc ();
      std::cout << "ok, 105 " << std::endl; 
      PointerValue ptrOne;
      rrc->GetAttribute ("SidelinkConfiguration", ptrOne);
      std::cout << "ok, 106 " << std::endl; 
      Ptr<LteSlUeRrc> slUeRrc = ptrOne.Get<LteSlUeRrc> ();
      std::cout << "ok, 107 " << std::endl; 

      slUeRrc->TraceConnectWithoutContext ("PC5SignalingPacketTrace",
                                           MakeBoundCallback (&TraceSinkPC5SignalingPacketTrace,
                                                              PC5SignalingPacketTraceStream));
                                                              std::cout << "ok, 108 " << std::endl; 
    }
  
  std::cout << "ok, 109 " << std::endl;  

  std::ostringstream txWithAddresses;
  txWithAddresses << "/NodeList/" << remoteUeDevs.Get (0)->GetNode ()->GetId () << "/ApplicationList/0/TxWithAddresses";
  //Config::ConnectWithoutContext (txWithAddresses.str (), MakeCallback (&SlOoc1Relay1RemoteRegularTestCase::DataPacketSinkTxNode, this));
  std::cout << "ok, 110 " << std::endl; 
  std::ostringstream rxWithAddresses;
  rxWithAddresses << "/NodeList/" << remoteUeDevs.Get (0)->GetNode ()->GetId () << "/ApplicationList/0/RxWithAddresses";
  std::cout << "ok, 111 " << std::endl; 
  Ipv6Address groupAddress ("7777:f000::");
  uint32_t groupL2Address = 0xFF;

  //Set Sidelink bearers
  Ptr<LteSlTft> sltft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupAddress, groupL2Address);
  proseHelper->ActivateSidelinkBearer (Seconds (2.0),allUeDevs, sltft);

  lteHelper->EnablePdcpTraces ();
  lteHelper->EnableSlPscchMacTraces ();
  lteHelper->EnableSlPsschMacTraces ();
  std::cout << "ok, 112 " << std::endl; 
  lteHelper->EnableSlRxPhyTraces ();
  lteHelper->EnableSlPscchRxPhyTraces ();
  std::cout << "ok, 113 " << std::endl;

  NS_LOG_INFO ("Simulation time " << simTime << " s");
  NS_LOG_INFO ("Starting simulation...");
  std::cout << "ok, 114 " << std::endl; 

  AnimationInterface anim("b_lte_sl_relay_cluster.xml");

  anim.SetMaxPktsPerTraceFile(500000); 
  anim.EnablePacketMetadata (true);
  std::cout << "ok, 115 " << std::endl; 

  Simulator::Stop (Seconds (simTime));
  std::cout << "ok, 116 " << std::endl; 
  Simulator::Run ();
  std::cout << "ok, 117" << std::endl; 
  Simulator::Destroy ();
  std::cout << "ok, 118 " << std::endl; 
  return 0;

}
