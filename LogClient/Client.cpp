#include "sendrecive1.h"
using namespace MY_SendRecive1;

#include <stdio.h>

#include <errno.h>
//
//  Главный вход
//
int wmain(int argc, wchar_t* argv[])
	{
	CmdLog			CmdLogPk;
	LogPackets		LogPk;

	WSABUF			SendLog[2];
	
	memset(&CmdLogPk,0,sizeof(CmdLog));
	CmdLogPk.Skip.AllSize		= sizeof(CmdLogPk);
	CmdLogPk.Skip.PacketType	= CMD;
	CmdLogPk.Cmd				= CMD_GETLOG;

	memcpy(CmdLogPk.Path,"2017-04-03",10);

	//CmdLogPk.Path[0]			=0;

	SendLog[0].buf = (char *)&CmdLogPk;
	SendLog[0].len = sizeof(CmdLogPk);

	SendLog[1].buf = nullptr;
	SendLog[1].len = 0;

	int ret;

	WSADATA WsaData;
	if(WSAStartup( MAKEWORD(2,2), &WsaData )!=0)
		{
		ret=WSAGetLastError();

		WSACleanup();
		return -1;
		}

	// Создадим обьект для работы с сокетом
	SendRecive1 *Cl = new SendRecive1();

	ret=Cl->CreateSocket();

	FILE *Ms=fopen("MediaServer.log","w+");
	if(Ms==NULL) 
			return 0;

	ret=Cl->Connect("192.168.12.187", 8881);

	Cl->Send(SendLog);

	unsigned char *data;
	int zz;

	while (1)
		{
		ret=Cl->Recive(0);

		if (ret != 0)
			{
			return false;
			}

		data = Cl->GetReciveBuff();

		// Возьмем порцию лога
		memcpy(&LogPk.Skip, data, sizeof(Packets));

		if (LogPk.Skip.PacketType == END)
			{
			fclose(Ms);
			return 0;
			}

		if (LogPk.Skip.PacketType == ERRORS)
			{
			fclose(Ms);
			return 0;
			}


		memcpy(&LogPk, data, sizeof(LogPackets));

		int xx=LogPk.Skip.AllSize-sizeof(Packets);

		int ss=fwrite(&((LogPackets *)data)->Data[0],1,LogPk.Skip.AllSize-sizeof(Packets),Ms);
		}
	}