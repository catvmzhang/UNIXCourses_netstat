#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <getopt.h>
#include "def.h"
#include <arpa/inet.h>
#include <regex.h>

#if(DEBUG)
	#define LOGD(...) printf(__VA_ARGS__)
#else
	#define LOGD(...) 
#endif

struct net **netTable;
int netSize;

int regCompStatus;
regex_t reg;


bool isProcess(char *path){
	DIR *dir;

	dir = opendir(path);
	bool res = (dir==NULL?false:true);
	if(res) closedir(dir);

	return res;	
}

bool isTCPUDPSocket(int inode, int *id){
	bool res = false;

	for(int i=0;i<netSize;i++){
		if(netTable[i]->inode == inode){
			*id = i;
			res = true;
			break;
		}
	}

	return res;
}

void fillProcInfo(struct net *entry, char *pid){
	int pidSize = strlen(pid);
	for(int i=0;i<pidSize;i++){
		entry->pid[i] = pid[i];
	}
	entry->pid[pidSize] = '\0';
		
	char name_path[128], pn_path[128];
	snprintf(pn_path, 128, "/proc/%s/comm",pid);
	snprintf(name_path, 128, "/proc/%s/cmdline",pid);
	
	FILE *f = fopen(name_path,"r");
	if(!f) return;

	char name[256];
	char args[128];
	memset(args, '\0', 128);
	int size = fread(name, sizeof(char), 256, f);
	if(size){
		name[size-1] = '\0';
		sscanf(name,"%*s%[^$]",args);
	}
	fclose(f);
	f = fopen(pn_path,"r");
	size = fread(name, sizeof(char), 256, f);
	name[size-1] = '\0';
	sprintf(name,"%s %s",name,args);
	strcpy(entry->name,name);
	

	LOGD("%s name: %s\n",pid,name);
	fclose(f);
}

void searchfdByProc(char *fd_path, char *pid){
	struct dirent *link;
	char link_path[128];
	DIR *fd_dir;
	fd_dir = opendir(fd_path);

	while( (link = readdir(fd_dir)) ){
		if(strcmp(link->d_name,".")==0 ||strcmp(link->d_name,"..")==0) continue;
		snprintf(link_path, 127, "%s/%s", fd_path, link->d_name);
		
		struct stat linkStat;
		stat(link_path,&linkStat);
		if( (linkStat.st_mode & S_IFMT) == S_IFSOCK){

			int id;
			if( isTCPUDPSocket( (int)linkStat.st_ino, &id) ){
				fillProcInfo(netTable[id], pid);
			}
		}

		memset(link_path, '\0', 128);
	}
}

void searchProc(){
	char *proc_path = "/proc";
	DIR *proc_dir = opendir(proc_path);

	struct dirent *pid_dirent;
	int pidLength = strlen(proc_path) + 30;
	char *pid_fd_path = (char*)malloc(sizeof(char) * pidLength);
	char buffer[256];	

	while( (pid_dirent=readdir(proc_dir)) ){
		if(strcmp(pid_dirent->d_name,".")==0 || strcmp(pid_dirent->d_name,"..")==0) continue;
		snprintf(pid_fd_path, pidLength-1, "%s/%s/fd", proc_path, pid_dirent->d_name);
				
		if( !isProcess(pid_fd_path) ) continue;
		searchfdByProc(pid_fd_path,pid_dirent->d_name);
		
		memset(pid_fd_path, '\0', pidLength);
	}

	free(pid_fd_path);
	closedir(proc_dir);
}

void searchNet(char *path, enum protocol type){
	int lineBufSize = 2048;
	char *lineBuf = (char*)malloc(sizeof(char) * lineBufSize);
	FILE *fp;

	if((fp=fopen(path,"r")) == NULL) return;
	if(fgets(lineBuf, lineBufSize-1, fp) == NULL){
		fclose(fp);
		return;
	}

	
	while(fgets(lineBuf, lineBufSize-1, fp)){
		char localAddr[40],remAddr[40];
		int inode;
		unsigned localPort,remPort;

		sscanf(lineBuf,"%*s %[^:]:%x %[^:]:%x %*s %*s %*s %*s %*s %*s %d %*s ",
				localAddr,&localPort,remAddr,&remPort,&inode);
		LOGD("%s\n",lineBuf);
		LOGD("scanf: %s:%d %s:%d %d (%d)\n",localAddr,localPort,remAddr,remPort,inode,type);
		net_init(&(netTable[netSize]),inode,localAddr,localPort,remAddr,remPort,type);
		LOGD("cpToStruct: (%s:%d) (%s:%d) %d\n\n",netTable[netSize]->localAddr,netTable[netSize]->localPort,netTable[netSize]->remAddr,netTable[netSize]->remPort,netTable[netSize]->inode);
		netSize++;

	}
	
}

