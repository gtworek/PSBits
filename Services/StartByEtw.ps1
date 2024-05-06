$TypeDef = @"
    using System;
    using System.Runtime.InteropServices;

    namespace Api
    {
        public static class Ntdll
        {
            [DllImport("ntdll.dll")]
            public static extern uint EtwEventWriteNoRegistration(
                Guid ProviderId,
                IntPtr EventDescriptor,
                uint Length,
                IntPtr UserData);
        }

    }
"@

$DebugPreference = "Continue"

if (-not ([System.Management.Automation.PSTypeName]'Api.Ntdll').Type)
{
    Add-Type -TypeDefinition $TypeDef
}

$eventDescriptorSize = 64 # sizeof(EVENT_DESCRIPTOR),  only 16 needed actually

$pEventDescriptor = [Runtime.InteropServices.Marshal]::AllocHGlobal($eventDescriptorSize)
[Runtime.InteropServices.Marshal]::WriteByte($pEventDescriptor, 0, $eventDescriptorSize)

$g = [guid]::new('e46eead8-0c54-4489-9898-8fa79d059e0e') # see sc.exe qtriggerinfo wersvc

Write-Debug ('Status before ETW: ' + ((Get-Service wersvc).Status))

$result = [Api.Ntdll]::EtwEventWriteNoRegistration($g, $pEventDescriptor, 0, [System.IntPtr]::Zero)

Start-Sleep -Seconds 1
Write-Debug ('Status  after ETW: ' + ((Get-Service wersvc).Status))

[Runtime.InteropServices.Marshal]::FreeHGlobal($pEventDescriptor)
