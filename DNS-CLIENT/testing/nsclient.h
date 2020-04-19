#pragma once
#ifndef NSCLIENT_H


#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "winsock2.h"
#include "windows.h"
#include "stdio.h"
#include "conio.h"
#pragma comment(lib,"ws2_32.lib") //Winsock Library

void dnsQuery(unsigned char *host, unsigned char *ip_dns);
void string2_dns_format(unsigned char*, unsigned char*);
unsigned char* ReadName(unsigned char*, unsigned char*, int*);
int IsValid(char *name);
void init_dns(struct DNS_HEADER *dns);
unsigned char* ReadName(unsigned char* reader, unsigned char* buffer, int* count);


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



struct QUESTION
{
	unsigned short qtype;
	unsigned short qclass;
};


#pragma pack(push, 1) // Stas: not sure where this line came from, need to check what it does?!
struct R_DATA
{
	unsigned short type;
	unsigned short _class;
	unsigned int ttl;
	unsigned short data_len;
};
#pragma pack(pop)

struct RES_RECORD
{
	unsigned char *name;
	struct R_DATA *resource;
	unsigned char *rdata;
};


typedef struct
{
	unsigned char *name;
	struct QUESTION *ques;
} QUERY;


#endif