bool isMatch(char *name){
	regmatch_t pmatch[1];

	if(regCompStatus!=0) return false;
	int status = regexec(&reg, name, (size_t)1, pmatch, 0);
	if(status!=0) {
		LOGD("regexec error\n");
		return false;
	}
	if(status == REG_NOMATCH) return false;
	else return true;
}

void showTable(bool hasFilter){
	char *protocalName[] = {"tcp","udp","tcp6","udp6"};
	bool setTCPhead = false,setUDPhead = false;	

	LOGD("regCompStatus: %d\n",regCompStatus);

	for(int i=0;i<netSize;i++){
		if(hasFilter){
			if(regCompStatus==0) {
				if(!isMatch(netTable[i]->name)) continue;
			}
			else continue;
		}
		if(!setTCPhead && (netTable[i]->type==tcp||netTable[i]->type==tcp6)){
			if(setUDPhead) printf("\n");
			setTCPhead = true;
			printf("List of TCP connections:\n");
			printf("%-5s %-30s %-30s %s\n","Proto","Local Address","Foreign Address","PID/Program name and arguments");
		}
		if(!setUDPhead && (netTable[i]->type==udp||netTable[i]->type==udp6)){
			if(setTCPhead) printf("\n");
			setUDPhead = true;
			printf("List of UDP connections:\n");
			printf("%-5s %-30s %-30s %s\n","Proto","Local Address","Foreign Address","PID/Program name and arguments");
		}

		char *localIP, *remIP;
		ntop(netTable[i], &localIP, &remIP);
		printf("%-5s %-30s %-30s %s/%s\n",
				protocalName[netTable[i]->type],
				localIP,
				remIP,netTable[i]->pid,netTable[i]->name);	
	}
}

int main(int argc, char **argv)
{

	char *const short_options = "tuf:";
	struct option long_options[] = {
		{"tcp", no_argument, NULL, 't'},
		{"udp", no_argument, NULL, 'u'},
		{"filter", required_argument, NULL, 'f'},
		{0, 0, 0, 0},
	};

	netTable = (struct net**)malloc(sizeof(struct net*)*1024);
	netSize=0;
	bool gettcp=false,getudp=false,hasFilter=false;

	int c;
	while( (c = getopt_long(argc, argv, short_options, long_options ,NULL)) != -1){
		switch(c){
			case 't':
				gettcp = true;
				break;
			case 'u':
				getudp = true;
				break;
			case 'f':
				regCompStatus = regcomp(&reg, optarg, REG_EXTENDED);
				hasFilter = true;
				break;
			default:
				printf("default\n");
				break;
		}
	}

	char *lastArgument = argv[argc-1];
	if(argc>1 && lastArgument[0]!='-'){
		printf("last: %s\n",lastArgument);
		regCompStatus = regcomp(&reg, lastArgument, REG_EXTENDED);
		hasFilter = true;
	}

	if(!gettcp && !getudp){
		gettcp = true;
		getudp = true;
	}
	if(gettcp){
		searchNet("/proc/net/tcp",tcp);
		searchNet("/proc/net/tcp6",tcp6);
	}
	if(getudp){
		searchNet("/proc/net/udp",udp);
		searchNet("/proc/net/udp6",udp6);	
	}

	searchProc();

	for(int i=0;i<netSize;i++){
		LOGD("%s:%d %s:%d %d %d %s (%s) \n",
				netTable[i]->localAddr,netTable[i]->localPort,netTable[i]->remAddr,
				netTable[i]->remPort,netTable[i]->inode,netTable[i]->type,netTable[i]->name,netTable[i]->pid);
	}
	LOGD("\n\n");

	showTable(hasFilter);

	return 0;	
}

