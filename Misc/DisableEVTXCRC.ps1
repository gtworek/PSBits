# evtx files are heavily protected with CRC32. If you change something at the binary level and do not re-calculate the content, it will not load anymore.
# the script demonstrates an undocummented "disable integrity check" flag in the evtx files

# copy will require admin permissions as regular users have no access to the default evtx location
# feel free to use any other evtx file

$fileName = "C:\Temp\System.evtx"
Copy-Item C:\Windows\System32\winevt\Logs\System.evtx $fileName


# step 1 - open the file and overwrite the CRC with 0 to make it fail

$fs = new-object IO.FileStream($fileName, [IO.FileMode]::Open)
$writer = new-object IO.BinaryWriter($fs)
$writer.BaseStream.Seek(124,"Begin") | Out-Null #set to the proper offset
$writer.Write([byte[]](0,0,0,0))
$writer.Close()


#####################
# try to open the file right now. It should fail as the checksum is not valid
#####################


# step 2 - change the bit, to make checksum ignored

$fs = new-object IO.FileStream($fileName, [IO.FileMode]::Open)
$writer = new-object IO.BinaryWriter($fs)
$writer.BaseStream.Seek(120,"Begin") | Out-Null #set to the proper offset
$writer.Write([byte](4))
$writer.Close()


#####################
# try to open the file right now. It should work. 
#####################