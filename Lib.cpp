// This is the main DLL file.
#include "Stdafx.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <vector>
#include "Timer.h"
#include <Windows.h>
#include "KulonDll.h"
#include <string>

#pragma comment (lib, "KulonDll.lib")

#ifdef UNICODE
 typedef LPWSTR LPTSTR;
#else
 typedef LPSTR LPTSTR;
#endif

using namespace std;
using namespace System;
using namespace System::Runtime::InteropServices;
using namespace KulonLib;
typedef long long ll; 
typedef unsigned int uint;
typedef pair <int, int> pii;


struct SETTINGS_FILE
{
	     char* IpAdres;
         char*  Port;
         char*  login;
         char*  pass;
         char*  ListFile;
         char*  LogFile;
};

struct DevLine
{
public:
	int id;
	int que;
	int MDB;
  uint line;
	std::string name;
	DevLine(int idv, int quev,int MDBv)
	{
		id=idv;
		que=quev;
		MDB=MDBv;
	}
};

bool operator < (const DevLine  &a,const DevLine  &b)
{
	if(a.id==b.id)
		if(a.que==b.que)
			return a.MDB<b.MDB;
		else
			return a.que<b.que;
	return a.id<b.id;
}

int list,cntrec;
int ConnectStatus;
int ServerStatus;
TTimer timer,timerNo,timerRec;
SETTINGS_FILE Settings;
map <uint, DEVICE_STATE> devices;
map <DevLine,int> usedActive;
map <int ,pair<int, int> > usedCount;
HANDLE hCon;

char charsToTrim[] = { '\t', '\n', '\r',' '};
void GetSettingsFile(SETTINGS_FILE &);
void LoadFile(std::string);
void DeviceStateCallback(DEVICE_STATE* state);	
void ServerStateCallback(int state);
void DeviceShotCallback(PHOTO_SHOT* shot);
void ResultCallback(int DeviceID,
		int type,
		int result,
		int modBusAddr);

public delegate void H_DeviceStateCallback(uint id);
public delegate void H_ServerStateCallback(uint id);
public delegate void H_ResultCallback(uint DeviceID,
		int type,
		int result,
		int modBusAddr);
public delegate void H_DeviceFileCallback(uint line, int que, uint id, int MDB,int proc);
public delegate void H_ResultFileCallBack(uint line, int res, int que, uint id, int MDB);

ref class ManagedGlobals {
  public:
  static H_DeviceStateCallback^ S_DeviceStateCallback = nullptr;
  static H_ServerStateCallback^ S_ServerStateCallback = nullptr;
  static H_ResultCallback^ S_ResultCallback = nullptr;
  static H_ResultFileCallBack^ S_ResultFileCallback=nullptr;
  static H_DeviceFileCallback^ S_DeviceFileCallback=nullptr;
};

namespace Lib
{
	public ref class Kulon
	{
		public :
			void H_Callback(H_DeviceStateCallback^ S_DeviceStateCallback,
				H_ServerStateCallback^ S_ServerStateCallback,
				H_ResultCallback^ S_ResultCallback,H_DeviceFileCallback^ S_DeviceFileCallback, H_ResultFileCallBack^ S_ResultFileCallback);
			int H_KulonTest(int a, int b);

      bool H_KulonLockDMX(unsigned int deviceID,
										 int modBusAddr,int startSlot,
										 int length)
      {
        return KulonLockDMX(deviceID,modBusAddr,startSlot,length);
      }


      bool H_KulonUnLockDMX(unsigned int deviceID,
										 int modBusAddr,int startSlot,
										 int length)
      {
       
        return KulonUnLockDMX(  deviceID,
										  modBusAddr, startSlot,
										  length);
      }

      bool H_KulonStartProgramm(unsigned int deviceID,int level,
										   String^ name,int mode,int startSlot,
										   int modBusAddr,int startFlag)
      {
        char st[256];
        for(int i=0; i<name->Length; i++)
          st[i]=name[i];
        st[name->Length]=0;
        return KulonStartProgramm(  deviceID, level,
										   st, mode, startSlot,
										    modBusAddr, startFlag);
      }
      bool H_KulonResetDevice(unsigned int deviceID,
											int modBusAddr)
      {
        return KulonResetDevice(  deviceID,
											 modBusAddr);
      }
      bool H_KulonSendIDRequest(unsigned int deviceID,
											int modBusAddr)
      {
      return KulonSendIDRequest(  deviceID,
											 modBusAddr);
      }
      bool H_KulonClearSendBuffer(unsigned int deviceID)
      {
        return KulonClearSendBuffer( deviceID);
      }

      String^ H_KulonGetLastError()
      {
        const char* st=KulonGetLastError();
        return gcnew String(st);
      }

      bool H_KulonSendCommand(unsigned int deviceID,String^ command)
      {
        char st[256];
        int k=0;
        for(int i=0; i<command->Length; i++)
          st[k++]=command[i];
        st[k]=0;
        return KulonSendCommand( deviceID, st);
      }

			void H_Disconnect()
			{
				ServerStatus=0;
				KulonDisconnectFromServer();
				//KulonStop();
			}
			int H_ServerStatus();
			DeviceState H_KulonGetDevice(uint id);
			String^ H_KulonStart();
			void H_LoadFile(String^);
			String^ H_GetDeviceId(uint Id);
			bool H_KulonStartDmx(uint DeviceId, int ModBusAddr, int StartSlot, int Length, array<Byte>^ PDmxData);
			//void WriteToFileList();
			array <uint>^ H_IdToArray();
	};

