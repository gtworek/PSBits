If you play with the NTFS USN Journal, you could spot "`enableRangeTracking`" feature and/or fsutil.exe switch. Does it look like something useful in DFIR scenarios? What it is all about?

The idea of USN Journal allows synchronously run applications to know what changed since the last run. An application does not have to be active all the time and can focus only on the latest changes, without scanning the entire volume. Changed data can then be analyzed, indexed, synchronized etc. And what if only a relatively small part of the large file changed? Traditionally, the entire file had to be processed again. And here the `enableRangeTracking` jumps in: next to the information about the file modification, an information about the changed part of the file appears within the Journal. If the tracking application is smart enough to understand such additional information, it may focus only on the changed part, without re-scanning an entire file, which sounds exactly like "better performance".

To enable the Range Tracking feature, you need to send an [`FSCTL_USN_TRACK_MODIFIED_RANGES`](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_usn_track_modified_ranges) IOCTL to the volume, providing an [`USN_TRACK_MODIFIED_RANGES`](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-usn_track_modified_ranges) structure, specifying chunk size and file size threshold. 

Chunk size tells the NTFS driver what is the granularity of tracking - if only a single byte is changed, the information about the entire chunk modification is stored. Larger size means less records, but also less precise information about the change. Smaller chunks lead to a flood of records about changed ranges. The `FileSizeThreshold` provides information about ignored files. If the file size is smaller than the threshold, range tracking information is not stored at all. 
If we focus on the purpose of USN journal (to let applications know something has changed), it makes no sense to store range information about each change. For small files, re-processing the entire file will be faster than digging into details, not to mention the space required for the USN Journal storage.

If you enable Range Tracking with `fsutil.exe` command, you can specify `ChunkSize`, and the `FileSizeThreshold` as `c=...` and `s=...` parameters respectively. What is not clearly documented is that there are some limits for these values: chunk cannot be smaller than 16KB and larger than 4GB. The size threshold minimal value is 1MB. If you specify anything not fitting within these boundaries, ntfs.sys will silently fix it to the acceptable value.

After you enable Range Tracking, the USN Journal starts recording [`USN_RECORD_V4`](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-usn_record_v4) structures, specifying details about chunk changes. What is interesting is that such records don’t contain any information about timestamp, when the change happened. To get such information, you should read it from the corresponding record describing the entire file change, as such records ([`USN_RECORD_V3`](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-usn_record_v3)) are stored, regardless of Range Tracking.

Couple of trivia related to Range Tracking:
- Range Tracking is not enabled by default,
- The Range Tracking feature was introduced in 2012R2/8.1,
- You need to have SeManageVolumePrivilege in your token to send `FSCTL_USN_TRACK_MODIFIED_RANGES` to a volume,
- once you enable Range Tracking, there is no way to disable it. You must delete USN Journal and re-create it.
- The data about tracked ranges (`USN_RECORD_V4` records) are not stored with other records, and go to the `$Max` attribute. You can observe it e.g. by issuing the "`fsutil.exe file layout c:\$Extend\$UsnJrnl:$J`" command.
- You can read V4 records with "`fsutil.exe usn readJournal ...`"
