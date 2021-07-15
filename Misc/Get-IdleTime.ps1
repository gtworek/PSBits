# one-liner getting the time all users are idle (not only the current session!)
# it relies on KUSER_SHARED_DATA.LastSystemRITEventTickCount
# be careful, as:
#  1. the LastSystemRITEventTickCount is updated once per second, so you can observe some 0-1000 values, even if users are active.
#  2. it uses uint32 by design, so values overflow every ~49 days.

$milliSecondsIdle =  [uint32][Environment]::TickCount - [uint32]([System.Runtime.InteropServices.Marshal]::ReadInt32(0x7ffe02e4))
