# Offline GPO Analysis

Some theory behind the scene is described in [this article](https://medium.com/@grzegorztworek/gpo-group-policy-object-is-one-of-the-most-useful-features-of-the-windows-ecosystem-73b6eeab812).

## Matching GUIDs with GPO names

If you have user permissions in AD, you can map GUIDs to GPO names. Here is the procedure:
1. Export your GPOs to CSV (`Get-GPO -all | Export-Csv -Path gpos.csv`)
1. Open the csv file exported by the *Parse-GPO.ps1* script in Excel
1. Insert gpos.csv to a new sheet (Sheet1)
1. Insert a column B in the result of the script
1. Fill up cells of your B column with `=VLOOKUP(MID(A3,FIND("{",A3)+1,36),Sheet1!$A:$B,2,FALSE)`

Of course you can do the same in PowerShell if you prefer. Up to you :)

*GPOPermissionsCollector.ps1* - collects permissions for GPOs and allows you to find anomalies. Requires domain access.
