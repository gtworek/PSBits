/*#############################################################################
# Be careful as this code sample changes the "protection" of a service.       #
# Protected service cannot be un-protected (error 5, because it is protected) #
# and cannot start (error 577 because it is not signed with                   #
# the proper cert containing EKU=1.3.6.1.4.1.311.61.4.1).                     #
# Rename the function to wmain() if you have read this comment ;P             #
#############################################################################*/

#include <Windows.h>

int w__main(int argc, PTSTR* argv)
{
	if (argc != 2 )
	{
		printf("Usage: SpoilService <servicename>\n");
		return;
	}

	SERVICE_LAUNCH_PROTECTED_INFO Info;
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.
	schService = OpenService(
		schSCManager,            // SCM database 
		argv[1],                 // name of service 
		SERVICE_CHANGE_CONFIG);  // need change config access 

	if (NULL == schService)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		return;
	}

	Info.dwLaunchProtected = SERVICE_LAUNCH_PROTECTED_ANTIMALWARE_LIGHT; // equals 3, but 1..3 will work same way

	// Do the change
	if (!ChangeServiceConfig2(schService,
		SERVICE_CONFIG_LAUNCH_PROTECTED,
		&Info))
	{
		printf("ChangeServiceConfig2 failed (%d)\n", GetLastError());
		return;
	}

	printf("Done.\n");
}
