$imgFile = (Get-ChildItem Env:\Temp).Value+"\dududodomu.png"

$days = 3652
$hours = 24
$startDate = [Datetime]::ParseExact('2015.08.06', 'yyyy.MM.dd', $null)

$endDate = $startDate.AddDays($days)
$ts = New-TimeSpan -Start ([Datetime]::Now) -End $endDate

$bmBM = New-Object System.Drawing.Bitmap ($days, $hours)
$graphics = [System.Drawing.Graphics]::FromImage($bmBM) 

$brush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
$graphics.FillRectangle($brush, 0, 0, $days, $hours)

$brush.Color = [System.Drawing.Color]::Green
$graphics.FillRectangle($brush, 0, 0, $ts.Days, $hours)
$graphics.FillRectangle($brush, $ts.Days, 0, 1, $ts.Hours+1)

$brush.Dispose()
$graphics.Dispose()

$bmBM.Save($imgFile)
$bmBM.Dispose()

iex $imgFile
