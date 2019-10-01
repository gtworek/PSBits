Some theory behind the scene is described in [this article](https://medium.com/@grzegorztworek/gpo-group-policy-object-is-one-of-the-most-useful-features-of-the-windows-ecosystem-73b6eeab812)

**Matching GUIDs with GPO names** <br>
If you have user permissions in the AD, you can map GUIDs to GPO names. Here comes the procedure:
1. export your GPOs to CSV (`Get-GPO -all | Export-Csv -Path gpos.csv`)
2. open the csv file exported by the Parse-GPO.ps1 script in excel
3. insert gpos.csv to a new sheet (Sheet1)
4. insert a column B in the result of the script
5. fill up cells of your B column with `=VLOOKUP(MID(A3,FIND("{",A3)+1,36),Sheet1!$A:$B,2,FALSE)`

Of course you can do the same in PowerShell if you prefer. Up to you :)