	void Kulon::H_Callback(H_DeviceStateCallback^ S_DeviceStateCallback,
		H_ServerStateCallback^ S_ServerStateCallback,
		H_ResultCallback^ S_ResultCallback,
    H_DeviceFileCallback^ S_DeviceFileCallback,
    H_ResultFileCallBack^ S_ResultFileCallback)
	{
		ManagedGlobals::S_DeviceStateCallback=S_DeviceStateCallback;
		ManagedGlobals::S_ServerStateCallback=S_ServerStateCallback;
		ManagedGlobals::S_ResultCallback=S_ResultCallback;
    ManagedGlobals::S_DeviceFileCallback=S_DeviceFileCallback;
    ManagedGlobals::S_ResultFileCallback=S_ResultFileCallback;
	}
	array <uint>^ Kulon::H_IdToArray()
	{
		array <uint>^ arr=gcnew array<uint>(devices.size());
		int j=0;
		for(map<uint,DEVICE_STATE>::iterator it=devices.begin(); it!=devices.end(); ++it)
			arr[j++]=it->first;
		return arr;
	}
	int Kulon::H_ServerStatus()
	{
		switch(ServerStatus)
		{
		case SCS_DISCONNECT:
			return 0;
		case SCS_CONNECT:
			return 1;
		case SCS_CONNECTING:
			return 2;
		case SCS_NETERROR:
			return 3;
		}
	}
	DeviceState Kulon:: H_KulonGetDevice(uint id)
	{
		 
		DeviceState res;
		/*if(devices.find(id)==devices.end())
			return NULL;*/
		DEVICE_STATE dev=devices[id];
		res.all_save_mes_count=dev.all_save_mes_count;
		res.camera_rele=dev.camera_rele;
		res.camera_status=dev.camera_status;
		res.command_count=dev.command_count;
		res.connection_quality=dev.connection_quality;
		res.connect_state=dev.connect_state;
		res.connect_type=dev.connect_type;
		res.state_script_list=gcnew String(dev.state_script_list.c_str());
		res.startmode_script_list=gcnew String(dev.startmode_script_list.c_str());
		res.offTime_script_list=gcnew String(dev.offTime_script_list.c_str());
		res.current_offTime_script=gcnew String(dev.current_offTime_script.c_str());
		res.current_save_mes_count=dev.current_save_mes_count;
		res.current_startmode_script=gcnew String(dev.current_startmode_script.c_str());
		res.current_state_script=gcnew String(dev.current_state_script.c_str());
		res.deviceID=dev.deviceID;
		res.devicestate=dev.devicestate;
		res.dimmerError=dev.dimmerError;
		res.dimmerState=dev.dimmerState;
		res.error_mask=dev.error_mask;
		res.groupID=dev.groupID;
		res.LightSensor=dev.LightSensor;
		res.tab_order=dev.tab_order;
		res.telemetry_errors=gcnew array<int>(TELEMETRY_SIZE);
		res.name=gcnew String(dev.name);
		res.m_sTel_number=gcnew String(dev.m_sTel_number);
		res.state_sended_req=dev.state_sended_req;
		res.warning_mask=dev.warning_mask;
		//res.last_connect_time=dev.last_connect_time;
		//res.last_telemetry_time=dev.last_telemetry_time;
		for(int i=0; i<TELEMETRY_SIZE; i++)
			res.telemetry_errors[i]=dev.telemetry_errors[i];

		res.telemetry=gcnew array<double>(TELEMETRY_SIZE);

		for(int i=0; i<TELEMETRY_SIZE; i++)
			res.telemetry[i]=dev.telemetry[i];


		res.InState_ext=gcnew array<uint>(8);
		for(int i=0; i<8; i++)
			res.InState_ext[i]=dev.InState_ext[i];

		res.rele_state_ext=gcnew array<uint>(8);

		for(int i=0; i<8; i++)
			res.rele_state_ext[i]=dev.rele_state_ext[i];

		res.rele_error_ext=gcnew array<uint>(8);

		for(int i=0; i<8; i++)
			res.rele_error_ext[i]=dev.rele_error_ext[i];

		res.warning_ext=gcnew array<uint>(8);

		for(int i=0; i<8; i++)
			res.warning_ext[i]=dev.warning_ext[i];

		res.m_ReleState=gcnew array<uint>(4);

		for(int i=0; i<4; i++)
			res.m_ReleState[i]=dev.m_ReleState[i];

		res.m_InState=gcnew array<uint>(4);

		for(int i=0; i<4; i++)
			res.m_InState[i]=dev.m_InState[i];
		return res;
	}

