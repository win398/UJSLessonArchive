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

//导入所需的包
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0

//声明空间和设置日志组件
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");


int 
main (int argc, char *argv[])
{
  //通过verbose这个flag控制UdpEchoClientApplication 和 UdpEchoServerApplication 启用日志记录组件，默认是开
  bool verbose = true;
  uint32_t nCsma = 3;
  //输出信息
  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);
  //如果verbose为false，就不输出log了
  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  nCsma = nCsma == 0 ? 1 : nCsma;

  //创建使用P2P链路链接的2个node
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  //声明csma的节点，也是总线的一部分
  NodeContainer csmaNodes;
  //将之前P2P的NodeContianer的第二个节点添加到CSMA的NodeContainer，
  //以获得CSMA device;这个node将会有两个device
  csmaNodes.Add (p2pNodes.Get (1));
  //创建一个额外节点组成的其余部分CSMA网络，包含3个node
  csmaNodes.Create (nCsma);

  //帮助创建一套 PointToPointNetDevice 对象，设置传送速率和信道延迟
  PointToPointHelper pointToPoint;
  //设置通道传输率为5M
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  //设置传输延迟为2ms
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  //安装P2P网卡设备到P2P网络节点
  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);
  
  //CsmaHelper工作只是就像一个 PointToPointHelper,但它创建和CSMA设备和连接频道
  CsmaHelper csma;
  //数据速率为每秒100比特
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  
  //使用NetDeviceContainer来保存创建的节点
  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  //安装协议栈
  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0));
  stack.Install (csmaNodes);
  
  //分配ipv4地址，从101.1.1.0开始分配，子网掩码为255.255.255.0
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);
  //将IP地址分配给CSMA设备接口
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);
  
  //建立一个服务器
  UdpEchoServerHelper echoServer (9);
  //安装节点，在1s时启动，10s时结束，安装在CSMA网段的最后一个节点上
  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  
  //通过helper拿到节点地址，设置最大包的值为1，时间间隔为1s,包大小1024
  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  
  //安装客户端服务器，2s时开始，10s时结束，安装在P2P网段的第一个节点上
  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  
  //如OSPF一样建立路由表
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  //启用pcap跟踪
  pointToPoint.EnablePcapAll ("second");
  //设置额外的参数
  csma.EnablePcap ("second", csmaDevices.Get (1), true);

  //运行，清理，结束
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
