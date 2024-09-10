# The function returns an array of exports starting with a name of module. 
# It is useful when none of exported functions has a name (e.g. cscript.exe)
# Be careful. It's more like PoC and may not cover all corner cases.

function Get-Exports ($fileNameParam)
{
    $DebugPreference = "Continue"

    function Array2Ulong([byte[]]$b)
    {
        [uint32]$f =     ([uint32]$b[3] -shl 24) `
                    -bor ([uint32]$b[2] -shl 16) `
                    -bor ([uint32]$b[1] -shl  8) `
                    -bor ([uint32]$b[0])
        return $f
    }

    function Rva2Foa($rvaParam)
    {
        for ($i = 0; $i -lt $numberOfSections; $i++)
        {
            if (($rvaParam -ge $sectionHeadersVA[$i]) -and ($rvaParam -le $sectionHeadersVA[$i] + $sectionHeadersSORD[$i]))
            {
                return $sectionHeadersPTRD[$i] + $rvaParam - $sectionHeadersVA[$i]
            }
        }
        return 0
    }

    $fs = new-object IO.FileStream($fileNameParam, [IO.FileMode]::Open, [System.IO.FileAccess]::Read)
    $reader = new-object IO.BinaryReader($fs)

    $readBufSize = 2048 # sections header are the fattest thing to fit here
    $readBuf1 = New-Object byte[] $readBufSize #general purpose buffer

    [void]$reader.BaseStream.Seek(0,"Begin")
    [void]$reader.Read($readBuf1, 0, $readBufSize)
    if (($readBuf1[0] -ne 77) -or ($readBuf1[1] -ne 90))
    {
        Write-Host 'Wrong DosHeader'
        return
    }

    $addressOfNewExeHeader = Array2Ulong($readBuf1[0x3C..0x3F])
    Write-Debug ("NtHeader address: $addressOfNewExeHeader")

    [void]$reader.BaseStream.Seek($addressOfNewExeHeader,"Begin")
    [void]$reader.Read($readBuf1, 0, $readBufSize)

    if (($readBuf1[0] -ne 80) -or ($readBuf1[1] -ne 69))
    {
        Write-Host 'Wrong NtHeader'
        return
    }

    $numberOfSections = ($readBuf1[0x07] -shl 8) + $readBuf1[0x06]
    Write-Debug ("Number of sections: $numberOfSections")

    $sizeOfOptionalHeader = ($readBuf1[0x15] -shl 8) + $readBuf1[0x14]
    Write-Debug ("Optional header size: $sizeOfOptionalHeader")

    if ($sizeOfOptionalHeader -eq 0)
    {
        Write-Host 'No optional header'
        return
    }

    if (($readBuf1[0x18] -ne 0x0B) -or ($readBuf1[0x19] -ne 0x02))
    {
        Write-Host 'PE64 magic mismatch'
        return
    }

    $exportVA = Array2Ulong($readBuf1[0x88..0x8B])
    Write-Debug ("Export VA: $exportVA")

    $exportSize = Array2Ulong($readBuf1[0x8C..0x8F])
    Write-Debug ("Export size: $exportSize")

    if (($exportSize -eq 0) -or ($exportVA -eq 0))
    {
        Write-Host 'No exports'
        return
    }


    $startOfSectionHeaders = $AddressOfNewExeHeader + 0x04 + 0x14 + $sizeOfOptionalHeader
    [void]$reader.BaseStream.Seek($startOfSectionHeaders,"Begin")
    [void]$reader.Read($readBuf1, 0, $readBufSize)


    $sectionHeadersVA = @()
    $sectionHeadersSORD = @()
    $sectionHeadersPTRD = @()

    for ($i = 0; $i -lt $numberOfSections; $i++)
    {
        $sectionHeadersVA += Array2Ulong($readBuf1[($i*0x28+0x0C)..($i*0x28+0x0F)])
        $sectionHeadersSORD += Array2Ulong($readBuf1[($i*0x28+0x10)..($i*0x28+0x13)])
        $sectionHeadersPTRD += Array2Ulong($readBuf1[($i*0x28+0x14)..($i*0x28+0x17)])
    }


    $exportFOA = Rva2Foa($exportVA)
    if ($exportFOA -eq 0)
    {
        Write-Host 'Can''t convert RVA to file offset'
        return
    }


    $readBuf2 = New-Object byte[] $exportSize # buffer for exports
    [void]$reader.BaseStream.Seek($exportFOA,"Begin")
    [void]$reader.Read($readBuf2, 0, $exportSize)

    $namesRva = Array2Ulong($readBuf2[0x0C..0x0F])
    Write-Debug ("Names RVA: $namesRva")
    $namesFoa = Rva2Foa($namesRva)
    Write-Debug ("Names FOA: $namesFoa")

    $numberOfNames = Array2Ulong($readBuf2[0x18..0x1B])
    Write-Debug ("Number of names: $numberOfNames")

    $buf2Offset = $namesFoa - $exportFOA

    $iExportNum = 0
    $namesArr = @()
    $nameStr = ''
    while ($iExportNum -le $numberOfNames)
    {
        $currentByte = $readBuf2[$buf2Offset]
        if ($currentByte -ne 0)
        {
            $nameStr += [char]$currentByte
        }
        else
        {
            Write-Debug $nameStr
            $namesArr += $nameStr
            $nameStr =''
            $iExportNum++
        }
        $buf2Offset++
    }


    $reader.Close()
    $fs.Close()

    return $namesArr
}


Get-Exports C:\Windows\System32\netsh.exe
