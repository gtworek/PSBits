*VSS* aka *Volume Shadow Copy Service* relies on a driver waiting for control codes sent through [`DeviceIoControl()`](https://docs.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-deviceiocontrol). One of these codes (undocumented, but referred usually as `IOCTL_VOLSNAP_SET_MAX_DIFF_AREA_SIZE`, allows the sender to resize storage size. Too small storage effectively means "*no storage at all*" because there is no possibility to fit a complete snapshot in the space provided.

In practice it means that one single `DeviceIoControl()` call removes all existing volume snapshots.

It is not directly dangerous, as only admins can make such call, and admins have a lot of other possibilities to permanently delete snapshots anyway.
