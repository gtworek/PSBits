# `NtPowerInformation()`

The document and code refer to the `NtPowerInformation()` syscall. It looks both interesting and undocumented, which makes it the perfect area for some digging.

Microsoft provides the following documentation:
- [`NtPowerInformation()`](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-ntpowerinformation) - the real syscall. Document specifies the function prototype, saying "*the only supported POWER_INFORMATION_LEVEL value is PlatformInformation*".
- [`POWER_INFORMATION_LEVEL`](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-power_information_level) - values to call the function with. The document presenting dozens of different POWER_INFORMATION_LEVEL values, and doing it's best to non-explain them.
- [`CallNtPowerInformation()`](https://docs.microsoft.com/en-us/windows/win32/api/powerbase/nf-powerbase-callntpowerinformation) - wrapper function, calling the `NtPowerInformation()` mentioned above. Document explains some (about 20 out of 90) `InformationLevel` values. Function tries to enable `SeShutdownPrivilege` and `SeCreatePagefilePrivilege` privileges for some `InformationLevel` values. Returns `STATUS_INVALID_PARAMETER` when called with undocumented value, even if the `NtPowerInformation()` works properly. It looks like it was the "*official*" way of calling the real function, before `NtPowerInformation()` syscall was documented.
- [`ADMINISTRATOR_POWER_POLICY`](https://docs.microsoft.com/en-us/windows/win32/api/WinNT/ns-winnt-administrator_power_policy) - structure description.
- [`PROCESSOR_POWER_INFORMATION`](https://docs.microsoft.com/en-us/windows/win32/power/processor-power-information-str) - structure description.
- [`SYSTEM_BATTERY_STATE`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_battery_state) - structure description.
- [`SYSTEM_POWER_CAPABILITIES`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_power_capabilities) - structure description.
- [`SYSTEM_POWER_INFORMATION`](https://docs.microsoft.com/en-us/windows/win32/power/system-power-information-str) - structure description.
- [`SYSTEM_POWER_POLICY`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_power_policy) - structure description.
- `winnt.h` - header file, being distributed by Microsoft as a part of SDK. Contains `POWER_INFORMATION_LEVEL` enum.

Most of `InformationLevel` values require `SeShutdownPrivilege`, which means it is allowed for each user on Windows 10. If other privilege requirements are identified, it is clearly written in the level description.

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
| 4 | `SystemPowerCapabilities` | Returns [`SYSTEM_POWER_CAPABILITIES`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_power_capabilities) structure. |
| 5 | `SystemBatteryState` | Returns [`SYSTEM_BATTERY_STATE`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_battery_state) structure. |
| 6 | `SystemPowerStateHandler` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 7 | `ProcessorStateHandler` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 8 | `SystemPowerPolicyCurrent` | Returns current system power policy. Uses  [`SYSTEM_POWER_POLICY`](https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_power_policy) structure. |
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
| 23 | `SystemPowerStateLogging` | ? |
| 24 | `SystemPowerLoggingEntry` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 25 | `SetPowerSettingValue` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 26 | `NotifyUserPowerSetting` | ? |
| 27 | `PowerInformationLevelUnused0` | ? |
| 28 | `SystemMonitorHiberBootPowerOff` | ? |
| 29 | `SystemVideoState` | ? |
| 30 | `TraceApplicationPowerMessage` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 31 | `TraceApplicationPowerMessageEnd` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 32 | `ProcessorPerfStates` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 33 | `ProcessorIdleStates` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 34 | `ProcessorCap` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 35 | `SystemWakeSource` | ? |
| 36 | `SystemHiberFileInformation` | ? |
| 37 | `TraceServicePowerMessage` | ? |
| 38 | `ProcessorLoad` | Returns `STATUS_ACCESS_DENIED` for non-admins. Looks like simulated CPU load, but requires more research to confirm. |
| 39 | `PowerShutdownNotification` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 40 | `MonitorCapabilities` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 41 | `SessionPowerInit` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 42 | `SessionDisplayState` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 43 | `PowerRequestCreate` | ? |
| 44 | `PowerRequestAction` | ? |
| 45 | `GetPowerRequestList` | ? |
| 46 | `ProcessorInformationEx` | ? |
| 47 | `NotifyUserModeLegacyPowerEvent` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 48 | `GroupPark` | ? |
| 49 | `ProcessorIdleDomains` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 50 | `WakeTimerList` | ? |
| 51 | `SystemHiberFileSize` | ? |
| 52 | `ProcessorIdleStatesHv` | ? |
| 53 | `ProcessorPerfStatesHv` | ? |
| 54 | `ProcessorPerfCapHv` | ? |
| 55 | `ProcessorSetIdle` | ? |
| 56 | `LogicalProcessorIdling` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 57 | `UserPresence` | ? |
| 58 | `PowerSettingNotificationName` | ? |
| 59 | `GetPowerSettingValue` | ? |
| 60 | `IdleResiliency` | ? |
| 61 | `SessionRITState` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 62 | `SessionConnectNotification` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 63 | `SessionPowerCleanup` | ? |
| 64 | `SessionLockState` | ? |
| 65 | `SystemHiberbootState` | ? |
| 66 | `PlatformInformation` | ? |
| 67 | `PdcInvocation` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 68 | `MonitorInvocation` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 69 | `FirmwareTableInformationRegistered` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 70 | `SetShutdownSelectedTime` | ? |
| 71 | `SuspendResumeInvocation` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 72 | `PlmPowerRequestCreate` | ? |
| 73 | `ScreenOff` | ? |
| 74 | `CsDeviceNotification` | ? |
| 75 | `PlatformRole` | ? |
| 76 | `LastResumePerformance` | ? |
| 77 | `DisplayBurst` | ? |
| 78 | `ExitLatencySamplingPercentage` | ? |
| 79 | `RegisterSpmPowerSettings` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 80 | `PlatformIdleStates` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 81 | `ProcessorIdleVeto` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 82 | `PlatformIdleVeto` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 83 | `SystemBatteryStatePrecise` | ? |
| 84 | `ThermalEvent` | ? |
| 85 | `PowerRequestActionInternal` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 86 | `BatteryDeviceState` | ? |
| 87 | `PowerInformationInternal` | ? |
| 88 | `ThermalStandby` | ? |
| 89 | `SystemHiberFileType` | ? |
| 90 | `PhysicalPowerButtonPress` | ? |
| 91 | `QueryPotentialDripsConstraint` | Kernel mode only? Returns `STATUS_ACCESS_DENIED` when called from user mode. |
| 92 | `EnergyTrackerCreate` | ? |
| 93 | `EnergyTrackerQuery` | ? |
| 94 | `UpdateBlackBoxRecorder` | ? |
| 95 | `PowerInformationLevelMaximum` | Not used. Marks the end of list. |
