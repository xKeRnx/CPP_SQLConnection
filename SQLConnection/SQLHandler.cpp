#include <windows.h>
#include "SQLHandler.h"
#include "logging.h"
#include <thread>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace PrivateHook
{
	std::vector<SQLHandler*> SQLHandler::HandlerList;
	std::mutex SQLHandler::HandlerListLock;
	SQLHandler* SQLHandler::MainHandler;

	std::mutex SQLHandler::RequestLock;
	SQLHandler* SQLHandler::RequestHandler;

	SQLHandler::SQLHandler()
	{
		std::string ServerInfoPath = SStub::ExePath() + "\\9Data\\ServerInfo\\ServerInfo.txt";
		std::ifstream ServerInfo;
		std::string UID = "";
		std::string PWD = "";
		std::string Server = "";
		std::string DSN = "";

		std::cout << "ServerInfo Path: " + ServerInfoPath +  "\n";

		ServerInfo.open(ServerInfoPath, std::ios::in);
		if (ServerInfo.is_open())
		{
			std::string line;
			bool ODBCInput = false;
			while (getline(ServerInfo, line))
			{
				if (ODBCInput)
				{
					std::string Line_UpperCase = line;
					transform(Line_UpperCase.begin(), Line_UpperCase.end(), Line_UpperCase.begin(), ::toupper);
					if (Line_UpperCase.find("SERVER=") != std::string::npos) {
						Server = line.substr(Line_UpperCase.find("SERVER=") + sizeof("SERVER=") - 1, Line_UpperCase.find(";", Line_UpperCase.find("SERVER=")) - (Line_UpperCase.find("SERVER=") + sizeof("SERVER=") - 1));
						UID = line.substr(Line_UpperCase.find("UID=") + sizeof("UID=") - 1, Line_UpperCase.find(";", Line_UpperCase.find("UID=")) - (Line_UpperCase.find("UID=") + sizeof("UID=") - 1));
						PWD = line.substr(Line_UpperCase.find("PWD=") + sizeof("PWD=") - 1, Line_UpperCase.find("\"", Line_UpperCase.find("PWD=")) - (Line_UpperCase.find("PWD=") + sizeof("PWD=") - 1));
						ConnectionString = "Driver={SQL Server};Server=" + Server + ";UID=" + UID + ";PWD=" + PWD;
					}
					else
					{
						UID = line.substr(Line_UpperCase.find("UID=") + sizeof("UID=") - 1, Line_UpperCase.find(";", Line_UpperCase.find("UID=")) - (Line_UpperCase.find("UID=") + sizeof("UID=") - 1));
						PWD = line.substr(Line_UpperCase.find("PWD=") + sizeof("PWD=") - 1, Line_UpperCase.find("\"", Line_UpperCase.find("PWD=")) - (Line_UpperCase.find("PWD=") + sizeof("PWD=") - 1));
						DSN = line.substr(Line_UpperCase.find("DSN=") + sizeof("DSN=") - 1, Line_UpperCase.find(";", Line_UpperCase.find("DSN=")) - (Line_UpperCase.find("DSN=") + sizeof("DSN=") - 1));
						ConnectionString = "DSN=" + DSN + ";UID=" + UID + ";PWD=" + PWD;
					}
					break;
				}
				if (line.find("; ODBC---") == 0)
					ODBCInput = true;
			}
			ServerInfo.close();
		}
		else
		{
			std::cout << "ServerInfo locked or not found\n";
		}

		this->HandlerList.push_back(this);
		MainHandler = this;
		this->SQLConnectionHandle = NULL;
		this->RequestHandler = new SQLHandler(ConnectionString);
	}

	SQLHandler::SQLHandler(std::string ConnectionString)
	{
		this->ConnectionString = ConnectionString;
	}

	int SQLHandler::ConnectToDataBase()
	{
		SQLHANDLE sqlEnvHandle;
		SQLHANDLE sqlStmtHandle;
		SQLCHAR retconstring[SQL_RETURN_CODE_LEN];
		SQLRETURN ret;
		SQLConnectionHandle = NULL;
		int retValue = 0;
		if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle))
		{
			return 0;
		}

		if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0))
		{
			return 0;
		}

		if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &SQLConnectionHandle))
		{
			return 0;
		}
		ret = SQLDriverConnectA(SQLConnectionHandle, NULL, (SQLCHAR*)(ConnectionString.c_str()), SQL_NTS, retconstring, 1024, NULL, SQL_DRIVER_NOPROMPT);
		if (SQL_SUCCEEDED(ret))
		{
			return 1;
		}
		else
		{
			std::cout << "Couldn't connect to DataBase!\n";
			return 0;
		}
	}

	SQLExecution SQLHandler::GetSQLExecutionTask()
	{
		std::unique_lock<std::mutex> lock(this->DataBaseLock);
		SQLExecution Task = this->DataBaseStack.at(0);
		this->DataBaseStack.erase(this->DataBaseStack.begin());
		return Task;
	}

	void SQLHandler::AddSQLExecutionTask(SQLExecution Task)
	{
		std::unique_lock<std::mutex> lock(MainHandler->DataBaseLock);
		MainHandler->DataBaseStack.push_back(Task);
	}

	bool SQLHandler::StackHasTasks()
	{
		std::unique_lock<std::mutex> lock(this->DataBaseLock);
		if (this->DataBaseStack.size() > 0)
			return true;
		return false;
	}

	int SQLHandler::StackSize() {
		std::unique_lock<std::mutex> lock(this->DataBaseLock);
		return this->DataBaseStack.size();
	}

	void SQLHandler::RunThread(int size)
	{
		if (true)
		{
			std::unique_lock<std::mutex> lock(SQLHandler::HandlerListLock);
			SQLHandler::HandlerList.push_back(this);
			std::cout << "Connected to DB and Thread started\n";
		}
		while (size == 0 || size >= MainHandler->StackSize())
		{
			if (this == MainHandler && MainHandler->SupportThreadRequierd())
			{
				/*std::thread  t(&SQLHandler::InitalizeSupportThread, this);
				t.join();
				t.detach();*/
			}
			if (MainHandler->StackHasTasks())
			{
				SQLExecution Task = MainHandler->GetSQLExecutionTask();
				Task.ExecuteTask(this->SQLConnectionHandle);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
		}
		if (true)
		{
			std::cout << "SQL Log!\n";
			std::unique_lock<std::mutex> lock(SQLHandler::HandlerListLock);
			auto it = std::find(SQLHandler::HandlerList.begin(), SQLHandler::HandlerList.end(), this);
			SQLHandler::HandlerList.erase(it);
		}
	}

	bool SQLHandler::SupportThreadRequierd()
	{
		int DBStackSize = 0;
		int HandlerListSize = 0;
		if (true)
		{
			std::unique_lock<std::mutex> lock(this->DataBaseLock);
			DBStackSize = this->DataBaseStack.size();
		}
		if (true)
		{
			std::unique_lock<std::mutex> lock(SQLHandler::HandlerListLock);
			HandlerListSize = this->HandlerList.size();
		}
		if (HandlerListSize * 100 < DBStackSize)
			return true;
		return false;
	}

	void SQLHandler::InitalizeSupportThread()
	{
		SQLHandler Handler(this->ConnectionString);
		Handler.ConnectToDataBase();
		std::unique_lock<std::mutex> lock(this->HandlerListLock);
		Handler.RunThread((this->DataBaseStack.size() + 1) * 100);
	}

	int SQLExecution::ExecuteTask(SQLHDBC sqlConnHandle)
	{
		return true;
	}

	SQLExecution::SQLExecution(int time, int MapType, char* MapName, int CharNo, unsigned long long DamageDealt, unsigned long long DamageTaken, unsigned long long  HealingDone)
	{
		MyExecuteType = SQLExecutionType::Ranking;
		this->time = time;
		this->MapType = MapType;
		this->MapName = MapName;
		this->CharNo = CharNo;
		this->DamageDealt = DamageDealt;
		this->DamageTaken = DamageTaken;
		this->HealingDone = HealingDone;
	}

}
