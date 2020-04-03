//Stas the Header Should be split to the H FILE
//Header Files
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "winsock2.h"
#include "windows.h"
#include "stdio.h"
#include "conio.h"
#pragma comment(lib,"ws2_32.lib") //Winsock Library


//Function Declarations need to go to h file
void dnsQuery(unsigned char*);
void ChangetoDnsNameFormat(unsigned char*, unsigned char*);
unsigned char* ReadName(unsigned char*, unsigned char*, int*);


//DNS header structure
//stas: "x: number" format is saying how many bits the field holds. need to go to h file
struct DNS_HEADER
{
	unsigned short id; // identification number

	unsigned char rd : 1; // recursion desired
	unsigned char tc : 1; // truncated message
	unsigned char aa : 1; // authoritive answer
	unsigned char opcode : 4; // purpose of message
	unsigned char qr : 1; // query/response flag

	unsigned char rcode : 4; // response code
	unsigned char cd : 1; // checking disabled
	unsigned char ad : 1; // authenticated data
	unsigned char z : 1; // its z! reserved
	unsigned char ra : 1; // recursion available

	unsigned short q_count; // number of question entries
	unsigned short ans_count; // number of answer entries
	unsigned short auth_count; // number of authority entries
	unsigned short add_count; // number of resource entries
};


//Stas: Constant sized fields of query structure, need to go to h file
struct QUESTION
{
	unsigned short qtype;
	unsigned short qclass;
};

//Stas: Constant sized fields of the resource record structure, need to go to h file.
#pragma pack(push, 1) // Stas: not sure where this line came from, need to check what it does?!
struct R_DATA
{
	unsigned short type;
	unsigned short _class;
	unsigned int ttl;
	unsigned short data_len;
};
#pragma pack(pop)

//Stas:Pointers to resource record contents, needs to go to h file.
struct RES_RECORD
{
	unsigned char *name;
	struct R_DATA *resource;
	unsigned char *rdata;
};

//Stas:Structure of a Query, needs to go to h file
typedef struct
{
	unsigned char *name;
	struct QUESTION *ques;
} QUERY;
//Stas: need to move main to separate C file and add argv, argc.
int main() 
{
	unsigned char hostname[100];
	WSADATA firstsock;
	
	if (WSAStartup(MAKEWORD(2, 2), &firstsock) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());// Stas: Elad, please change to to perror if needed.
		return 1;
	}
	while (1) {
		printf("\nEnter Hostname to Lookup : ");// stas: Elad please check the exact required syntax
		gets((char*)hostname); // stas: Elad, please replace gets with better func if have time.
		if (strcmp(hostname, "quit") == 0)
			break;
		//Stas: Elad, please add here input check and return the right error message.
		dnsQuery(hostname); 
		
	}
	return 0;
}
// Stas: Need to add the definition to H file.
void init_dns(struct DNS_HEADER *dns) {

	dns->id = (unsigned short)htons(GetCurrentProcessId());
	dns->qr = 0; //Stas: 0 This is a query
	dns->opcode = 0; //Stas: 0 is a standard query
	dns->aa = 0; //Stas: 0 is Not Authoritative, we can force Authorative by setting 1.. I think, not sure.. anyway, works both ways :)
	dns->tc = 0; //This message is not truncated
	dns->rd = 1; //Stas:Recursion bit, not sure if we are equired to support it, but why not?
	dns->ra = 1; //Stas: Recursion enabled, can be set to zero to disable.
	dns->z = 0; // Stas: must be always set to zero by PA1 instuctions.
	dns->ad = 0; // Stas: not sure for this one, Elad please chekck it.
	dns->cd = 0;// Stas: not sure for this one, Elad please chekck it.
	dns->rcode = 0;
	dns->q_count = htons(1); //Stas: We have only one question
	dns->ans_count = 0;
	dns->auth_count = 0;
	dns->add_count = 0;

}


