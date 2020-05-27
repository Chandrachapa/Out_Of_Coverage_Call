#include <cstdlib>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
#include "ns3/netanim-module.h"
//#include "ns3/gtk-config-store.h"
using namespace ns3;
int main (int argc, char *argv[])
{
  //whether to use carrier aggregation
  bool useCa = false;
  uint32_t numEnb = 1;
  uint32_t numUes = 1;
  CommandLine cmd;
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  // Parse again so you can override default values from the command line
  cmd.AddValue ("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.AddValue ("numEnb","the radius of the disc where UEs are placed around an eNB",numEnb);
  cmd.AddValue ("numUes", "how many UEs are attached to each eNB", numUes);
  cmd.Parse (argc, argv);
  if (useCa)
    {
      Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
      Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers",UintegerValue (2));
      Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager",StringValue ("ns3::RrComponentCarrierManager"));
    }
  Ptr < LteHelper > lteHelper = CreateObject < LteHelper > ();
   // Uncomment to enable logging
    //lteHelper->EnableLogComponents ();
  // Create Nodes: eNodeB and UE
  NodeContainer enbNodes;
  NodeContainer ueNodes;
  enbNodes.Create (numEnb);
  ueNodes.Create (numUes);
  // Install Mobility Model
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (enbNodes);
  BuildingsHelper::Install (enbNodes);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ueNodes);
  BuildingsHelper::Install (ueNodes);
  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs;
  // Default scheduler is PF, uncomment to use RR
  //lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");
  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);
  // Attach a UE to a eNB
  lteHelper->Attach (ueDevs, enbDevs.Get (0));
  // Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  lteHelper->ActivateDataRadioBearer (ueDevs, bearer);
  lteHelper->EnableTraces ();
  Simulator::Stop (Seconds (1.05));
  Simulator::Run ();
  AnimationInterface animator ("lte.xml");
  for (uint32_t i = 0; i < enbNodes.GetN (); ++i)
    {
      animator.UpdateNodeDescription (enbNodes.Get (i), "EnodeB" + i);
      animator.UpdateNodeColor (enbNodes.Get (i), 250, 200, 45);
      AnimationInterface::SetConstantPosition (enbNodes.Get (i),(rand () % 100) + 1,(rand () % 100) + 1);
      animator.UpdateNodeSize (i, 10, 10);
    }
  for (uint32_t j = 0; j < ueNodes.GetN (); ++j)
    {
      animator.UpdateNodeDescription (ueNodes.Get (j), "UE" + j);
      animator.UpdateNodeColor (ueNodes.Get (j), 20, 10, 145);
      AnimationInterface::SetConstantPosition (ueNodes.Get (j),(rand () % 100) + 1,(rand () % 100) + 1);
      animator.UpdateNodeSize (enbNodes.GetN () + j, 10, 10);
    }
  Simulator::Destroy ();
  return 0;
}