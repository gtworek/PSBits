# Simple script for decoding vbe/jse file content. You need to ignore first 12 chars: '#@~^' mark + 8 chars of Base64 encoded length.
# I am not the first one doing it, but I went slightly different way. Plus it's PowerShell and not Python. ¯\_(ツ)_/¯

# Data enters here:
$EncodedString = "6`tP3MDKDP""+k;:/PH+XY@&"

# Magic happens here:
$cstr = @()
$cstr += "`tWf3E""zy`$BJLg6OtZ!GoFKPT08b)Aw\M*4hm_?^c(I;uHN}Q+2pX5q19D]R-v'&/Cax=~ .riUneY%jS,[dsl:#Vk|``{7"
$cstr += "`t{_+e*5YthwpQWog?SLjU~,9J""!0fDrO-\v6xnP 2Eu|KT]%8}ZCHMG7^`$[&3ki;qIR.Vla#):mc4bAB'=``(Xyz/sF1Nd"
$cstr += "`tnKoXxJ/e!u%^l5Q?~B, -|_Hy23+6f`$qW4hP{=:TgNjVGDtMr""R81c.zAE'\Z0ksmap;C*)}O9dL&IY][#``USwF7v(bi"
$carr = @(0,2,1,0,2,1,2,1,1,2,1,2,0,1,2,1,0,1,2,1,0,0,2,1,1,2,0,1,2,1,1,2,0,0,1,2,1,2,1,0,1,0,0,2,1,0,1,2,0,1,2,1,0,0,2,1,1,0,0,2,1,0,1,2)

$DecodedString = ""

$es = $EncodedString.Replace('@&',"`n")
$es = $es.Replace('@#',"`r")
$es = $es.Replace('@*','>')
$es = $es.Replace('@!','<')
$es = $es.Replace('@$','@')

for ($i = 0; $i -lt $es.Length; $i++)
{
    $ac = $carr[$i % ($carr.Count)]
    $ec = $es[$i]
    $pc = $cstr[$ac].IndexOf($ec)
    $dc = ($cstr[$ac][($pc + 1) % $cstr[$ac].Length], $ec)[!($pc + 1)]
    $DecodedString += $dc
}

# Output leaves this way:
Write-Host $DecodedString
