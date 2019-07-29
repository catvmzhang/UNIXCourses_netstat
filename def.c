#include "def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

void net_init(struct net **self, int inode, char *localAddr, unsigned localPort, char *remAddr, unsigned remPort, enum protocol type){
	(*self) = (struct net*)malloc(sizeof(struct net));

	(*self)->inode = inode;
	strcpy((*self)->localAddr,localAddr);
	strcpy((*self)->remAddr,remAddr);
	(*self)->localPort = localPort;
	(*self)->remPort = remPort;
	(*self)->type = type;

	memset((*self)->pid,'\0',sizeof((*self)->pid));
	memset((*self)->name,'\0',sizeof((*self)->name));

}

void produce_ipv4(char *ip, unsigned port, char **res){
	*res = (char*)malloc(sizeof(char) * INET_ADDRSTRLEN + 8);

	struct in_addr ipPack;
	unsigned ipNumber;

	sscanf(ip, "%x", &ipNumber);
	ipPack.s_addr = ipNumber;
	inet_ntop(AF_INET, &ipPack, *res, INET_ADDRSTRLEN);
	if(port == 0) sprintf(*res,"%s:*", *res);
	else sprintf(*res, "%s:%d", *res, port);
	
}

void produce_ipv6(char *ip, unsigned port, char **res){
	*res = (char*)malloc(sizeof(char) * INET6_ADDRSTRLEN+8);

	unsigned hexIP[4];
	struct in6_addr ipPack;

	sscanf(ip, "%8x%8x%8x%8x", &hexIP[0], &hexIP[1], &hexIP[2], &hexIP[3]);
	for(int i=0;i<4;i++){
		ipPack.s6_addr32[i] = hexIP[i];
	}
	inet_ntop(AF_INET6, &ipPack, *res, INET6_ADDRSTRLEN);
	if(port == 0) sprintf(*res, "%s:*", *res);
	else sprintf(*res, "%s:%d", *res, port);

}

void ntop(struct net *self, char **localIP, char **remIP){
	if( (self)->type==tcp || (self)->type==udp ){	
		produce_ipv4(self->localAddr, self->localPort, localIP);
		produce_ipv4(self->remAddr, self->remPort, remIP);
	
	}else{
		produce_ipv6(self->localAddr, self->localPort, localIP);
		produce_ipv6(self->remAddr, self->remPort, remIP);

	}
}