	bool Kulon::H_KulonStartDmx(uint DeviceId, int ModBusAddr, int StartSlot, int Length, array<Byte>^ PDmxData)
	{
		byte* PDmxData_c=new byte[PDmxData->Length];
		for(int i=0;i<PDmxData->Length; i++)
			PDmxData_c[i]=PDmxData[i];
		return KulonSetDMX(DeviceId, ModBusAddr, StartSlot, Length, PDmxData_c);
	}

	String^ Kulon::H_GetDeviceId(uint Id)
	{
		DEVICE_STATE dev=devices[Id]; 
		String^ str="";
		str+=dev.deviceID.ToString()+"\t"
			+gcnew String(dev.name)+"\t"
			+gcnew String(dev.m_sTel_number)+"\t"
			+dev.connect_state.ToString()+"\t"
			+dev.devicestate.ToString();
		return str;
	}

	int Kulon::H_KulonTest(int a, int b)
	{
		return KulonTest(a,b);
	}

	String^ Kulon::H_KulonStart()
	{
		setlocale(LC_ALL, "rus");   
		SetConsoleCP(866);
		hCon=GetStdHandle(STD_OUTPUT_HANDLE);
		GetSettingsFile(Settings);
		KulonStart(&DeviceStateCallback, 
		&DeviceShotCallback, 
		&ServerStateCallback, 
		&ResultCallback);
	
	KulonSetServerSettings(Settings.IpAdres, 
		Settings.Port, 
		Settings.login, 
		Settings.pass);
	KulonConnectToServer();
		while(ConnectStatus<5)
		{
			switch(ConnectStatus)
			{
			case SCS_NETERROR:
				return "Произошла ошибка сооединенния!";
			case SCS_DEVICELIST:
				return "Получен спиcок объектов";
			}
			Sleep(500);
		}
		return "Неверный логин или пароль";
	}

	void Kulon::H_LoadFile(String^ strc)
	{
		std::string str="";
		for(int i=0; i<strc->Length; i++)
			str+=strc[i];
		LoadFile(str);
	}

}

char* StrTrim(string buf)
{
	char* strc;
	string str="";
	for(int i=0; buf[i]; i++)
	{
		if(!binary_search(charsToTrim,charsToTrim+4,buf[i]))
			str+=buf[i];
	}
	strc=new char[str.length()+1];
	strcpy(strc, str.c_str());
	return strc;
}

char* StrTrim(char* buf,bool bo=true)
{
	char* strc;
	string str="";
	for(int i=0; buf[i]; i++)
	{
		if(buf[i]=='"'&&bo)
			break;
		if(bo&&!binary_search(charsToTrim,charsToTrim+4,buf[i]))
			str+=buf[i];
		if(buf[i]=='"'&&!bo)
		bo=true;
	}
	strc=new char[str.length()+1];
	strcpy(strc, str.c_str());
	return strc;
}

int StringToInt(string str)
{
	char* strc=new char[str.length()+1];
	strcpy(strc, str.c_str());
	str=StrTrim(strc);
	strcpy(strc, str.c_str());
	int res=atoi(strc);
	delete []strc;
	strc=NULL;
	return res;
}

