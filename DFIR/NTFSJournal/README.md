I wanted to dump NTFS Journal, and import it later for some automated analysis. 
This is perfectly possible with `fsutil.exe usn readjournal c: csv` but date and time is localized. 
I like it, when I read some text, and not in scripts.

Then I have tried with PowerShell (take a look at the brilliant [PoshUSNJournal](https://www.powershellgallery.com/packages/PoshUSNJournal/0.4.3.0) by Boe Prox) but it is too slow and eats a lot of memory.

I have finally decided to write it in C. For flexibility, performance and FUN.
Enjoy the C code, and the compiled exe, as usual.

More coming some day!
