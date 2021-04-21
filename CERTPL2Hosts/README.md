**\[PL\]**:
[CERT Polska](https://www.cert.pl/) utrzymuje listę hostów, które służą do rozmaitych ataków phishingowych itp. Wielu administratorów "wycina" te adresy na DNSach, sprzęcie sieciowym itp, jednak równie skutecznie można to wykonać na poziomie lokalnego pliku hosts. Metoda ta jest wygodna zwłaszcza w przypadku pojedynczych komputerów, pracujących w sieci nie wyposażonej w zaawansowaną infrastrukturę, i obsługiwanych na codzień przez osoby mogące być ofiarami ataków. Wpisane do pliku hosts adresy z CERT Polska, zamiast do atakującego, prowadzić będą na dedykowaną stronę CERT, zawierającą ostrzeżenie o szkodliwym charakterze danej witryny.

Prosty skrypt `Update-CERTHosts.ps1` napisany w PowerShell pozwala na zautomatyzowanie procesu pobierania list z CERT i dodawania ich do pliku hosts. Skrypt `Install-CERTHost.ps1` automatyzuje instalację i ustawia *Scheduled Task* automatycznie wykonujący aktualizację ~~co godzinę~~ co dziesięć minut, zgodnie z zaleceniami CERT.

**Instalacja zautomatyzowana** - wymaga zaufania (lub sprawdzenia), że skrypt spod wskazanego URL nie zrobi nic złego:
1. Uruchom PowerShell lub PowerShell ISE z uprawnieniami administratora
1. Wykonaj polecenie (jedna linia): `iex (iwr 'https://raw.githubusercontent.com/gtworek/PSBits/master/CERTPL2Hosts/Install-CERTHosts.ps1' -DisableKeepAlive -UseBasicParsing).Content`

**Instalacja półautomatyczna** - daje szansę na obejrzenie skryptu przed jego uruchomieniem:
1. Uruchom PowerShell lub PowerShell ISE z uprawnieniami administratora
1. Uruchom skrypt znajdujący się na https://github.com/gtworek/PSBits/blob/master/CERTPL2Hosts/Install-CERTHosts.ps1

**Instalacja ręczna** - tylko skrypt aktualizujący wymaga uruchomienia, można go sprawdzić/zmodyfikować przed utworzeniem *Taska*:
1. Skopiuj skrypt znajdujący się na https://github.com/gtworek/PSBits/blob/master/CERTPL2Hosts/Update-CERTHosts.ps1 do lokalizacji, do której użytkownik bez praw administratora nie ma praw zapisu. Na przykład do `C:\Program Files\`.
1. Utwórz *Scheduled Task* wykonujący skrypt z żądaną częstotliwością, działający na prawach administratora lub systemu.

**Odinstalowanie** - jeżeli ktoś naprawdę potrzebuje, albo po prostu chce wiedzieć:
1. Z Menu Start uruchom Task Scheduler (Harmonogram zadań).
1. Na wyświetlonej liście znajdź pozycję `CERT.PL do hosts`.
1. Kliknij prawym i usuń lub wyłącz zadanie.
1. Usuń adresy dodane do pliku hosts:
    1. Uruchom cmd.exe lub PowerShell z uprawnieniami administratora,
    1. W cmd.exe lub PowerShell uruchom `notepad C:\Windows\System32\drivers\etc\hosts`,
    1. Znajdź linię `### Start of https://hole.cert.pl/domains/domains_hosts.txt content ###` (zwykle około dwudziestej linii w pliku),
    1. Znajdź linię `### End of https://hole.cert.pl/domains/domains_hosts.txt content ###` (zwykle na końcu pliku),
    1. Usuń te linie i całą zawartość pomiędzy nimi,
    1. Zapisz zmodyfikowany plik hosts,
1. Możesz również usunąć plik `Update-CERTHosts.ps1` z `C:\Program Files`.
1. Opcjonalnie, możesz posłużyć się kopią zapasową pliku hosts, wykonaną automatycznie przy pierwszym uruchomieniu skryptu i dostępną w pliku `C:\Windows\System32\drivers\etc\hosts_holecert.bak`

**\[EN\]**: as these scripts rely on cert.pl data, useful only for Polish users, I am providing the description only in Polish. Scripts are relatively simple, though.