void dnsQuery(unsigned char *host)
{
	unsigned char buf[65536], *qname, *reader;
	int i, j, stop;

	SOCKET s;
	struct sockaddr_in a;

	struct RES_RECORD answers[20], auth[20], addit[20]; //the replies from the DNS server
	struct sockaddr_in dest;

	struct DNS_HEADER *dns = NULL; //Stas: if have time change to typedef struct so it will look more elegant.
	struct QUESTION *qinfo = NULL;

	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //UDP packet for DNS queries

	//Configure the sockaddress structure with information of DNS server
	dest.sin_family = AF_INET;
	dest.sin_port = htons(53);

	dest.sin_addr.s_addr = inet_addr("192.168.1.1"); // stas: change it to argv input instead

	//Set the DNS structure to standard queries
	dns = (struct DNS_HEADER *)&buf; // Stas: check the syntax here.
	//Stas: Fills the DNS QUERY structure.
	init_dns(dns);
	
	//point to the query portion
	qname = (unsigned char*)&buf[sizeof(struct DNS_HEADER)];
	
	//this will convert www.google.com to 3www6google3com ;
	ChangetoDnsNameFormat(qname, host);

	qinfo = (struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it

	qinfo->qtype = htons(1); //we are requesting the ipv4 address
	qinfo->qclass = htons(1); //stas: need to understand that

	
	//Stas: Need to Set here a 2 seconds time limit.explained here: https://www.lowtek.com/sockets/select.html
	if (sendto(s, (char*)buf, sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(struct QUESTION), 0, (struct sockaddr*)&dest, sizeof(dest)) == SOCKET_ERROR)
	{
		printf("%d error", WSAGetLastError());//Stas: Elad, please change this to perror if needed.
	}
	i = sizeof(dest);
	
	//Stas: Need to Set here a 2 seconds time limit. explained here: https://www.lowtek.com/sockets/select.html
	if (recvfrom(s, (char*)buf, 65536, 0, (struct sockaddr*)&dest, &i) == SOCKET_ERROR)
	{
		printf("Failed. Error Code : %d", WSAGetLastError()); //Stas: Elad, please change this to perror if needed.
	}
	
	dns = (struct DNS_HEADER*)buf;
	if (dns->rcode == 0) {
		//move ahead of the dns header and the query field
		reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(struct QUESTION)]; // stas: move char pointer to the right place (which is?)

		//reading answers
		stop = 0;

		for (i = 0; i < ntohs(dns->ans_count); i++)
		{
			answers[i].name = ReadName(reader, buf, &stop);
			reader = reader + stop;

			answers[i].resource = (struct R_DATA*)(reader);
			reader = reader + sizeof(struct R_DATA);

			if (ntohs(answers[i].resource->type) == 1) //if its an ipv4 address
			{
				answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len));

				for (j = 0; j < ntohs(answers[i].resource->data_len); j++)
					answers[i].rdata[j] = reader[j];

				answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';

				reader = reader + ntohs(answers[i].resource->data_len);

			}
			else
			{
				answers[i].rdata = ReadName(reader, buf, &stop);
				reader = reader + stop;
			}

		}
		//print Ips
		for (i = 0; i < ntohs(dns->ans_count); i++)
		{
			if (ntohs(answers[i].resource->type) == 1) //IPv4 address
			{
				long *p;
				p = (long*)answers[i].rdata;
				a.sin_addr.s_addr = (*p); //working without ntohl
				printf("has IPv4 address : %s", inet_ntoa(a.sin_addr)); // Stas: Elad, please check the correct print format.
			}
			printf("\n");
		}

		
	}
	else if( dns->rcode == 1)
	printf("\nFormat Erorr, try again!");
	else if (dns->rcode == 2)
	printf("\nName Server Erorr, try again!");
	else if (dns->rcode == 3)
	printf("\nName  Erorr, try again!");
	else if (dns->rcode ==4)
	printf("\nNot Implemented Erorr, try again!");
	else if (dns->rcode == 5)
	printf("\n Refused Erorr, try again!");

	return;
}

