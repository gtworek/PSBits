# `NtPowerInformation()`

The document and code refer to the `NtPowerInformation()` syscall. It looks both interesting and undocumented, which makes it the perfect area for some digging.

Microsoft provides the following documentation:
- [`NtPowerInformation()`](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-ntpowerinformation) - the real syscall. Document specifies the function prototype, saying "*the only supported POWER_INFORMATION_LEVEL value is PlatformInformation*".
- [`POWER_INFORMATION_LEVEL`](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-power_information_level) - values to call the function with. The document presenting dozens of different POWER_INFORMATION_LEVEL values, and doing it's best to non-explain them.
- [`CallNtPowerInformation()`](https://docs.microsoft.com/en-us/windows/win32/api/powerbase/nf-powerbase-callntpowerinformation) - wrapper function, calling the `NtPowerInformation()` mentioned above. Document explains some (about 20 out of 90) `InformationLevel` values. Function tries to enable `SeShutdownPrivilege` and `SeCreatePagefilePrivilege` privileges for some `InformationLevel` values. Returns `STATUS_INVALID_PARAMETER` when called with undocumented value, even if the `NtPowerInformation()` works properly. It looks like it was the "*official*" way of calling the real function, before `NtPowerInformation()` syscall was documented.
- [`ADMINISTRATOR_POWER_POLICY`](https://docs.microsoft.com/en-us/windows/win32/api/WinNT/ns-winnt-administrator_power_policy) - structure description.
- [`MONITOR_DISPLAY_STATE`](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-_monitor_display_state) - enum description.
- [`PROCESSOR_POWER_INFORMATION`](https://docs.microsoft.com/en-us/windows/win32/power/processor-power-information-str) - structure description.
- [`SYSTEM_BATTERY_STATE`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_battery_state) - structure description.
- [`SYSTEM_POWER_CAPABILITIES`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_power_capabilities) - structure description.
- [`SYSTEM_POWER_INFORMATION`](https://docs.microsoft.com/en-us/windows/win32/power/system-power-information-str) - structure description.
- [`SYSTEM_POWER_POLICY`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_power_policy) - structure description.
- `ntpoapi.h` - header file, distributed by Microsoft as a part of SDK. Contains related enums and structures.
- `winnt.h` - header file, distributed by Microsoft as a part of SDK. Contains `POWER_INFORMATION_LEVEL` enum.

Most of `InformationLevel` values require `SeShutdownPrivilege`, which means it is allowed for each user on Windows 10. If other privilege requirements are identified, it is clearly written in the level description.

Some power-related parameters can be spotted in the registry under `HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Power`.

If you want to call `NtPowerInformation()` on your own, you can use the following C code:
```C
typedef NTSTATUS(__stdcall* PO_NtPowerInformation)(
	IN POWER_INFORMATION_LEVEL InformationLevel,
	IN OPTIONAL PVOID InputBuffer,
	IN ULONG InputBufferLength,
	OUT OPTIONAL PVOID OutputBuffer,
	IN ULONG OutputBufferLength
	);

DWORD MyNtPowerInformation(
	IN POWER_INFORMATION_LEVEL InformationLevel,
	IN OPTIONAL PVOID InputBuffer,
	IN ULONG InputBufferLength,
	OUT OPTIONAL PVOID OutputBuffer,
	IN ULONG OutputBufferLength
)
{
	static HMODULE hNtoskrnlModule = NULL;
	static PO_NtPowerInformation pfnNtPowerInformation = NULL;

	if (NULL == hNtoskrnlModule)
	{
		hNtoskrnlModule = LoadLibraryEx(L"ntdll.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (NULL == hNtoskrnlModule)
		{
			return GetLastError();
		}
		pfnNtPowerInformation = (PO_NtPowerInformation)GetProcAddress(hNtoskrnlModule, "NtPowerInformation");
		if (NULL == pfnNtPowerInformation)
		{
			return GetLastError();
		}
	}
	if (NULL == pfnNtPowerInformation)
	{
		return ERROR_PROC_NOT_FOUND;
	}

	return pfnNtPowerInformation(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}
```
---

## `POWER_INFORMATION_LEVEL` reference

The information below relies on Win10 1903. Feel free to contribute.

In some cases, "*returns...*" means "*fills the output buffer with...*".


| Value | Level | Comments |
| ---: | --- | --- |
| 0 | `SystemPowerPolicyAc` | Used for managing system power policy. Uses  [`SYSTEM_POWER_POLICY`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_power_policy) structure. `NtPowerInformation()` gets the policy if `InputBuffer` is NULL, and sets it otherwise. |
| 1 | `SystemPowerPolicyDc` | DC counterpart of `SystemPowerPolicyAc` described above. |
| 2 | `VerifySystemPolicyAc` | Gets the power policy, and returns it matched against OS/hardware capabilities. |
| 3 | `VerifySystemPolicyDc` | DC counterpart of `VerifySystemPolicyAc` described above. |
| 4 | `SystemPowerCapabilities` | Returns [`SYSTEM_POWER_CAPABILITIES`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_power_capabilities) structure. You can see it with `!pocaps` in WinDbg. |
| 5 | `SystemBatteryState` | Returns [`SYSTEM_BATTERY_STATE`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_battery_state) structure. See also `SystemBatteryStatePrecise`(83). |
| 6 | `SystemPowerStateHandler` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 7 | `ProcessorStateHandler` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 8 | `SystemPowerPolicyCurrent` | Returns current system power policy. Uses  [`SYSTEM_POWER_POLICY`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_power_policy) structure. You can see it with `!popolicy` in WinDbg. |
| 9 | `AdministratorPowerPolicy` | Returns `STATUS_ACCESS_DENIED` for non-admins. Used for managing system power policy. Uses [`ADMINISTRATOR_POWER_POLICY`](https://docs.microsoft.com/en-us/windows/win32/api/WinNT/ns-winnt-administrator_power_policy) structure. `NtPowerInformation()` gets the policy if `InputBuffer` is NULL, and sets it otherwise. |
| 10 | `SystemReserveHiberFile` | Requires `SeCreatePagefilePrivilege`. BOOLEAN input parameter specifies if hibernation file should be allocated or deallocated on the disk. |
| 11 | `ProcessorInformation` | Returns [`PROCESSOR_POWER_INFORMATION`](https://docs.microsoft.com/en-us/windows/win32/power/processor-power-information-str) structure. |
| 12 | `SystemPowerInformation` | Returns [`SYSTEM_POWER_INFORMATION`](https://docs.microsoft.com/en-us/windows/win32/power/system-power-information-str) structure. |
| 13 | `ProcessorStateHandler2` | Returns `STATUS_NOT_IMPLEMENTED`. |
| 14 | `LastWakeTime` | Returns *ULONGLONG that specifies the interrupt-time count, in 100-nanosecond units, at the last system wake time.* |
| 15 | `LastSleepTime` | Returns *ULONGLONG that specifies the interrupt-time count, in 100-nanosecond units, at the last system sleep time.* |
| 16 | `SystemExecutionState` | Returns the structure, which resembles the `EXECUTION_STATE` as specified in the [`SetThreadExecutionState`](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate). Requires additional research. |
| 17 | `SystemPowerStateNotifyHandler` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 18 | `ProcessorPowerPolicyAc` | Returns `STATUS_NOT_IMPLEMENTED`. |
| 19 | `ProcessorPowerPolicyDc` | Returns `STATUS_NOT_IMPLEMENTED`. |
| 20 | `VerifyProcessorPowerPolicyAc` | Returns `STATUS_NOT_IMPLEMENTED`. |
| 21 | `VerifyProcessorPowerPolicyDc` | Returns `STATUS_NOT_IMPLEMENTED`. |
| 22 | `ProcessorPowerPolicyCurrent` | Returns `STATUS_NOT_IMPLEMENTED`. |
| 23 | `SystemPowerStateLogging` | Returns information about power logging. Requires additional research. |
| 24 | `SystemPowerLoggingEntry` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 25 | `SetPowerSettingValue` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 26 | `NotifyUserPowerSetting` | Returns `STATUS_INVALID_PARAMETER`. Does not look like really checking, and it is why I believe it means "not implemented".  |
| 27 | `PowerInformationLevelUnused0` | Returns `STATUS_INVALID_PARAMETER`. Does not look like really checking, and it is why I believe it means "not implemented". The name suggests it as well. |
| 28 | `SystemMonitorHiberBootPowerOff` | Returns `STATUS_ACCESS_DENIED` for non-admins. Turns your monitor off until the reboot.  |
| 29 | `SystemVideoState` | Returns [`MONITOR_DISPLAY_STATE`](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-_monitor_display_state). |
| 30 | `TraceApplicationPowerMessage` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 31 | `TraceApplicationPowerMessageEnd` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 32 | `ProcessorPerfStates` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 33 | `ProcessorIdleStates` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 34 | `ProcessorCap` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 35 | `SystemWakeSource` | Returns last wake reasons. The structure remains a bit unclear for me. Used by `powercfg.exe /lastwake`. |
| 36 | `SystemHiberFileInformation` | Returns data about hinernation file. Probably it is about the size only, but it needs additional research.  |
| 37 | `TraceServicePowerMessage` | Sends the notification to registered parties. Requires additional research. Registering for power events is [somewhat documented](https://docs.microsoft.com/en-us/windows/win32/power/registering-for-power-events). |
| 38 | `ProcessorLoad` | Returns `STATUS_ACCESS_DENIED` for non-admins. Looks like simulated CPU load, but requires more research to confirm. |
| 39 | `PowerShutdownNotification` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 40 | `MonitorCapabilities` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 41 | `SessionPowerInit` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 42 | `SessionDisplayState` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 43 | `PowerRequestCreate` | Internal implementation of [`PowerCreateRequest()`](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powercreaterequest). Applications can use this function to ask for more power if they need it, i.e during installation, some computation, media processing etc. |
| 44 | `PowerRequestAction` | Internal implementation of [`PowerClearRequest`](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powerclearrequest). |
| 45 | `GetPowerRequestList` | Returns `STATUS_ACCESS_DENIED` for non-admins. Returns the current list of power requests. Used by `powercfg.exe /requests`. |
| 46 | `ProcessorInformationEx` | Returns [`PROCESSOR_POWER_INFORMATION`](https://docs.microsoft.com/en-us/windows/win32/power/processor-power-information-str) structure. It looks similar to `ProcessorInformation` and seems to be "cpu group"-aware.  |
| 47 | `NotifyUserModeLegacyPowerEvent` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 48 | `GroupPark` | Returns `STATUS_ACCESS_DENIED` for non-admins. Returns `STATUS_ACCESS_DENIED` if run without kernel debug (sounds strange, but it looks exactly like this). Probably it forces parking of selected CPU cores. Requires additional research. |
| 49 | `ProcessorIdleDomains` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 50 | `WakeTimerList` | Returns `STATUS_ACCESS_DENIED` for non-admins. Returns the list of wake timers, probably using the `WAKE_TIMER_INFO` defined in `ntpoapi.h`. Used by `powercfg.exe /waketimers`. |
| 51 | `SystemHiberFileSize` | Returns `STATUS_ACCESS_DENIED` for non-admins. Gets the ULONG hiberfil size as percent of the physical memory size. Returns ULONG size in bytes.  |
| 52 | `ProcessorIdleStatesHv` | ? |
| 53 | `ProcessorPerfStatesHv` | ? |
| 54 | `ProcessorPerfCapHv` | ? |
| 55 | `ProcessorSetIdle` | Returns `STATUS_ACCESS_DENIED` for non-admins. Returns `STATUS_ACCESS_DENIED` if run without kernel debug (sounds strange, but it looks exactly like this). Probably it forces idle on selected CPU cores. Requires additional research. |
| 56 | `LogicalProcessorIdling` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 57 | `UserPresence` | Returns `STATUS_NOT_IMPLEMENTED`. |
| 58 | `PowerSettingNotificationName` | ? |
| 59 | `GetPowerSettingValue` | Returns power settings for the given GUID. |
| 60 | `IdleResiliency` | Returns `STATUS_ACCESS_DENIED` for non-admins. Gets the `POWER_IDLE_RESILIENCY` structure defined in `ntpoapi.h`. Requires additional research. |
| 61 | `SessionRITState` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 62 | `SessionConnectNotification` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 63 | `SessionPowerCleanup` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 64 | `SessionLockState` | Returns `STATUS_ACCESS_DENIED` for non-admins. Looks like sending lock notification to the Winlogon. Could not make it working. Requires additional research. |
| 65 | `SystemHiberbootState` | Returns BOOLEAN value reflecting the `HiberbootEnabled` value from registry. Practically, it means "Fast Startup". |
| 66 | `PlatformInformation` | Returns BOOLEAN value indicating whether the system supports the connected standby power model. |
| 67 | `PdcInvocation` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 68 | `MonitorInvocation` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 69 | `FirmwareTableInformationRegistered` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 70 | `SetShutdownSelectedTime` | Ignores parameters and stores current high resolution clock value. It looks like a part of preparation for shutdown, when *Fast Startup* is enabled.  |
| 71 | `SuspendResumeInvocation` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 72 | `PlmPowerRequestCreate` | Similar to `PowerRequestCreate`(43). Requires additional research. |
| 73 | `ScreenOff` | Must be called with NULLs as input and output. Turns monitor off with no special reason. Mouse move turns monitor back on. |
| 74 | `CsDeviceNotification` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 75 | `PlatformRole` | Returns `POWER_PLATFORM_ROLE` as defined in `winnt.h`. |
| 76 | `LastResumePerformance` | Returns performance data about the last resume from hybrid boot, using `RESUME_PERFORMANCE`  as defined in `winnt.h`. |
| 77 | `DisplayBurst` | Must be called with NULLs as input and output. Used for wake the screen up to display critical notifications. |
| 78 | `ExitLatencySamplingPercentage` | Returns `STATUS_ACCESS_DENIED` for non-admins. Have no idea about the purpose. |
| 79 | `RegisterSpmPowerSettings` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 80 | `PlatformIdleStates` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 81 | `ProcessorIdleVeto` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 82 | `PlatformIdleVeto` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 83 | `SystemBatteryStatePrecise` | Returns [`SYSTEM_BATTERY_STATE`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_battery_state) structure. See also `SystemBatteryState`(5). |
| 84 | `ThermalEvent` | Takes some input and returns no output. Totally uknown otherwise, despite the easy name. |
| 85 | `PowerRequestActionInternal` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 86 | `BatteryDeviceState` | ? |
| 87 | `PowerInformationInternal` | ? |
| 88 | `ThermalStandby` | Same as `ScreenOff`(73), but specifies thermal standby as a reason.  |
| 89 | `SystemHiberFileType` | Returns `STATUS_ACCESS_DENIED` for non-admins. Looks like takes 1 for reduced or 2 for full hiberfile, and returns size in bytes. Used by `powercfg.exe /h /type`. |
| 90 | `PhysicalPowerButtonPress` | Returns `STATUS_ACCESS_DENIED` for non-admins. If non-zero value is provided as input, the 0x1C8 `MANUALLY_INITIATED_POWER_BUTTON_HOLD` BSOD happens after 7 seconds. Somewhat [documented](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/bug-check-0x1c8--manually-initiated-power-button-hold). |
| 91 | `QueryPotentialDripsConstraint` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 92 | `EnergyTrackerCreate` | Returns `STATUS_ACCESS_DENIED` for non-admins. Requires additional research. |
| 93 | `EnergyTrackerQuery` | Returns `STATUS_ACCESS_DENIED` for non-admins. Requires additional research. |
| 94 | `UpdateBlackBoxRecorder` | Stores some data in the *BlackBox* memory area for relatively simply access during memory dump analysis. The format and the real usage remain cryptic, but the mechanism seems to be commonly used within the OS. Probably, the blackbox can be read with [`!blackboxbsd`](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/-blackboxbsd) and [`!blackboxscm`](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/-blackboxscm) in WinDbg. |
| 95 | `PowerInformationLevelMaximum` | Not used. Marks the end of list. |
