//Stas the Header Should be split to the H FILE
//Header Files
//#define DEBUG_NEW new(__FILE__, __LINE__)
//#define new DEBUG_NEW
#include "nsclient.h"

//Function Declarations need to go to h file


//DNS header structure
//stas: "x: number" format is saying how many bits the field holds. need to go to h file

//Elad: Checks if hostname is valid
int IsValid(char *name)
{
	int i, string_len;
	string_len = strlen(name) - 1;

	for (i = 0; i < string_len; i++)
	{

		if ((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z') || (name[i] >= '0' && name[i] <= '9') || (name[i] == '.') || (name[i] == '-'))
			continue;
		else
			return 0;
	}
	return 1;
}

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


void dnsQuery(unsigned char *host, unsigned char *ip_dns)
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

	dest.sin_addr.s_addr = inet_addr(ip_dns); // stas: change it to argv input instead

	//Set the DNS structure to standard queries
	dns = (struct DNS_HEADER *)&buf; // Stas: check the syntax here.
	//Stas: Fills the DNS QUERY structure.
	init_dns(dns);

	//point to the query portion
	qname = (unsigned char*)&buf[sizeof(struct DNS_HEADER)];

	//this will convert www.google.com to 3www6google3com ;
	string2_dns_format(qname, host);

	/*Stas: This is ugly and unefficient implementation, Should be changed if we have exta time left*/
	qinfo = (struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; 
	qinfo->qtype = htons(1); //we are requesting the ipv4 address
	qinfo->qclass = htons(1); //stas: need to understand that


	
	if (sendto(s, (char*)buf, sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(struct QUESTION), 0, (struct sockaddr*)&dest, sizeof(dest)) == SOCKET_ERROR)
	{
		printf("%d error", WSAGetLastError());//Stas: Elad, please change this to perror if needed.
	}
	i = sizeof(dest);

	//Stas: Need to Set here a 2 seconds time limit. explained here: https://www.lowtek.com/sockets/select.html
	fd_set readfds;
	struct timeval tv;
	//while (1) {

		FD_ZERO(&readfds);
		FD_SET(s, &readfds);

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		int rv = select(s + 1, &readfds, NULL, NULL, &tv);
		if (rv == 1) {

			if (recvfrom(s, (char*)buf, 65536, 0, (struct sockaddr*)&dest, &i) == SOCKET_ERROR)
			{
				printf("Failed. Error Code : %d", WSAGetLastError()); //Stas: Elad, please change this to perror if needed.
			}
		}
		else printf("TIMEOUT\n");

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
				answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len +1)); //Stas: Alawys remmeber to add +1 to lenght, else heap error accurs when freeing memory.s

				for (j = 0; j < ntohs(answers[i].resource->data_len); j++)
					answers[i].rdata[j] = reader[j];

				answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';

				reader = reader + ntohs(answers[i].resource->data_len);

				//free(answers[i].rdata);
			}
			else
			{
				answers[i].rdata = ReadName(reader, buf, &stop);
				reader = reader + stop;

			}
			//
			
			
		}
		
		//print IPs
		for (i = 0; i < ntohs(dns->ans_count); i++)
		{
			if (ntohs(answers[i].resource->type) == 1) //IPv4 address
			{
				long *p;
				p = (long*)answers[i].rdata;
				a.sin_addr.s_addr = (*p); //working without ntohl
				printf("%s", inet_ntoa(a.sin_addr)); // Stas: Elad, please check the correct print format.
			}
			printf("\n");
		}
		
		for (i = 0; i < ntohs(dns->ans_count); i++) {
			if (answers[i].rdata)
				free(answers[i].rdata);
			if (answers[i].name)
				free(answers[i].name);
		}
		

	}
	//Stas: Elad, please check the correct format for printing.
	else if (dns->rcode == 1)
		printf("\nFormat Erorr, try again!");
	else if (dns->rcode == 2)
		printf("\nERROR: SERVER FAILURE");
	else if (dns->rcode == 3)
		printf("\nERROR: NONEXISTENT");
	else if (dns->rcode == 4)
		printf("\nERROR: NOT IMPLEMNTED");
	else if (dns->rcode == 5)
		printf("\nERROR: REFUSED");

	return;
}

unsigned char* ReadName(unsigned char* reader, unsigned char* buffer, int* count)
{
	unsigned char *name;
	unsigned int p = 0, skipped = 0, offset;
	int i, j;

	*count = 1;
	name = (unsigned char*)malloc(257); // Stas: 256 +1, to avoid heap Error for max case.

	name[0] = '\0';

	//read the names in 3www6google3com format
	//Stas: I dont like this imlemetation, need to create an elegant funtion if have time.
	while (*reader != 0)
	{
		if (*reader >= 192)
		{
			offset = (*reader) * 256 + *(reader + 1) - 49152; //49152 = 11000000 00000000 ;)
			reader = buffer + offset - 1;
			skipped = 1; //we have skipped to another location so counting wont go up!
		}
		else
		{
			name[p++] = *reader;
		}

		reader = reader + 1;

		if (skipped == 0) *count = *count + 1; //if we havent skipped to another location then we can count up
	}

	name[p] = '\0'; //string complete
	if (skipped == 1)
	{
		*count = *count + 1; //number of steps we actually moved forward in the packet
	}

	//now convert 3www6google3com0 to www.google.com
	//Stas: I dont like this imlemetation, need to create an elegant funtion if have time.
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

//this will convert www.google.com to 3www6google3com ;
void string2_dns_format(unsigned char* dns, unsigned char* host)
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
			lock++;
		}
	}
	*dns++ = '\0';
}