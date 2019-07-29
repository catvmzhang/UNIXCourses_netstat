#ifndef _DEF_H_
#define _DEF_H_

enum protocol{
	tcp=0,udp,tcp6,udp6
};

struct net{
	char localAddr[40];
	char remAddr[40];
	int localPort;
	unsigned remPort;
	unsigned inode;
	enum protocol type;

	char pid[16];
	char name[256];

	
};

void net_init(struct net **self, int inode, char *localAddr, unsigned locarPort, char *remAddr, unsigned remPort,enum protocol type);
void ntop(struct net *self, char **localIP, char **remIP); 
#endif
