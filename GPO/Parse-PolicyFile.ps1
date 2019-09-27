$polregtypes = ("REG_NONE","REG_SZ","REG_EXPAND_SZ","REG_BINARY","REG_DWORD","REG_DWORD_BIG_ENDIAN","REG_LINK","REG_MULTI_SZ","REG_RESOURCE_LIST","REG_FULL_RESOURCE_DESCRIPTOR","REG_RESOURCE_REQUIREMENTS_LIST","REG_QWORD")
$polblobsize = 1024 #limit of data size before we stop trying to display data and simply say "(BLOB)"

if ($file -eq $null)
    {
    $polFile = (dir *.pol)[0]
    }
else
    {
    $polFile = $file
    }
$arrtmp2=@()

if (Test-Path ($polFile.FullName))
    {
    $polbytes = [System.IO.File]::ReadAllBytes($polFile.FullName)
    if (([System.Text.Encoding]::ASCII.GetString($polbytes, 0, 4)) -cne "PReg")
        {
        Write-Host ("[??? POL] Not looking like POL file "+$polFile.FullName) -ForegroundColor Red
        $polrow = New-Object psobject
        $polrow | Add-Member -Name "FullName" -MemberType NoteProperty -Value $polFile.FullName
        $polrow | Add-Member -Name "Hive" -MemberType NoteProperty -Value "???"
        $polrow | Add-Member -Name "Key" -MemberType NoteProperty -Value "???"
        $polrow | Add-Member -Name "Value" -MemberType NoteProperty -Value "???"
        $polrow | Add-Member -Name "Data" -MemberType NoteProperty -Value "???"
        }
    else
        {
        #Hive (HKLM/HKCU) to be determined from name, ? for unknown. stupid but works
        $POLHive = "?"
        if ($polFile.FullName -like "*}_Machine_Registry.pol")
            {
            $POLHive = "HKLM"
            }
        if ($polFile.FullName -like "*}_User_Registry.pol")
            {
            $POLHive = "HKCU"
            }
        #[key;value;type;size;data] 
        $polbytes = $polbytes[8..($polbytes.Count)]
        $PolStrings = [System.Text.Encoding]::Unicode.GetString($polbytes)
        
        $polip=0
        $polstrtmp=""

            $polrow = New-Object psobject
            $polrow | Add-Member -Name "FullName" -MemberType NoteProperty -Value $polFile.FullName
            $polrow | Add-Member -Name "Hive" -MemberType NoteProperty -Value $POLHive
            $polrow | Add-Member -Name "Key" -MemberType NoteProperty -Value "??"
            $polrow | Add-Member -Name "Value" -MemberType NoteProperty -Value "??"
            $polrow | Add-Member -Name "Type" -MemberType NoteProperty -Value "??"
            $polrow | Add-Member -Name "Data" -MemberType NoteProperty -Value "??"
            $polip2 = 0 #field

        while ($true)
            {
                switch ($polip2)
                    {
                    0   #key
                        {
                        if (($polbytes[0+$polip] -eq 0) -and ($polbytes[1+$polip] -eq 0) -and ($polbytes[2+$polip] -eq 59) -and ($polbytes[3+$polip] -eq 0)) #00;0
                            {
                            if ($polstrtmp[0] -eq "[") 
                                {
                                $polstrtmp = $polstrtmp.Substring(1)
                                }
                            $polrow.Key = $polstrtmp
                            $polstrtmp = ""
                            $polip2 = 1
                            $polip += 2 #skip ";"
                            }
                        else 
                            {
                            $polstrtmp += [System.Text.Encoding]::Unicode.GetString($polbytes[(0+$polip)..(1+$polip)])
                            }
                        break
                        } #1

                    1   #value
                        {
                        if ($polbytes[0+$polip] -or $polbytes[1+$polip])                                                    
                            {
                            $polstrtmp += [System.Text.Encoding]::Unicode.GetString($polbytes[(0+$polip)..(1+$polip)])
                            }
                        else #weve got data, save
                            {
                            $polrow.Value = $polstrtmp
                            $polstrtmp = ""
                            $polip2 = 2
                            $polip += 2 #skip ";"
                            }
                        break
                        } #1

                    2   #type
                        {
                        if (($polbytes[4+$polip] -eq 59) -and ($polbytes[5+$polip] -eq 0)) #....;0 
                            {
                            if (($polbytes[$polip+1]) -or ($polbytes[$polip+2]) -or ($polbytes[$polip+3])) 
                                {
                                Write-Host "[??? POL] Non-zero MSB for type." -ForegroundColor Red
                                }
                            $polrow.Type = $polregtypes[$polbytes[$polip]]
                            $polstrtmp = ""
                            $polip2 = 3
                            $polip += 4 #skip "00;0"
                            }
                        else
                            {
                            Write-Host "[??? POL] Unusually long type field." -ForegroundColor Red
                            }
                        break
                        } #2

                    3   #size
                        {
                        if (($polbytes[4+$polip] -eq 59) -and ($polbytes[5+$polip] -eq 0)) #....;0 
                            {
                            $poldatasize = 256*256*256*$polbytes[3+$polip] + 256*256*$polbytes[2+$polip] + 256*$polbytes[1+$polip] + $polbytes[$polip]
                            $polstrtmp = ""
                            $polip2 = 4
                            $polip += 4 #skip "..;0"
                            }
                        else
                            {
                            Write-Host "[??? POL] Unusually long size field." -ForegroundColor Red
                            }
                        break
                        } #3

                    4   #data
                        {
                        $poldata = $polbytes[($polip)..($polip+$poldatasize-1)]
                        if ($poldatasize -gt $polblobsize)
                            {
                            $polrow.Data = "(BLOB)"
                            }
                        else
                            {
                            if (($polrow.Type -eq "REG_MULTI_SZ") -or ($polrow.Type -eq "REG_SZ") -or ($polrow.Type -eq "REG_EXPAND_SZ"))
                                {
                                while (!(($poldata[($poldata.Count-1)]) -or ($poldata[($poldata.Count-2)]))) #if double \0 at the end
                                    {
                                    $poldata = $poldata[0..($poldata.Count-3)]
                                    if ($poldata.Count -le 4)
                                        {
                                        break
                                        }
                                    }
                                <#
                                $poldata = $poldata[0..($poldata.Count-3)]
                                if ($polrow.Type -eq "REG_MULTI_SZ") #multi is terminated with double\0 Requires more research
                                    {
                                    $poldata = $poldata[0..($poldata.Count-3)]
                                    }
                                #>
                                $polrow.Data = ([System.Text.Encoding]::Unicode.GetString($poldata))
                                }
                            if ($polrow.Type -eq "REG_DWORD")
                                {
                                $polrow.Data = ("0x"+(($poldata[3..0]|ForEach-Object ToString X2) -join ''))
                                }
                            }
                        $polip += $poldatasize #skip
                        if ($polbytes[$polip] -ne 93) #simple check if we point to the closing "]"
                            {
                            Write-Host "[??? POL] Something strange happened." -ForegroundColor Red
                            }
                        $arrtmp2 += $polrow
                        $polrow = New-Object psobject #re-init data for row
                        $polrow | Add-Member -Name "FullName" -MemberType NoteProperty -Value $polFile.FullName
                        $polrow | Add-Member -Name "Hive" -MemberType NoteProperty -Value $POLHive
                        $polrow | Add-Member -Name "Key" -MemberType NoteProperty -Value "??"
                        $polrow | Add-Member -Name "Value" -MemberType NoteProperty -Value "??"
                        $polrow | Add-Member -Name "Type" -MemberType NoteProperty -Value "??"
                        $polrow | Add-Member -Name "Data" -MemberType NoteProperty -Value "??"
                        $polip2 = 0 #reset field number
                        break
                        } #4

                    default 
                        {
                        Write-Host "[??? POL] Something strange happened." -ForegroundColor Red
                        } #default
                    } #switch

            $polip += 2
            if ($polbytes.Count -le $polip)
                {
                break
                }
            } #while
        } #else PReg
        if ($file -eq $null)
            {
            $arrtmp2 | Out-GridView
            }
    } #file exists
else
    {
    Write-Host ("[??? POL] Cannot find the file for parsing") -ForegroundColor Red
    }
