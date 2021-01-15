**\[PL\]**:
[CERT Polska](https://www.cert.pl/) utrzymuje listę hostów, które służą do rozmaitych ataków phishingowych itp. Wielu administratorów "wycina" te adresy na DNSach, sprzęcie sieciowym itp, jednak równie skutecznie można to wykonać na poziomie lokalnego pliku hosts. Metoda ta jest wygodna zwłaszcza w przypadku pojedynczych komputerów, pracujących w sieci nie wyposażonej w zaawansowaną infrastrukturę, i obsługiwanych na codzień przez osoby mogące być ofiarami ataków. Wpisane do pliku hosts adresy z CERT Polska, zamiast do atakującego, prowadzić będą na dedykowaną stronę CERT, zawierającą ostrzeżenie o szkodliwym charakterze danej witryny.

Prosty skrypt `Update-CERTHosts.ps1` napisany w PowerShell pozwala na zautomatyzowanie procesu pobierania list z CERT i dodawania ich do pliku hosts. Skrypt `Install-CERTHost.ps1` automatyzuje instalację i ustawia *Scheduled Task* automatycznie wykonujący aktualizację co godzinę.

Instalacja zautomatyzowana:
1. Uruchom PowerShell lub PowerShell ISE z uprawnieniami administratora
1. Wykonaj polecenie `iex (iwr 'https://raw.githubusercontent.com/gtworek/PSBits/master/CERTPL2Hosts/Install-CERTHosts.ps1')`

Instalacja półautomatyczna:
1. Uruchom PowerShell lub PowerShell ISE z uprawnieniami administratora
1. Uruchom skrypt znajdujący się na https://github.com/gtworek/PSBits/blob/master/CERTPL2Hosts/Install-CERTHosts.ps1

Instalacja ręczna:
1. Skopiuj skrypt znajdujący się na https://github.com/gtworek/PSBits/blob/master/CERTPL2Hosts/Update-CERTHosts.ps1 do lokalizacji, do której użytkownik bez praw administratora nie ma praw zapisu. Na przykład do `C:\Program Files\`.
1. Utwórz *Scheduled Task* wykonujący skrypt z żądaną częstotliwością, działający na prawach administratora lub systemu.

**\[EN\]**: as these scripts rely on cert.pl data, useful only for Polish users, I am providing the description only in Polish. Scripts are relatively simple, though.
