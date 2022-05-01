#include <iostream>
#include "SQLHandler.h"

int main()
{
    std::cout << "Programm started...!\n";

	// Starts Loop for SQL-Related Stuff
	PrivateHook::SQLHandler handler;
	if (handler.ConnectToDataBase())
		if (handler.RequestHandler->ConnectToDataBase())
			handler.RunThread(0);
		else
			std::cout << "RequestHandler couldn't connect to DataBase\n";
	else
		std::cout << "MainHandler couldn't connect to DataBase\n";

	system("pause");
}
