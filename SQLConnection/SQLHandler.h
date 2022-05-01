#pragma once

#include <Windows.h>
#include <string>
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>
#include <mutex>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>

#define Type_Len  8
#define SQL_RESULT_LEN 240
#define SQL_RETURN_CODE_LEN 1000

#define CharNo_Len  8
#define Mapname_Len  50

namespace PrivateHook
{
	class SQLExecution;
	class SQLHandler
	{
	public:
		SQLHandler();
		SQLHandler(std::string ConnectionString);
		~SQLHandler() = default;
		int ConnectToDataBase();
		void InitalizeSupportThread();
		void RunThread(int size);
		bool StackHasTasks();
		int StackSize();
		SQLExecution GetSQLExecutionTask();
		static void AddSQLExecutionTask(SQLExecution Task);

		bool SupportThreadRequierd();

		static std::vector<SQLHandler*> HandlerList;
		static std::mutex HandlerListLock;
		static SQLHandler* MainHandler;
		static std::mutex RequestLock;
		static SQLHandler* RequestHandler;
		std::vector<SQLExecution> DataBaseStack;
		std::mutex DataBaseLock;

	private:
		std::string ConnectionString;
		SQLHDBC SQLConnectionHandle;
	};

	class SQLExecution
	{
		enum SQLExecutionType
		{
			Ranking = 0
		};
	public:
		SQLExecution();
		~SQLExecution() = default;

		int ExecuteTask(SQLHDBC sqlConnHandle);

		SQLExecution(int time, int MapType, char* MapName, int CharNo, unsigned long long DamageDealt, unsigned long long DamageTaken, unsigned long long  HealingDone);

	private:
		SQLExecutionType MyExecuteType;
		int time;
		int MapType;
		char* MapName;
		int CharNo;
		unsigned long long DamageDealt;
		unsigned long long DamageTaken;
		unsigned long long  HealingDone;
	};
}