unsigned char* ReadName(unsigned char* reader, unsigned char* buffer, int* count)
{
	unsigned char *name;
	unsigned int p = 0, jumped = 0, offset;
	int i, j;

	*count = 1;
	name = (unsigned char*)malloc(256);

	name[0] = '\0';

	//read the names in 3www6google3com format
	while (*reader != 0)
	{
		if (*reader >= 192)
		{
			offset = (*reader) * 256 + *(reader + 1) - 49152; //49152 = 11000000 00000000 ;)
			reader = buffer + offset - 1;
			jumped = 1; //we have jumped to another location so counting wont go up!
		}
		else
		{
			name[p++] = *reader;
		}

		reader = reader + 1;

		if (jumped == 0) *count = *count + 1; //if we havent jumped to another location then we can count up
	}

	name[p] = '\0'; //string complete
	if (jumped == 1)
	{
		*count = *count + 1; //number of steps we actually moved forward in the packet
	}

	//now convert 3www6google3com0 to www.google.com
	for (i = 0; i < (int)strlen((const char*)name); i++)
	{
		p = name[i];
		for (j = 0; j < (int)p; j++)
		{
			name[i] = name[i + 1];
			i = i + 1;
		}
		name[i] = '.';
	}

	name[i - 1] = '\0'; //remove the last dot

	return name;
}

//Retrieve the DNS servers from the registry
/*
void RetrieveDnsServersFromRegistry()
{
	HKEY hkey = 0;
	char name[256];

	char *path = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";

	char *fullpath[256];
	unsigned long s = sizeof(name);
	int dns_count = 0, err, i, j;
	HKEY inter;
	unsigned long count;

	//Open the registry folder
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hkey);

	//how many interfaces
	RegQueryInfoKey(hkey, 0, 0, 0, &count, 0, 0, 0, 0, 0, 0, 0);

	for (i = 0; i < count; i++)
	{
		s = 256;
		//Get the interface subkey name
		RegEnumKeyEx(hkey, i, (char*)name, &s, 0, 0, 0, 0);

		//Make the full path
		strcpy((char*)fullpath, path);
		strcat((char*)fullpath, "\\");
		strcat((char*)fullpath, name);

		//Open the full path name
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, (const char*)fullpath, 0, KEY_READ, &inter);

		//Extract the value in Nameserver field
		s = 256;
		err = RegQueryValueEx(inter, "NameServer", 0, 0, (unsigned char*)name, &s);

		if (err == ERROR_SUCCESS && strlen(name) > 0)
		{
			strcpy(dns_servers[dns_count++], name);
		}
	}

	for (i = 0; i < dns_count; i++)
	{
		for (j = 0; j < strlen(dns_servers[i]); j++)
		{
			if (dns_servers[i][j] == ',' || dns_servers[i][j] == ' ')
			{
				strcpy(dns_servers[dns_count++], dns_servers[i] + j + 1);
				dns_servers[i][j] = 0;
			}
		}
	}

	printf("\nThe following DNS Servers were found on your system...");
	for (i = 0; i < dns_count; i++)
	{
		printf("\n%d) %s", i + 1, dns_servers[i]);
	}
}
*/
//this will convert www.google.com to 3www6google3com ;
void ChangetoDnsNameFormat(unsigned char* dns, unsigned char* host)
{
	int lock = 0, i;

	strcat((char*)host, ".");

	for (i = 0; i < (int)strlen((char*)host); i++)
	{
		if (host[i] == '.')
		{
			*dns++ = i - lock;
			for (; lock < i; lock++)
			{
				*dns++ = host[lock];
			}
			lock++; //or lock=i+1;
		}
	}
	*dns++ = '\0';
//	printf("\ndns: %s host: %s\n", dns, host);
}
