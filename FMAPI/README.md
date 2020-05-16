FMAPI is documented at https://docs.microsoft.com/en-us/previous-versions/windows/desktop/fmapi/file-management-api--fmapi- 
It is a set of functions intended to find and undelete deleted files and folders. It is built into Windows (through fmapi.dll), but the API itself checks if you are using it within WinPE (where it works) or within the regular OS (and then returns error code #50).

The trick here is to fool FMAPI, making it thinking it is running under WinPE. And that's all. Feel free to play with the code provided.

TODO:
- [ ] Modify the way how registry changes are made.
