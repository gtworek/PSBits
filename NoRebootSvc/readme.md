The source code and the compiled .exe demonstrate how Windows Services may delay shutown. 

Needed pieces, making your service working such way, include:
1. registering with `RegisterServiceCtrlHandlerEx()`
1. accepting `SERVICE_CONTROL_PRESHUTDOWN`
1. switching to `SERVICE_STOP_PENDING`
1. increasing `dwWaitHint` value

It is somewhat documented at https://docs.microsoft.com/en-us/windows/win32/services/service-control-handler-function but in practice requires more trying than following instructions step by step.

The function responsible for displaying shutdown message is not documented at all, and feel free to use it if you need to.

I am not really sorry for the message displayed during shutdown. Just a bad joke/prank if you install the service on a friend's machine.

- Installation: `sc.exe create NoRebootSvc binPath= c:\noreboot.exe`
- Uninstall: `sc.exe delete NoRebootSvc`
- Remember about starting the service.
- If you try to stop it, it just stops. It goes into infinite loop only when it is running during a shutdown.
- If you are hit, and you wait at the bluish screen, you can try to kill the service process over the network, running on the remote machine `taskkill.exe /S server_name /IM noreboot.exe` The only other option is to switch power off.

**Have fun.**
