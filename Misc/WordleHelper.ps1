if ($null -eq $wordlist5)
{
    $wordlistURL = "https://github.com/dwyl/english-words/raw/master/words_alpha.txt" #other lists may require slightly different processing. 
    $wordlistRAW = Invoke-WebRequest -Uri $wordlistURL -UseBasicParsing
    $wordlist = $wordlistRAW.Content -split "\r\n" 
    Write-Host $wordlist.Count "words downloaded. Processing..."

    $wordlist5 = [System.Collections.ArrayList]@()
    foreach ($word in $wordlist)
    {
        if (($word.Trim()).Length -eq 5)
        {
            $null = $wordlist5.Add($word.ToUpper())
        }
    }
}
$wordlist = $wordlist5

for ($j = 0; $j -lt 5; $j++)
{

    Write-Host $wordlist.Count "words matching."
    if ("Y" -eq (Read-Host "Want to see them all? [Y/N]"))
    {
        if (Test-Path Variable:PSise)
        {
            $wordlist | Out-GridView
            }
        else
        {
            $wordlist | Format-Table
        }
    }

    $guessOK = $false
    while (!$guessOK)
    {
        $guess = Read-Host -Prompt "Type your guess"
        if ($guess.Length -eq 5)
        {
            $guessOK = $true
        }
        else
        {
            Write-Host "Use 5-letters word!" -ForegroundColor Red
        }
    }

    $guess = $guess.ToUpper()

    if (!$wordlist.Contains($guess))
    {
        Write-Host "Your word list doesn't contain '$guess' ..." -ForegroundColor Yellow
    }

    $answerOK = $false
    while (!$answerOK)
    {
        $answer = Read-Host -Prompt "Type wordle answer (B=black, Y=yellow, G=green)"
        $answer = $answer.ToUpper()
        if ($answer.Length -ne 5)
        {
            Write-Host "Use 5-letters answer!" -ForegroundColor Red
            continue
        }
        $answerOK = $true
        foreach ($letter in $answer.ToCharArray())
        {
            if (('B','Y','G') -notcontains $letter)
            {
                $answerOK = $false
                Write-Host "Use only 'B', 'Y', and 'G' in Wordle answer!" -ForegroundColor Red
            }
        }
    }

    for ($i = 0; $i -lt 5; $i++)
    {
        $bc = switch (($answer.ToCharArray())[$i])
        {
            "B" {[System.ConsoleColor]::Black}
            "Y" {[System.ConsoleColor]::Yellow}
            "G" {[System.ConsoleColor]::Green}
            default {[System.ConsoleColor]::Red}
        }
        Write-Host ($guess.ToCharArray())[$i] -NoNewline -ForegroundColor White -BackgroundColor $bc
    }
    Write-Host
    Write-Host "Analyzing..."
    Write-Host

    for ($i = 0; $i -lt 5; $i++)
    {
        $newWordlist = [System.Collections.ArrayList]@()
        $guessLetter = ($guess.ToCharArray())[$i]
        $answerLetter = ($answer.ToCharArray())[$i]
        switch ($answerLetter)
        {
            "B" {
                    foreach ($word in $wordlist)
                    {
                        if ($word.ToCharArray() -notcontains $guessLetter)
                        {
                            $null = $newWordlist.Add($word)
                        }
                    }
                    break
                }
            "Y" {
                    foreach ($word in $wordlist)
                    {
                        if ($word.ToCharArray() -contains $guessLetter)
                        {
                            if ($word.ToCharArray()[$i] -ne $guessLetter) #it would be green
                            {
                                $null = $newWordlist.Add($word)
                            }
                        }
                    }
                    break
                }
            "G" {
                    foreach ($word in $wordlist)
                    {
                        if ($word.ToCharArray()[$i] -eq $guessLetter)
                        {
                            $null = $newWordlist.Add($word)
                        }
                    }
                    break
                }
        }
        $wordlist = $newWordlist
    }
}
