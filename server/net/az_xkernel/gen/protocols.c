/*
 * protocol initialization calls
 *
 * Warning: this file is generated from graph.comp and is overwritten
 * every time 'make compose' is run
 */

#include "upi.h"
#include "x_util.h"
#include "compose.h"
#include "protocols.h"


XObj		protl_tab[15+1];
static XObj	argv[15];


void
build_pgraph_dev()
{
}


void
build_pgraph()
{

  /* building protocol XKLANCE */
  protl_tab[0] = xCreateProtl(xklance_init, "xklance", "SE0", 0, argv);
  if ( protl_tab[0] == ERR_XOBJ ) {
    Kabort("Could not create xklance protocol");
  }


  /* building protocol ETH */
  argv[0] = protl_tab[0];
  protl_tab[1] = xCreateProtl(eth_init, "eth", "", 1, argv);
  if ( protl_tab[1] == ERR_XOBJ ) {
    Kabort("Could not create eth protocol");
  }


  /* building protocol ARP */
  argv[0] = protl_tab[1];
  protl_tab[2] = xCreateProtl(arp_init, "arp", "", 1, argv);
  if ( protl_tab[2] == ERR_XOBJ ) {
    Kabort("Could not create arp protocol");
  }


  /* building protocol VNET */
  argv[0] = protl_tab[1];
  argv[1] = protl_tab[2];
  protl_tab[3] = xCreateProtl(vnet_init, "vnet", "", 2, argv);
  if ( protl_tab[3] == ERR_XOBJ ) {
    Kabort("Could not create vnet protocol");
  }


  /* building protocol IP */
  argv[0] = protl_tab[3];
  protl_tab[4] = xCreateProtl(ip_init, "ip", "", 1, argv);
  if ( protl_tab[4] == ERR_XOBJ ) {
    Kabort("Could not create ip protocol");
  }


  /* building protocol ICMP */
  argv[0] = protl_tab[4];
  protl_tab[5] = xCreateProtl(icmp_init, "icmp", "", 1, argv);
  if ( protl_tab[5] == ERR_XOBJ ) {
    Kabort("Could not create icmp protocol");
  }


  /* building protocol VMUX */
  argv[0] = protl_tab[3];
  argv[1] = protl_tab[4];
  protl_tab[6] = xCreateProtl(vmux_init, "vmux", "", 2, argv);
  if ( protl_tab[6] == ERR_XOBJ ) {
    Kabort("Could not create vmux protocol");
  }


  /* building protocol BLAST */
  argv[0] = protl_tab[6];
  protl_tab[7] = xCreateProtl(blast_init, "blast", "", 1, argv);
  if ( protl_tab[7] == ERR_XOBJ ) {
    Kabort("Could not create blast protocol");
  }


  /* building protocol VSIZE */
  argv[0] = protl_tab[6];
  argv[1] = protl_tab[7];
  protl_tab[8] = xCreateProtl(vsize_init, "vsize", "", 2, argv);
  if ( protl_tab[8] == ERR_XOBJ ) {
    Kabort("Could not create vsize protocol");
  }


  /* building protocol UDP */
  argv[0] = protl_tab[4];
  protl_tab[9] = xCreateProtl(udp_init, "udp", "", 1, argv);
  if ( protl_tab[9] == ERR_XOBJ ) {
    Kabort("Could not create udp protocol");
  }


  /* building protocol TCP */
  argv[0] = protl_tab[4];
  protl_tab[10] = xCreateProtl(tcp_init, "tcp", "", 1, argv);
  if ( protl_tab[10] == ERR_XOBJ ) {
    Kabort("Could not create tcp protocol");
  }


  /* building protocol BIDCTL */
  argv[0] = protl_tab[6];
  protl_tab[11] = xCreateProtl(bidctl_init, "bidctl", "", 1, argv);
  if ( protl_tab[11] == ERR_XOBJ ) {
    Kabort("Could not create bidctl protocol");
  }


  /* building protocol BID */
  argv[0] = protl_tab[8];
  argv[1] = protl_tab[11];
  protl_tab[12] = xCreateProtl(bid_init, "bid", "", 2, argv);
  if ( protl_tab[12] == ERR_XOBJ ) {
    Kabort("Could not create bid protocol");
  }


  /* building protocol CHAN */
  argv[0] = protl_tab[12];
  argv[1] = protl_tab[11];
  protl_tab[13] = xCreateProtl(chan_init, "chan", "", 2, argv);
  if ( protl_tab[13] == ERR_XOBJ ) {
    Kabort("Could not create chan protocol");
  }


  /* building protocol UDPTEST */
  argv[0] = protl_tab[9];
  protl_tab[14] = xCreateProtl(udptest_init, "udptest", "", 1, argv);
  if ( protl_tab[14] == ERR_XOBJ ) {
    Kabort("Could not create udptest protocol");
  }

}
