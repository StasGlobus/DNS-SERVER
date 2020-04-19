#include "nsclient.h"

int main(int argc, char *argv[])
{
	unsigned char hostname[254];
	WSADATA firstsock;

	if (WSAStartup(MAKEWORD(2, 2), &firstsock) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());// Stas: Elad, please change to to perror if needed.
		return 1;
	}
	while (1) {
		printf("\n>");// stas: Elad please check the exact required syntax
		fgets(hostname, 254, stdin);
		//replace th '\n' to '/0' instead
		char *pos;
		if ((pos = strchr(hostname, '\n')) != NULL)
			*pos = '\0';
		//
		if (strcmp(hostname, "quit") == 0)
			break;
		if (IsValid(hostname))
			dnsQuery(hostname, argv[1]);
		else
			printf("ERROR: BAD NAME\n");
	}
	return 0;
}