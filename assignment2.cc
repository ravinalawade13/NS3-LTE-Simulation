#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"

#include <ns3/boolean.h>
 #include <ns3/log.h>
 #include <ns3/lte-amc.h>
 #include <ns3/lte-vendor-specific-parameters.h>
 #include <ns3/math.h>
 #include "ns3/pf-ff-mac-scheduler.h"
 #include <ns3/pointer.h>
 #include <ns3/simulator.h>
  
 #include <cfloat>
 #include <set>

using namespace ns3;

// set LOG level as LOG_LEVEL_DEBUG
NS_LOG_COMPONENT_DEFINE("demo");

int main(int argc, char* argv[]) {
    // Enable logging for the "demo" component at DEBUG level
    LogComponentEnable("demo", LOG_LEVEL_DEBUG);

    uint16_t numNodePairs = 1;
    double simTime = 10.0;
    Time interPacketInterval = MilliSeconds(1);
    // Carrier aggregation is not used
    bool useCa = false;
    // Downlink data flows are not disabled
    bool disableDl = false;
    // Uplink data flows are not disabled
    bool disableUl = false;
    // Data flows between peer UEs are not disabled
    bool disablePl = false;
    // Scheduler type
    std::string scheduler;
    // Speed
    double speed = 0.0;
    // RngRun
    uint32_t RngRun = 10;
    // Full Buffer Flag
    bool fullBufferFlag = true;

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("numNodePairs", "Number of eNodeBs + UE pairs", numNodePairs);
    cmd.AddValue("simTime", "Total duration of the simulation", simTime);
    cmd.AddValue("interPacketInterval", "Inter packet interval", interPacketInterval);
    cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
    cmd.AddValue("disableDl", "Disable downlink data flows", disableDl);
    cmd.AddValue("disableUl", "Disable uplink data flows", disableUl);
    cmd.AddValue("disablePl", "Disable data flows between peer UEs", disablePl);
    cmd.AddValue("scheduler", "Scheduler type (string)", scheduler);
    cmd.AddValue("speed", "Speed of UE walking (double)", speed);
    cmd.AddValue("RngRun", "RngRun (uint32_t)", RngRun);
    cmd.AddValue("fullBufferFlag", "Full Buffer Flag (bool)", fullBufferFlag);
    cmd.Parse(argc, argv);

    // for (int s=0;s<2;s++) {
    //     if(s==0) {
    //         speed = 0.0;
    //     }
    //     else {
    //         speed = 5.0;
    //     }
    // for (int x=0; x<4; x++) {
        
    // RngRun = 10;
    for (uint32_t i = 0; i < 5; i++) {
        RngSeedManager::SetSeed(RngRun);
        RngSeedManager::SetRun(RngRun);

        Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
        Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
        lteHelper->SetEpcHelper(epcHelper);

        //make pgw node
        Ptr<Node> pgw = epcHelper->GetPgwNode();
        //place pgw node at 700,200
        Ptr<ListPositionAllocator> pgwPositionAlloc = CreateObject<ListPositionAllocator>();
        pgwPositionAlloc->Add(Vector(700.0, 200.0, 0.0));
        MobilityHelper pgwMobility;
        pgwMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        pgwMobility.SetPositionAllocator(pgwPositionAlloc);
        pgwMobility.Install(pgw);
        //create a single remote host
        NodeContainer remoteHostContainer;
        remoteHostContainer.Create(1);
        Ptr<Node> remoteHost = remoteHostContainer.Get(0);
        // set position of remote host at 500,500
        Ptr<ListPositionAllocator> remoteHostPositionAlloc = CreateObject<ListPositionAllocator>();
        remoteHostPositionAlloc->Add(Vector(200.0, 500.0, 0.0));
        MobilityHelper remoteHostMobility;
        remoteHostMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        remoteHostMobility.SetPositionAllocator(remoteHostPositionAlloc);
        remoteHostMobility.Install(remoteHostContainer);
        InternetStackHelper internet;
        internet.Install(remoteHostContainer);

        //connect remote host to pgw with point to point link of 1Gbps and 500ns delay
        PointToPointHelper p2ph;
        p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
        p2ph.SetChannelAttribute("Delay", TimeValue(NanoSeconds(500)));
        NetDeviceContainer pgwUeDevices;
        pgwUeDevices = p2ph.Install(pgw, remoteHost);

        //create a network of 4 enb nodes
        // LTE FDD with 50 RBs for UL and 50 RBs for DL
        NodeContainer enbNodes;
        enbNodes.Create(4); // 4 eNodeBs
        //position enb nodes at vertices of a square of 1km x 1km
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
        positionAlloc->Add(Vector(0.0, 0.0, 0.0));
        positionAlloc->Add(Vector(1000.0, 0.0, 0.0));
        positionAlloc->Add(Vector(1000.0, 1000.0, 0.0));
        positionAlloc->Add(Vector(0.0, 1000.0, 0.0));
        // Install ConstantPositionMobilityModel for eNodeBs
        MobilityHelper enbMobility;
        enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        enbMobility.SetPositionAllocator(positionAlloc);
        enbMobility.Install(enbNodes);
        // enb Tx power is 30dBm
        Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(30));

        // Create UE nodes and assign them to eNodeBs
        NodeContainer ueNodes;
        ueNodes.Create(20); // 5 UE nodes per eNodeB

        for (uint32_t i = 0; i < enbNodes.GetN(); ++i) {
            // Install mobility model for UE nodes
            MobilityHelper ueMobility;
            Ptr<Node> enb = enbNodes.Get(i);
            // get enb Position
            Vector enbPosition = enb->GetObject<MobilityModel>()->GetPosition();
            // print enb position
            NS_LOG_DEBUG("eNodeB " << i << " position: " << enbPosition);
            // set initial position of UE in a square of 1km x 1km with center as enbPosition
            ueMobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                    "rho", DoubleValue(500.0),
                                    "X", DoubleValue(enbPosition.x),
                                    "Y", DoubleValue(enbPosition.y));

            for (uint32_t j = 0; j < 5; ++j) {
                Ptr<Node> ue = ueNodes.Get(i * 5 + j);
                // print ue node number 
                NS_LOG_DEBUG("UE " << i * 5 + j);
                // assign speed to variable SpeedModel for UE
                if (speed == 0.0) {
                    ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
                } else {
                std::ostringstream oss;
                oss << "ns3::ConstantRandomVariable[Constant=" << speed << "]";
                ueMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                            "Bounds", RectangleValue(Rectangle(enbPosition.x - 500.0, enbPosition.x + 500.0, enbPosition.y - 500.0, enbPosition.y + 500.0)),
                                            "Speed", StringValue(oss.str()));
                }

                ueMobility.Install(ue);
                // print ue position
                NS_LOG_DEBUG("UE " << i * 5 + j << " position: " << ue->GetObject<MobilityModel>()->GetPosition());
            }

        }

        //set scheduler according to command line argument Proportional Fair (PF), Round Robin (RR), Max Throughput (MT) and Blind Average Throughput Schedulers (BATS)
        if(scheduler == "RR") {
            Config::SetDefault("ns3::LteHelper::Scheduler", StringValue("ns3::RrFfMacScheduler"));
            NS_LOG_DEBUG("Scheduler: Round Robin");
        } else if(scheduler == "PF") {
            Config::SetDefault("ns3::LteHelper::Scheduler", StringValue("ns3::PfFfMacScheduler"));
            NS_LOG_DEBUG("Scheduler: Proportional Fair");
        } else if(scheduler == "MT") {
            Config::SetDefault("ns3::LteHelper::Scheduler", StringValue("ns3::FdMtFfMacScheduler"));
            NS_LOG_DEBUG("Scheduler: Max Throughput");
        } else if(scheduler == "BATS") {
            Config::SetDefault("ns3::LteHelper::Scheduler", StringValue("ns3::FdBetFfMacScheduler"));
            NS_LOG_DEBUG("Scheduler: Blind Average Throughput");
        } else {
            NS_LOG_ERROR("Invalid scheduler type");
            // using default scheduler
        }

        // //set scheduler according to command line argument Proportional Fair (PF), Round Robin (RR), Max Throughput (MT) and Blind Average Throughput Schedulers (BATS)
        // if(x == 0) {
        //     scheduler = "RR";
        //     Config::SetDefault("ns3::LteHelper::Scheduler", StringValue("ns3::RrFfMacScheduler"));
        //     NS_LOG_DEBUG("Scheduler: Round Robin");
        // } else if(x == 1) {
        //     scheduler = "PF";
        //     Config::SetDefault("ns3::LteHelper::Scheduler", StringValue("ns3::PfFfMacScheduler"));
        //     NS_LOG_DEBUG("Scheduler: Proportional Fair");
        // } else if(x == 2 ) {
        //     scheduler = "MT";
        //     Config::SetDefault("ns3::LteHelper::Scheduler", StringValue("ns3::FdMtFfMacScheduler"));
        //     NS_LOG_DEBUG("Scheduler: Max Throughput");
        // } else if(x ==3) {
        //     scheduler = "BATS";
        //     Config::SetDefault("ns3::LteHelper::Scheduler", StringValue("ns3::FdBetFfMacScheduler"));
        //     NS_LOG_DEBUG("Scheduler: Blind Average Throughput");
        // } else {
        //     NS_LOG_ERROR("Invalid scheduler type");
        //     // using default scheduler
        // }

        // 50 RBs for DL and 50 RBs for UL
        Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(50));
        Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(50));

        // Install LTE devices
        NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
        NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);
        
        //install internet stack on pgw node
        internet.Install(pgw);
        //assign ip addresses
        Ipv4AddressHelper ipv4;
        Ipv4StaticRoutingHelper ipv4RoutingHelper;
        ipv4.SetBase("10.0.0.0", "255.0.0.0");
        Ipv4InterfaceContainer pgwIpIfaces = ipv4.Assign(pgwUeDevices);
        Ipv4Address remoteHostAddr = pgwIpIfaces.GetAddress(1);
        Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
        remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
        //assign IP address to UEs
        internet.Install(ueNodes);
        Ipv4InterfaceContainer ueIpIface;
        ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));
        // Set the default gateway for the UE
        for (uint32_t u = 0; u < ueNodes.GetN(); ++u) {
            Ptr<Node> ueNode = ueNodes.Get(u);
            Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
            ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
        }

        // Attach UEs to the eNodeBs based on received signal strength from each eNodeB
        // Automatically attach UEs to the eNodeB with the highest RSRP
        lteHelper->Attach(ueLteDevs);

        //install applications
        // udp traffic in full buffer case from UE 1500B per 1ms i.e UE uplink is 12Mbps and UE downlink is 12Mbps
        // udp traffic in non full buffer case from UE 1500B per 10ms i.e UE uplink is 1.2Mbps and UE downlink is 1.2Mbps
        uint16_t dlPort = 1234;
        uint16_t ulPort = 2222;
        uint16_t plPort = 3000;
        ApplicationContainer clientApps;
        ApplicationContainer serverApps;
        
        // UDP application settings
        uint32_t packetSize = 1500; // bytes
        uint32_t maxPacketCount = 1000000; // maximum number of packets
        // DataRate dlDataRate;
        if (fullBufferFlag) {
            NS_LOG_DEBUG("Full Buffer Flag is true");
            interPacketInterval = MilliSeconds(1);
        } else {
            NS_LOG_DEBUG("Full Buffer Flag is false");
            interPacketInterval = MilliSeconds(10);
        }

        // Create UDP client and server for each UE
        for (uint32_t u = 0; u < ueNodes.GetN(); ++u) {
            // UE IP address
            Ipv4Address ueIpAddr = ueIpIface.GetAddress(u);

            // UDP client for downlink traffic
            UdpClientHelper dlClientHelper(ueIpAddr, dlPort);
            dlClientHelper.SetAttribute("Interval", TimeValue(interPacketInterval));
            dlClientHelper.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
            // set packet size for UDP client  
            dlClientHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
            clientApps.Add(dlClientHelper.Install(remoteHost));
            
            // UDP server for downlink traffic
            UdpServerHelper dlServerHelper(dlPort);
            serverApps.Add(dlServerHelper.Install(ueNodes.Get(u)));

        }
        
        lteHelper->EnableRlcTraces();

        serverApps.Start(Seconds(0.0));
        clientApps.Start(Seconds(0.0));
        
        Simulator::Stop(Seconds(simTime));

        // get SINR graph
        // Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper>();
        // remHelper->SetAttribute("Channel", PointerValue(lteHelper->GetDownlinkSpectrumChannel()));
        // remHelper->SetAttribute("OutputFile", StringValue("rem.out"));
        // remHelper->SetAttribute("XMin", DoubleValue(-2000.0));
        // remHelper->SetAttribute("XMax", DoubleValue(2000.0));
        // remHelper->SetAttribute("XRes", UintegerValue(100));
        // remHelper->SetAttribute("YMin", DoubleValue(-2000.0));
        // remHelper->SetAttribute("YMax", DoubleValue(2000.0));
        // remHelper->SetAttribute("YRes", UintegerValue(75));
        // remHelper->SetAttribute("Z", DoubleValue(0.0));
        // remHelper->SetAttribute("UseDataChannel", BooleanValue(true));
        // remHelper->SetAttribute("RbId", IntegerValue(10));
        // remHelper->Install();

        Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
        
        // file name should be <sceduler name>-<UE speed in int>-<RngRun>.txt
        rlcStats->SetDlOutputFilename("DlRlcStats-" + scheduler + "-" + std::to_string((int)speed) + "-" + std::to_string(RngRun) + ".csv");
        NS_LOG_DEBUG(rlcStats);
        NS_LOG_DEBUG("File name: " << "DlRlcStats-" + scheduler + "-" + std::to_string((int)speed) + "-" + std::to_string(RngRun) + ".csv");
        Simulator::Run();
        serverApps.Stop(Seconds(simTime));
        clientApps.Stop(Seconds(simTime));
        
        RngRun++;
        Simulator::Destroy();
    // }
    // }
}
    

    return 0;
}