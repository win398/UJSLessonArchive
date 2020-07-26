/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

//声明了一个叫SecondScriptExample的日志构件,可以实现打开或者关闭控制台日志的输出
NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int 
main (int argc, char *argv[])
{
  //决定是否开启两个UdpApplication的Logging组件
  bool verbose = true;
  uint32_t nCsma = 3;
  uint32_t nWifi = 3;
  bool tracing = false;
  //打印信息
  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  // The underlying restriction of 18 is due to the grid position
  // allocator's configuration; the grid layout will exceed the
  // bounding box if more than 18 nodes are provided.
  if (nWifi > 18)
    {
      std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  //创建使用P2P链路链接的2个节点
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  //设置传送速率和信道延迟,传输速率5Mbps,延迟2ms
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  //安装P2P网卡设备到P2P网络节点
  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  //创建NodeContainer类对象，用于总线(CSMA)网络
  NodeContainer csmaNodes;
  //将第二个P2P节点添加到CSMA的NodeContainer
  csmaNodes.Add (p2pNodes.Get (1));
  //创建Bus network上另外3个node
  csmaNodes.Create (nCsma);

  //创建和设置CSMA设备及信道，通信速率是100M，延迟6560s
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  
  //安装网卡设备到CSMA信道的网络节点
  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  //创建NodeContainer类对象，用于WiFi网络
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  //设置WiFi网络的第一个节点为AP
  NodeContainer wifiApNode = p2pNodes.Get (0);

  //初始化物理信道,在物理部分设置虚拟信道部分
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  //YansWifiPhyHelper共享相同的底层信道,也就是说,它们共享相同的无线介质,可以相互通信
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  //SetRemoteStationManager的方法告诉助手使用何值速率控制算法
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  //配置MAC类型和基础设施网络的SSID。先创建IEEE802.11的SSID对象，
  //用来设置MAC层的“SSID”属性值。助手创建的特定种类MAC层被“ns3::StaWifiMac”类型属性所指定。
  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  //安装网卡设备到WiFi信道的网络节点，并配置参数，在MAC层和PHY层可以调用方法来安装这些站的无线设备
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  //配置AP节点
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  //创建单一AP共享相同的PHY层属性
  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  //加入移动模型。希望STA节点能够移动，而使AP节点固定住
  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  //RandomWalk2dMobilityModel,节点以一个随机的速度在一个随机方向上移动
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  //安装协议栈
  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  //分配IP地址
  //10.1.1.0创建2个点到点设备需要的2个地址
  //10.1.2.0分配地址给CSMA网络
  //10.1.3.0分配地址给STA设备和无线网络的AP

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

  //最右端的节点放置echo服务端程序。
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  //将回显客户端放在最后创建的STA节点上，指向CSMA网络的服务器
  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  //启用路由
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //设置终止时间
  Simulator::Stop (Seconds (10.0));

  //将pcap数据打印出来
  if (tracing == true)
    {
      pointToPoint.EnablePcapAll ("third");
      phy.EnablePcap ("third", apDevices.Get (0));
      csma.EnablePcap ("third", csmaDevices.Get (0), true);
    }

  //运行，结束
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