byte* FileRead(string path)
{
	FILE * pFile;
	long lSize;
	byte * buffer;
	size_t result;
	char* strc=StrTrim(path);
	pFile = fopen (strc, "rb" );
	if (pFile==NULL) {fputs ("File error",stderr); return NULL;}
	delete[] strc;
	strc=NULL;
	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);

	buffer = new byte[lSize];
	if (buffer == NULL) {fputs ("Memory error",stderr); return NULL;}

	result = fread (buffer,1,lSize,pFile);
	if (result != lSize) {fputs ("Reading error",stderr); return NULL;}
	fclose (pFile);
	return buffer;
}

string* ParseDelim(char* buf)
{
	bool bo=false;
	string* mass=new string[5];
	int i=0;
	for(int j=0; buf[j]; j++)
	{
		if(!binary_search(charsToTrim,charsToTrim+4,buf[j]))
			bo=true;
		if(bo&&buf[j]!=';')
			mass[i]+=buf[j];
		if(buf[j]==';')
		{
			i++;
			bo=false;
		}
	}
	return mass;
}

COORD GetCurrCursor()
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(hCon, &info);
	COORD pos = {info.dwCursorPosition.X, info.dwCursorPosition.Y};
	return pos;
}

void OnTimedEvent()
{
	COORD curs,currcurs;
	curs.X=0;
	DEVICE_STATE dev;
	currcurs=GetCurrCursor();
	for(map <DevLine, int>::iterator it=usedActive.begin(); it!=usedActive.end(); ++it)
	{
		if(usedCount[it->first.id].second!=it->first.que)
			continue;
		dev = devices[it->first.id];
		double proc;
		if(dev.all_save_mes_count!=0)
			proc = dev.current_save_mes_count / (dev.all_save_mes_count + .0);
		else
			proc=0;
		curs.Y=it->second;
		SetConsoleCursorPosition(hCon,curs);
    int que=usedCount[dev.deviceID].first-usedCount[dev.deviceID].second;
    ManagedGlobals::S_DeviceFileCallback((uint)it->first.line,que,(uint)it->first.id, it->first.MDB,proc*100);
		printf("%s, %.2llf%%, MDB = %d, Очередь = %d.  \n", dev.name,proc*100,it->first.MDB,que);
		SetConsoleCursorPosition(hCon,currcurs);
	}

}

void TimerStart()
{
	timer.SetTimedEvent(&OnTimedEvent);
    timer.Start(10000);
}

void GetSettingsFile(SETTINGS_FILE &st)
	{
		char* path="Settings.ini";
		char** mass=new char*[6];
		char buf[256];
		int count = -3;
		ifstream f(path);
		if(!f)
		{
			puts("Файл не прочитан");
			return;
		}
		while(count<5)
		{
			f.getline(buf,256);
			if (++count >= 0)
			mass[count]=StrTrim(buf,false);
		}
		f.close();
		st.IpAdres=mass[0];
		st.Port = mass[1];
		st.login = mass[2];
		st.pass = mass[3];
		st.ListFile = mass[4];
		st.LogFile = mass[5];
		delete[]mass;
		mass=NULL;
	}

void GetListDevice()
	{
		for(map<uint,DEVICE_STATE>::iterator it=devices.begin(); it!=devices.end(); ++it)
		{
			DEVICE_STATE dev=it->second;
			printf("%d\t%s\t%s\t%d\t%d\n", 
				dev.deviceID,
				dev.name,
				dev.m_sTel_number,
				dev.connect_state,
				dev.devicestate);
				//cout<<dev.state_script_list<<endl;
				//puts("");
		}
	}


void clearLine(const COORD &curs)
{
	SetConsoleCursorPosition(hCon,curs);
	for(int i=0; i<80; i++)
		printf(" ");
	//SetConsoleCursorPosition(hCon,GetCurrCursor());
}

