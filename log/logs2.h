#include <windows.h>

#include <fstream>

namespace My_LOG
{

#define Always        0x00
#define Critical      0x01
#define Info	      0x02
#define Warning	      0x03
#define All		      0x04

#define Console		  0x01

class LOG
{
private: 

	HANDLE				m_LOG;

	int					m_Level;

	int					m_Console;

	int					m_SizeLog;

	int					m_Rotate;

	int					m_Day;			// Для перехода дня

	wchar_t				m_Path[MAX_PATH];

	wchar_t				m_Name[64];


	CRITICAL_SECTION	LogCritical; 
		
public:
	
	LOG(wchar_t *,wchar_t *);
	
	~LOG();

	int  SetParameter(int,int);

	int Print (wchar_t *);
	int Print (int ,wchar_t *Mess);
	int Print (int ,int User,wchar_t *Mess);

	void Flush();

	void Close();


private:
	void OpenOrCreateLog(wchar_t * Path,wchar_t *Nmae);

	int	Rotate();

};
}
