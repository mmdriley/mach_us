static MapEntry ethMap[] = { 
	{ "ethtest", 12288 },
	{ "ip", 14592 },
	{ "arp", 2054 },
	{ "rarp", 32821 },
	{ "blast", 12289 },
	{ "vsize", 12290 },
	{ "chan", 12291 },
	{ 0, 0 }
};

static MapEntry ipMap[] = { 
	{ "iptest", 100 },
	{ "udp", 17 },
	{ "tcp", 201 },
	{ "icmp", 1 },
	{ "blast", 101 },
	{ "vsize", 203 },
	{ "chan", 102 },
	{ 0, 0 }
};

static MapEntry udpMap[] = { 
	{ "pmap", 111 },
	{ 0, 0 }
};

static Entry	entries[] = {
	{ "vaddr", 12, 0 },
	{ "rarp", 4, 0 },
	{ "pmap", 10, 0 },
	{ "chantest", 10013, 0 },
	{ "eth", 1, ethMap },
	{ "select", 14, 0 },
	{ "iptest", 10002, 0 },
	{ "vchan", 15, 0 },
	{ "icmp", 7, 0 },
	{ "tcptest", 10006, 0 },
	{ "ethtest", 10001, 0 },
	{ "arp", 3, 0 },
	{ "sunrpctest", 10009, 0 },
	{ "ip", 2, ipMap },
	{ "vsize", 11, 0 },
	{ "chan", 13, 0 },
	{ "xrpctest", 10014, 0 },
	{ "sunrpc", 9, 0 },
	{ "blasttest", 10008, 0 },
	{ "udp", 5, udpMap },
	{ "blast", 8, 0 },
	{ "tcp", 6, 0 },
	{ "udptest", 10005, 0 },
	{ 0, 0, 0 }
};