void updateList(int DeviceID)
{
	COORD curs,currcurs;
	curs.X=0;
	currcurs=GetCurrCursor();
	for(map <DevLine, int>::iterator its=usedActive.begin(); its!=usedActive.end(); ++its)
		{
			if(its->first.id==DeviceID)
			{
				wchar_t buff[256];
				DWORD r=0;
				curs.Y=its->second;
				if(ReadConsoleOutputCharacter(hCon,buff,80,curs,&r))
				{
					char st[256];
					char num[12];
					sprintf(st, "%ws", buff );
					clearLine(curs);
					int stlen=strlen(st);
					for(int i=stlen-1; i>=0; i--)
						if(st[i]>=48&&st[i]<=57&&st[i-1]==' ')
						{
							itoa(usedCount[DeviceID].first-usedCount[DeviceID].second, num, 10);
							int k=strlen(num);
							num[k++]='.';
							num[k]=0;
							for(int j=0; num[j]; j++)
								st[i++]=num[j];
							st[i]=0;
							break;
						}
					SetConsoleCursorPosition(hCon,curs);
					cout<<st;
					
				}
			}
		}
	SetConsoleCursorPosition(hCon,currcurs);
}

void OutList(vector <DevLine> spa, vector <DevLine> spn)
{
	if(!timer.enabled)
		TimerStart();
	int y;
	double proc;
	DEVICE_STATE dev;
	if(!spa.empty())
		puts("Обьекты которые в сети:");
	for(int i=0; i<spa.size(); i++)
	{

		updateList(spa[i].id);
		//SetConsoleCursorPosition(hCon,GetCurrCursor());
		y=GetCurrCursor().Y;
		usedActive[spa[i]]=y;
		dev=devices[spa[i].id];
		double proc;
		if(dev.all_save_mes_count!=0)
			proc = dev.current_save_mes_count / (dev.all_save_mes_count + .0);
		else
			proc=0;
		printf("%s, %.2llf%%, MDB = %d, Очередь = %d.\n", dev.name,proc*100,spa[i].MDB,usedCount[dev.deviceID].first-usedCount[dev.deviceID].second);
	}
	if(spn.empty())
		return;
	puts("Обьекты которые не в сети:");
	for(int i=0; i<spn.size(); i++)
	{
		y=GetCurrCursor().Y;
		usedActive[spn[i]]=y;
		dev=devices[spa[i].id];
		double proc;
		if(dev.all_save_mes_count!=0)
			proc = dev.current_save_mes_count / (dev.all_save_mes_count + .0);
		else
			proc=0;
		printf("%s, %.2llf%%, MDB = %d, Очередь = %d.\n", dev.name,proc*100,spn[i].MDB,usedCount[dev.deviceID].first-usedCount[dev.deviceID].second);
	}
}

void LoadFile(string path)
	{
		puts("cfg file load...");
		vector <DevLine> spa,spn;
		char* buf=new char[256];
		path = StrTrim(path);
		fstream f(path);
		if(!f)
		{
			puts("Ошибка загрузки cfg файла!");
			return;
		}
		uint counter = 0;
		while (!f.eof())
		{
			f.getline(buf,256);
			if (++counter > 1&&strlen(buf)>0)
			{
				string* pars = ParseDelim(buf);
				char nameFile[256];
				int j=0;
				for(int i=pars[4].size()-1; i>=0&&pars[4][i]!='\\'; i--)
					nameFile[j++]=pars[4][i];
				nameFile[j]=0;
				reverse(nameFile, nameFile+strlen(nameFile));
				cout<<"Upload file: "<<pars[2]<<" "<<nameFile<<endl;
				byte* pFileData=FileRead(pars[4]);
				if(pFileData==NULL)continue;
				SYSTEMTIME time;
				GetSystemTime(&time);

				KulonSaveFile(StringToInt(pars[2]), 
					time, nameFile, 
					StringToInt(pars[3]), 
					0, 
					_msize(pFileData),
					pFileData);
				DevLine dev(StringToInt(pars[2]), usedCount[StringToInt(pars[2])].first++, StringToInt(pars[3]));
				dev.name=string(nameFile);
        dev.line=counter;
				if(devices[StringToInt(pars[2])].connect_state)
					spa.push_back(dev);
				else
					spn.push_back(dev);
				delete[] pFileData;
				pFileData=NULL;
			}
		}
		OutList(spa,spn);
		f.close();
		delete[] buf;
	}

void DeviceStateCallback(DEVICE_STATE *Device)
{
	devices[Device->deviceID]=*Device;
	ManagedGlobals::S_DeviceStateCallback(Device->deviceID);
	//SYSTEMTIME time;
	//GetSystemTime(&time);
	#ifdef SNELTYN
	/*printf("StateCalback %d:%d:%d ID=%d\n",
		time.wHour,
		time.wMinute,
		time.wSecond,
		Device->deviceID);*/
#endif
}
void ConnectFrom()
{
  cout<<ServerStatus<<" "<<SCS_CONNECT<<endl;
  while(ServerStatus!=SCS_CONNECT){
    cout<<"OK"<<endl;
   /* KulonStart(&DeviceStateCallback, 
		&DeviceShotCallback, 
		&ServerStateCallback, 
		&ResultCallback);
    KulonSetServerSettings(Settings.IpAdres, 
		Settings.Port, 
		Settings.login, 
		Settings.pass);*/
        Sleep(5000);
    KulonConnectToServer();
  }
}

