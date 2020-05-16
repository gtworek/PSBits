Password stealing DLL I wrote around 1999, some time before Active Directory was announced. And of course it still works. <br>
First, it was written in 32-bit Delphi (pardon my language) and when it stopped working as everything changed into 64-bit - in (so much simpler when it comes to Win32 API) C, as I did not have 64-bit Delphi. The original implementation was a bit more complex, including broadcasting the changed password over the network etc. but now it works as a demonstration of an idea, so let's keep it as simple as possible.<br>
It works everywhere - on local machines for local accounts and on DCs for domain accounts. 

Why does it work? Because password changing may be extended with:
- verification of complexity (used by password filters) 
- notification (used by parties willing to know about password changes)

The idea is fully documented at https://docs.microsoft.com/en-us/windows/win32/secmgmt/password-filters

A DLL installation process is described at https://docs.microsoft.com/en-us/windows/win32/secmgmt/installing-and-registering-a-password-filter-dll

And the `PasswordChangeNotify()` function is documented at  https://docs.microsoft.com/en-us/windows/win32/api/ntsecapi/nc-ntsecapi-psam_password_notification_routine