void goReconnect(){
  KulonConnectToServer();
}

void ServerStateCallback(int State)
{
	ManagedGlobals::S_ServerStateCallback((uint)State);
	if(State==SCS_DEVICELIST&&list++!=2)
		return;
	switch (State)
	{
		case SCS_DISCONNECT:
             puts("Disconnect from the server.");
             KulonDisconnectFromServer();
			       ServerStatus=State;
             timerRec.SetTimedEvent(&goReconnect);
             timerRec.Start(5000);
             //KulonConnectToServer();
             break;
        case SCS_CONNECT:
             puts("Connected to server.");
             timerRec.Stop();
			 ServerStatus=State;
             break;
        case SCS_CONNECTING:
             puts("Connecting to server...");
			 ServerStatus=State;
             break;
        case SCS_NETERROR:
             puts("NetError.");
			 ServerStatus=State;
			 ConnectStatus=State;
             break;
        case SCS_INCORRECT_PASSWORD:
             puts("Incorrect password, please verify connection settings.");
			 KulonStop();
			 ConnectStatus=State;
             break;
		case SCS_DEVICELIST:
			 puts("Получен список объектов");
			 GetListDevice();
			 ConnectStatus=State;
			 break;
	}
}

char * CharRes(int res)
{
	switch(res)
	{
		case 166: 
			return "OK";
		case 4138:
			return "Ошибка";
	}
	char buf[15];
	itoa(res, buf,10);
	return buf;
}

void writeLog(char* name, int MDB, int res)
{
	SYSTEMTIME time;
	GetSystemTime(&time);
	ofstream f(Settings.LogFile,ios_base::app);
	if(!f)
	{
		puts("Не удалось записать лог");
		return;
	}
	f<<"["<<time.wDay<<"."<<time.wMonth<<"."<<time.wYear<<" "<<time.wHour<<":"<<time.wMinute<<":"<<time.wSecond<<"] "<<name<<", MDB = "<<MDB<<", Status = "<<CharRes(res)<<"."<<endl;
}

void ResultCallback(int DeviceID,int type,int result,int modBusAddr)
{
	ManagedGlobals::S_ResultCallback((uint)DeviceID,type,result,modBusAddr);
  
	timer.Stop();
	COORD curs,currcurs;
	DEVICE_STATE dev;
	curs.X=0;
	DevLine devl(DeviceID,usedCount[DeviceID].second++,modBusAddr);
	map <DevLine, int>::iterator it = usedActive.find(devl);
	if(it!=usedActive.end())
	{
    int que=usedCount[DeviceID].first-usedCount[DeviceID].second;
    ManagedGlobals::S_ResultFileCallback((uint)devl.line, result, que, (uint)DeviceID, modBusAddr);
		currcurs=GetCurrCursor();
		curs.Y=usedActive[devl];
		SetConsoleCursorPosition(hCon,curs);
		dev=devices[it->first.id];
		printf("%s, %.2llf%%, MDB = %d, Status - %s, Очередь = %d.\n", dev.name,double(100),modBusAddr,CharRes(result),que);
		writeLog(dev.name,modBusAddr,result);
		
		updateList(DeviceID);
		SetConsoleCursorPosition(hCon,currcurs);
		map <int , pair <int, int >>::iterator itUc=usedCount.find(DeviceID);
		if(itUc!=usedCount.end()&&usedCount[DeviceID].first==usedCount[DeviceID].second)
		{
			usedCount.erase(itUc);
				for(map <DevLine, int>::iterator its=usedActive.begin(); its!=usedActive.end();)
					if(its->first.id==DeviceID)
					{
						usedActive.erase(its);
						its=usedActive.begin();
					}
					else
						++its;
					
		}

		if(usedActive.empty())
		{
			puts("Просмотрите log по загрузке");
			timer.Stop();
			return;
		}
		timer.Start(10000);
	}
}

void DeviceShotCallback(PHOTO_SHOT* shot)
{
	printf("Пришло фото с ID: %d\n", shot->deviceID);
}