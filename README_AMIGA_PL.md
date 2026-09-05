Raptor: Call of the Shadows - Amiga Port (68030/68060 & EC/LC, RTG/AGA, AHI/MHI/CAMD)

Wersja: 0.9.9-rc.1 — ostatnia kompilacja przedpremierowa.

=====================================================

Port gry Raptor: Call of the Shadows na Amigę, oparty na rekonstrukcji
silnika (reverse engineering) projektu open source skynettx/raptor.

Port jest rozwijany wyłącznie z myślą o klasycznych systemach Amiga:
AmigaOS 3.2, procesorze klasy 68060 (pełny 68060 z FPU albo
68EC060/68LC060 z użyciem binarnego raptor_nofpu w wersji soft-float)
lub procesorze 68030, dla którego przeznaczone są dwa osobne, dedykowane
pliki binarne: raptor_030_fpu (68030 z zewnętrznym 68881/68882 FPU)
oraz raptor_030 (68030 bez FPU, soft-float). Renderowanie RTG jest
obsługiwane na każdym systemie z kompatybilną kartą graficzną RTG
(np. CyberVision 64/3D, Picasso IV lub podobną) przez Picasso96 albo
CyberGraphX. Natywne renderowanie AGA (GFX=AGA) to osobna, pełna
ścieżka renderowania — nie fallback RTG — dostępna na sprzęcie
z AGA (Amiga 1200, Amiga 4000), a także do wyboru w WinUAE oraz
zgodnych konfiguracjach PiStorm/Emu68, gdy AGA jest dostępne.
Działa również na dowolnej Amidze z PiStorm/Emu68 (A500, A600, A1200,
A2000 itp.), gdzie według konfiguracji środowiska można wybrać RTG
albo AGA. Gra renderuje obraz w rozdzielczości 320x200, z paletą
8-bitową, na dedykowanym ekranie.

UWAGA: To repozytorium NIE zawiera plików danych gry. Potrzebujesz
własnej, legalnej kopii oryginalnych plików gry. Obsługiwana jest
wyłącznie pełna wersja 1.2. Grę można kupić na Steam:
https://store.steampowered.com/app/358360/Raptor_Call_of_the_Shadows_1994_Classic_Edition/


Wymagania
---------

Procesor:   Motorola 68030 z zewnętrznym 68881/68882 FPU albo 68060
            (lub PiStorm / Emu68). Cztery cele budowania:
              raptor         - 68060 + wbudowane FPU (kompilacja z -m68881)
              raptor_nofpu   - 68060 bez FPU (68EC060/68LC060 lub
                               uszkodzone FPU), soft-float
              raptor_030_fpu - 68030 + zewnętrzne 68881/68882 FPU (-m68030)
              raptor_030     - 68030 bez FPU, soft-float
FPU:        Kompilacje raptor oraz raptor_030_fpu WYMAGAJĄ FPU.
            Kompilacje soft-float (raptor_nofpu, raptor_030) wykonują
            operacje zmiennoprzecinkowe programowo przez
            mathieeedoubbas.library (obecną w Kickstart ROM). Warianty
            68030 buduje się za pomocą:
              build_amiga_030.sh       lub  make -f Makefile.amiga CPU=030 NOFPU=1
              build_amiga_030_fpu.sh   lub  make -f Makefile.amiga CPU=030 NOFPU=0
            (kompilację 68060 soft-float buduje się za pomocą
            build_amiga_nofpu.sh / NOFPU=1).
System:     AmigaOS 3.2 (intuition.library v39+, graphics.library v39+)
Pamięć:     Minimum 4 MB Fast RAM (zalecane 8 MB) + 2 MB Chip RAM
            (standardowo). Sama gra wykorzystuje około 3 MB Fast RAM;
            w trybie RTG bitmapa ekranu znajduje się w pamięci karty
            graficznej, więc Chip RAM jest potrzebny wyłącznie systemowi
            operacyjnemu/Workbenchowi.

Grafika:    RTG z Picasso96 albo CyberGraphX (CGX)
            (np. CyberVision 64/3D, Picasso IV, UAEGFX/PiStorm)
            — wymagany tryb 320x200x8
Miejsce:    około 25 MB wolnego miejsca (pliki gry + zapisane gry)
Joystick:   Opcjonalny — port 1 (DB9); wymaga lowlevel.library v40+
            (dołączonej do AmigaOS 3.2); obsługiwane są również pady CD32
Dźwięk:     AHI (ahi.device v4+) dla efektów dźwiękowych; domyślnie
            żaden backend muzyki nie jest włączony (MUSIC=OFF). Muzykę
            można wybrać jawnie przez MUSIC=ADLIB (wbudowana emulacja
            OPL3), MUSIC=CAMD (MIDI przez camd.library), MUSIC=MHI
            (MP3 przez MHI decoder driver) lub MUSIC=WAVE (pliki WAV
            z drawera WAVE/).

Domyślny tryb grafiki (GFX=AUTO / GFX=RTG) wymaga RTG: gra najpierw
próbuje użyć Picasso96, a następnie przełącza się na CyberGraphX (CGX /
cybergraphics.library). Jeśli żadna z tych opcji nie zadziała, zostanie
wyświetlony angielski requester, a gra NIE zostanie uruchomiona (brak
cichego fallbacku). Renderowanie RTG jest obsługiwane na każdym
systemie z kompatybilną kartą graficzną RTG (np. CyberVision 64/3D,
Picasso IV lub podobną). Obsługiwaną i testowaną konfiguracją jest
Picasso96 RTG.

Natywne renderowanie AGA (GFX=AGA) to osobna, pełna ścieżka
renderowania — nie fallback dla RTG. Działa na sprzęcie z AGA
(Amiga 1200, Amiga 4000) i może być również wybrane w WinUAE oraz
zgodnych konfiguracjach PiStorm/Emu68, gdy AGA jest dostępne;
w WinUAE i na PiStorm/Emu68 wybierz RTG albo AGA zgodnie z
konfiguracją systemu.

Prezentacja klatek wykorzystuje najszybszą ścieżkę dostępną dla
aktywnego drivera: p96WritePixelArray w Picasso96, WritePixelArray
w CyberGraphX oraz własny konwerter chunky-to-planar (C2P) dla AGA.
Natywne renderowanie AGA zostało zoptymalizowane dzięki buforowaniu
przekonwertowanej bitmapy AGA używanej przy blitach C2P, co ogranicza
niepotrzebną pracę konwersji, gdy klatka gry nie ulega zmianie.
Aktywna ścieżka jest zapisywana przy uruchomieniu
("[VIDEO] blit path: ...") podczas pracy z Shell (raptor > RAPTOR.LOG).


Instalacja
----------

1. Rozpakuj archiwum do wybranego katalogu, np. Games:.
   Archiwum zawiera gotowy do użycia drawer "Raptor" wraz z własną ikoną:

    Raptor/           drawer gry (z ikoną)
      raptor          główny binarny plik (kompilacja 68060 + FPU), z ikoną
      raptor_nofpu    binarny plik soft-float dla systemów 68060 BEZ FPU
                      (68LC060/68EC060 albo uszkodzone FPU), z ikoną
      raptor_030_fpu  kompilacja 68030 + zewnętrzne 68881/68882 FPU, z ikoną
      raptor_030      binarny plik soft-float dla systemów 68030 BEZ FPU,
                      z ikoną
      README_AMIGA.md ten plik, z ikoną

2. Skopiuj pliki danych oryginalnej gry do drawera Raptor:

   FILE0000.GLB
   FILE0001.GLB
   FILE0002.GLB
   FILE0003.GLB
   FILE0004.GLB

   Wymagane są wszystkie pięć plików. Tylko pełna wersja 1.2 gry
   zawiera je wszystkie. Wielkość liter w nazwach plików nie ma znaczenia.

3. Ten port na Amigę w ogóle nie korzysta z SETUP.INI — wszystkie
   ustawienia mają wbudowane wartości domyślne i można je wybierać
   wyłącznie za pomocą parametrów command line albo ToolTypes ikony
   (GFX=, MUSIC=, NOSOUND itd.). Gra zapisuje save games i profile
   pilotów w katalogu programu, dlatego katalog musi umożliwiać zapis.


Uruchamianie
------------

Z poziomu Shell/CLI uruchom grę po prostu poleceniem:

   raptor

Domyślnie żaden backend muzyki nie jest włączony. Jeżeli nie podano
opcji MUSIC=, Raptor używa MUSIC=OFF i nie inicjalizuje muzyki
AdLib/OPL3, CAMD, MHI ani WAVE. Efekty dźwiękowe pozostają aktywne,
chyba że wybrano NOSOUND. Aby włączyć muzykę, wybierz jawnie
MUSIC=ADLIB, MUSIC=CAMD, MUSIC=MHI lub MUSIC=WAVE.

Efekty dźwiękowe są odtwarzane przez AHI (ahi.device); muzyka gra przez
backend wybrany parametrem MUSIC=. MUSIC=ADLIB używa wbudowanej emulacji
AdLib/OPL3 (autentyczne brzmienie Raptora) miksowanej ze strumieniem
audio AHI. Aby odtwarzać muzykę przez zewnętrzny synthesizer albo
software synth CAMD, uruchom grę z parametrem MUSIC=CAMD. Aby
odtwarzać soundtrack jako pliki MP3 przez MHI decoder driver (np. kartę
Prisma Megamix), użyj MUSIC=MHI. Aby odtwarzać soundtrack jako
predekodowane pliki WAV z drawera WAVE/, użyj MUSIC=WAVE. Szczegóły
oraz konfigurację MIDI/MHI opisano w sekcji „Dźwięk (AHI + CAMD + MHI)”
poniżej.


Dźwięk (AHI + CAMD + MHI)
--------------------------

Efekty dźwiękowe i muzyka korzystają z dwóch niezależnych, natywnych
podsystemów Amigi:

   Efekty dźwiękowe: AHI (ahi.device), 11025 Hz, 16-bit stereo —
                    natywna częstotliwość próbkowania sampli gry —
                    odtwarzane przez standardowy, podwójnie buforowany
                    interfejs device przez dedykowany audio task. AHI v4+
                    musi być zainstalowane (pakiet AHI user, dostępny
                    bezpłatnie w Aminet). Działa każda karta dźwiękowa
                    zgodna z AHI, podobnie jak wbudowany Paula przez
                    tryb audio AHI.

   Muzyka:          Domyślnie żaden backend muzyki nie jest włączony.
                    Jeżeli nie podano opcji MUSIC=, Raptor używa MUSIC=OFF
                    i nie inicjalizuje muzyki AdLib/OPL3, CAMD, MHI ani
                    WAVE. Efekty dźwiękowe pozostają aktywne, chyba że
                    wybrano NOSOUND. Aby włączyć muzykę, wybierz jawnie
                    MUSIC=ADLIB, MUSIC=CAMD, MUSIC=MHI lub MUSIC=WAVE.

                    MUSIC=ADLIB odtwarza utwory MUS przez wbudowaną
                    emulację AdLib/OPL3 (autentyczne brzmienie Raptora),
                    miksowaną ze strumieniem audio AHI. Emulator korzysta
                    z lekkiego rdzenia dbopl z DOSBoxa, który zużywa tylko
                    kilka procent mocy procesora 68060.

                    Alternatywnie parametr MUSIC=CAMD odtwarza utwory MUS
                    jako strumień zdarzeń General MIDI przez camd.library
                    (CAMD), standardowe Amiga MIDI API — tak samo, jak
                    wersje silnika dla Windows i Linux korzystają odpowiednio
                    z WinMM i ALSA MIDI. Dane MIDI są wysyłane do stałego
                    klastra wyjściowego CAMD "out.0".

                    Aby usłyszeć muzykę CAMD, potrzebujesz jednego z poniższych:
                    - interfejsu MIDI z zewnętrznym synthesizerem i działającym
                      CAMD MIDI driverem (np. drivera szeregowego z pakietu
                      camd40 dostępnego w Aminet), albo
                    - software synthesizera z interfejsem CAMD (np. CAMD
                      Toolkit albo Timidity z CAMD driverem) podłączonego
                      do klastra "out.0".

                    OSTRZEŻENIE: samo camd.library NIE wystarczy — jeśli do
                    klastra nie jest podłączony MIDI driver ani synthesizer,
                    muzyka CAMD będzie NIESŁYSZALNA (efekty dźwiękowe nadal
                    będą działać). Dotyczy to również CaffeineOS, gdzie
                    camd.library jest zainstalowane, ale domyślnie NIE jest
                    skonfigurowane. Jeżeli CAMD nie może zostać
                    zainicjalizowany albo nie skonfigurowano działającego
                    wyjścia MIDI, muzyka CAMD pozostaje cicha; efekty
                    dźwiękowe działają normalnie. Raptor nie przełącza się
                    automatycznie na inny backend muzyki. W razie potrzeby
                    wybierz jawnie MUSIC=ADLIB, MUSIC=MHI, MUSIC=WAVE lub
                    MUSIC=OFF.

   Muzyka (MHI):    Parametr MUSIC=MHI odtwarza soundtrack jako pliki MP3
                    przez MHI decoder driver — standard Amiga dla dźwięku
                    MPEG, używany przez hardware decoders, takie jak Prisma
                    Megamix (prismamhi.library), MAS Player
                    (mhimaspro/mhimasstd.library), Prelude MPEGit
                    (mhimpegit.library) albo hardware mpeg.device, np. Delfina
                    (mhimdev.library). Driver sam dekoduje i wysyła MP3,
                    dlatego nie ingeruje w strumień AHI używany przez efekty.

                    Wewnątrz katalogu gry utwórz drawer o nazwie "MP3" i
                    skopiuj do niego pliki MP3 ze ścieżki dźwiękowej. Każdy
                    utwór w grze jest dopasowywany do pliku według fragmentu
                    tytułu bez uwzględniania wielkości liter (dopasowanie
                    podciągu, wyłącznie "*.mp3", wygrywa pierwszy pasujący
                    plik w drawerze) — numery tracków nie są używane, więc
                    trzymaj dokładnie jeden plik na utwór. Zalecane
                    (uproszczone) nazwy plików:

                        Main Menu.mp3
                        Game Over.mp3
                        Boss 1.mp3
                        Boss 2.mp3
                        Boss 3.mp3
                        Credits.mp3
                        Wave Music 1.mp3
                        Wave Music 2.mp3
                        Wave Music 3.mp3
                        Wave Music 4.mp3
                        Wave Music 5.mp3
                        Wave Music 6.mp3
                        Night Waves.mp3
                        Hangar.mp3
                        Raptor Intro.mp3
                        Apogee Fanfare.mp3

                    ("Fanfare for Duke II.mp3" to nazwa zastępcza
                    (DOS v1.1+) fanfary Apogee; ten port mapuje logo Apogee
                    na "Apogee Fanfare.mp3", więc ten plik nie jest wymagany.)

                    Nazwy plików muzycznych mogą używać zwykłych spacji,
                    na przykład `Main Menu.mp3` lub `Wave Music 1.wav`.
                    Jako zgodny fallback dla archiwów, które automatycznie
                    zastępują spacje, Raptor akceptuje także równoważne
                    nazwy, w których każda spacja została zamieniona na
                    znak podkreślenia `_`, na przykład `Main_Menu.mp3`
                    lub `Wave_Music_1.wav`.

                    Jeżeli istnieją oba warianty, pierwszeństwo ma
                    standardowa nazwa ze spacjami. Dotyczy to plików MP3
                    używanych przez `MUSIC=MHI` oraz plików WAV
                    używanych przez `MUSIC=WAVE`.

                    Jeśli dla utworu nie ma pasującego pliku, pozostaje on
                    cichy (efekty dźwiękowe nadal działają). Mapowanie
                    (fragment tytułu -> utwór w grze) opisano w
                    src/mpumhi.cpp (mhi_song_map).

                    Gra automatycznie wybiera driver: próbuje kolejno
                    prismamhi.library, mhimaspro/mhimasstd.library,
                    mhimpegit.library, mhimdev.library, a następnie skanuje
                    LIBS:MHI/ w poszukiwaniu innych zainstalowanych driverów.
                    Parametr MHIDRIVER= (np. -mhidriver=mhimaspro.library)
                    wymusza użycie konkretnego drivera. Jeżeli sterownik MHI
                    nie może zostać otwarty, katalog MP3 nie istnieje albo
                    nie zostanie znaleziony pasujący plik MP3, muzyka MHI
                    pozostaje cicha; efekty dźwiękowe działają normalnie.
                    Raptor nie przełącza się automatycznie na inny backend
                    muzyki. W razie potrzeby wybierz jawnie MUSIC=ADLIB,
                    MUSIC=CAMD, MUSIC=WAVE lub MUSIC=OFF. Uwaga: dla
                    klasycznych komputerów 68k nie istnieje software-only
                    MHI decoder — MUSIC=MHI wymaga jednego z opisanych
                    wyżej hardware decoders.

    Muzyka (WAVE):   Parametr MUSIC=WAVE odtwarza soundtrack jako
                     predekodowane pliki WAV z drawera o nazwie "WAVE"
                     wewnątrz katalogu gry, miksowane bezpośrednio do
                     strumienia audio AHI — nie jest potrzebny żaden MHI
                     driver, zewnętrzny dekoder ani resampling w czasie
                     działania. Każdy utwór w grze jest mapowany na plik
                     według tego samego mapowania tytułów ścieżki
                     dźwiękowej co backend muzyki MP3: nazwa pliku WAV
                     jest identyczna z nazwą pliku MP3, z tą różnicą, że
                     rozszerzenie zmienia się z ".mp3" na ".wav" (spacje
                     i dokładne nazwy bazowe pozostają zachowane), np.
                     "Main Menu.wav", "Wave Music 1.wav", "Boss 1.wav".

                     Nazwy plików muzycznych mogą używać zwykłych spacji,
                     na przykład `Main Menu.mp3` lub `Wave Music 1.wav`.
                     Jako zgodny fallback dla archiwów, które automatycznie
                     zastępują spacje, Raptor akceptuje także równoważne
                     nazwy, w których każda spacja została zamieniona na
                     znak podkreślenia `_`, na przykład `Main_Menu.mp3`
                     lub `Wave_Music_1.wav`.

                     Jeżeli istnieją oba warianty, pierwszeństwo ma
                     standardowa nazwa ze spacjami. Dotyczy to plików MP3
                     używanych przez `MUSIC=MHI` oraz plików WAV
                     używanych przez `MUSIC=WAVE`.

                     Zalecane nazwy plików WAV:

                         Main Menu.wav
                         Game Over.wav
                         Boss 1.wav
                         Boss 2.wav
                         Boss 3.wav
                         Credits.wav
                         Wave Music 1.wav
                         Wave Music 2.wav
                         Wave Music 3.wav
                         Wave Music 4.wav
                         Wave Music 5.wav
                         Wave Music 6.wav
                         Night Waves.wav
                         Hangar.wav
                         Raptor Intro.wav
                         Apogee Fanfare.wav

                     Wymagany format pliku: kontener RIFF/WAVE,
                     nieskompresowane PCM, próbki 16-bitowe ze znakiem
                     little-endian, stereo (2 kanały), 11025 Hz —
                     natywna częstotliwość ścieżki audio gry. Jeżeli
                     katalog WAVE nie istnieje, plik WAV nie zostanie
                     znaleziony albo ma nieobsługiwany format, dany utwór
                     pozostaje cichy; efekty dźwiękowe działają normalnie.
                     Raptor nie przełącza się automatycznie na inny backend
                     muzyki. Muzyka WAVE ma
                     własne ustawienie głośności: klucz "music_wave" w
                     pliku amiga.cfg oraz osobną głośność muzyki w
                     ustawieniach muzyki w grze. Muzyka WAVE i efekty
                     dźwiękowe Sound Blaster były testowane razem i
                     działają poprawnie.

Stan audio i informacje diagnostyczne są wypisywane do konsoli przy
uruchomieniu (Shell/CLI). Żaden plik logu nie jest tworzony automatycznie
— aby zapisać komunikaty do pliku, przekieruj stdout, np.:

   raptor > RAPTOR.LOG


Parametry Command Line
----------------------

Wszystkie parametry są rozpoznawane bez uwzględniania wielkości liter
i mogą być podawane zarówno z poprzedzającym je myślnikiem (styl GNU),
jak i bez niego (styl AmigaDOS). Przykłady: "-nosound", "NOSOUND"
i "nosound" są równoważne. Parametry można łączyć w dowolnej kolejności.

   -nosound    Wyłącza CAŁY dźwięk (muzykę i efekty dźwiękowe). Ani
               ahi.device, ani camd.library nie zostaną otwarte.
               Jest to najszybsza ścieżka uruchamiania.

   -nomusic    Wyłącza wyłącznie muzykę; efekty dźwiękowe (strzały,
               eksplozje itd.) nadal są odtwarzane przez AHI.
               W tym trybie camd.library nie zostanie otwarte.

   -music=M    Wybiera backend muzyki. M może przyjąć wartość:
                 ADLIB — wbudowana emulacja AdLib/OPL3 miksowana ze
                         strumieniem audio AHI (autentyczne brzmienie
                         Raptora, zawsze słyszalne);
                 CAMD  — strumień zdarzeń General MIDI przez camd.library;
                         wymaga skonfigurowanego MIDI drivera albo CAMD
                         software synthesizera w klastrze "out.0", w przeciwnym
                         razie muzyka będzie NIESŁYSZALNA (zob. sekcję
                         „Dźwięk (AHI + CAMD + MHI)” powyżej);
                  MHI   — pliki MP3 z drawera MP3/ przez MHI decoder driver
                          (np. Prisma Megamix); jeśli MHI driver nie może
                          zostać otwarty, muzyka przechodzi na MUSIC=OFF
                          (zob. sekcję powyżej);
                  WAVE  — predekodowane pliki WAV z drawera WAVE/,
                          miksowane do strumienia audio AHI (zob.
                          „Muzyka (WAVE)” powyżej);
                  OFF   — brak muzyki (tak samo jak -nomusic; efekty dźwiękowe
                          nadal są odtwarzane przez AHI).
                Działa również forma bez myślnika: "MUSIC=MHI". Bez opcji
                MUSIC= stanem domyślnym jest MUSIC=OFF: żaden backend
                muzyki nie jest inicjalizowany, a efekty dźwiękowe działają.
                -nomusic / MUSIC=OFF ma zawsze pierwszeństwo przed
                MUSIC=ADLIB, MUSIC=CAMD, MUSIC=MHI i MUSIC=WAVE, niezależnie
                od kolejności parametrów.

    -mhidriver=D Zastępuje automatyczne wykrywanie MHI decoder drivera
                (istotne wyłącznie razem z MUSIC=MHI). D to nazwa biblioteki
                drivera albo pełna ścieżka, np.
                "-mhidriver=prismamhi.library" lub
                "MHIDRIVER=LIBS:MHI/mhimaspro.library".

    -mouse=ON|OFF
                Włącza lub wyłącza urządzenie myszy. OFF (albo starsza
                forma -nomouse) wyłącza całą obsługę myszy: okno nie
                rejestruje żadnych zdarzeń myszy (brak IDCMP_MOUSEMOVE/
                MOUSEBUTTONS), kursor nie jest aktualizowany, a sterowanie
                statkiem myszą w grze jest wyłączone. Jest to opcja
                wydajnościowa/diagnostyczna — przy wyłączonej myszy
                przetwarzanie zdarzeń myszy w każdej klatce jest całkowicie
                pomijane. Domyślnie ON. Działa również forma bez myślnika:
                "MOUSE=OFF". Jawna forma =wartość ma zawsze pierwszeństwo
                przed starszą flagą NOMOUSE, niezależnie od kolejności
                parametrów.

    -nomouse    Starsza forma MOUSE=OFF (patrz wyżej).

    -joystick=ON|OFF
                Włącza lub wyłącza odpytywanie joysticka/pada CD32.
                OFF (albo starsza forma -nojoy) wyłącza całe odpytywanie
                portu gier: port nie jest w ogóle czytany, a joystick jest
                raportowany jako nieobecny. Jest to opcja wydajnościowa/
                diagnostyczna — przydatna do namierzania fantomowych
                wejść na prawdziwym sprzęcie bez rekompilacji. Domyślnie
                ON. Działa również forma bez myślnika: "JOYSTICK=OFF".
                Jawna forma =wartość ma zawsze pierwszeństwo przed starszą
                flagą NOJOY, niezależnie od kolejności parametrów.

    -nojoy      Starsza forma JOYSTICK=OFF (patrz wyżej).

    -gfx=M      Wybiera ścieżkę drivera grafiki używaną dla ekranu gry.
               M może przyjąć wartość:
                 AUTO  — najpierw próbuje P96 (Picasso96) RTG; jeśli P96
                         nie jest dostępne albo nie oferuje pasującego trybu,
                         przełącza się na CGX (CyberGraphX). Jeśli żaden
                         driver nie utworzy użytecznego ekranu, zostanie
                         wyświetlony angielski requester, a gra NIE zostanie
                         uruchomiona (brak cichego fallbacku do klasycznego
                         chipsetu — jest to ustawienie domyślne);
                 RTG   — to samo co AUTO (wymagane RTG, brak cichego fallbacku);
                  AGA   — wymusza natywny ekran chipsetu 320x200x8
                          (osobna, natywna ścieżka renderowania dla
                          sprzętu z AGA: Amiga 1200 i Amiga 4000;
                          dostępna także w WinUAE i zgodnych
                          konfiguracjach PiStorm/Emu68, gdy AGA
                          jest dostępne).

               Działa również forma bez myślnika: "GFX=RTG". Jeśli ścieżka
               RTG nie może otworzyć ekranu 320x200x8, szuka trybu 320x240x8,
               otwierając go z czarnym pasem wysokości 40 wierszy na dole
               ("letterbox"). Obejmuje to karty i drivery RTG, których
               domyślna lista trybów NIE zawiera 320x200x8, ale zawiera
               320x240x8 (częsty domyślny układ Picasso96 ScreenModes).
               Fallback ten dotyczy zarówno trybów P96, jak i CGX.

               W razie niepowodzenia gra wyświetla angielski requester
               z informacjami pomocnymi przy diagnostyce: jaki tryb jest
               wymagany oraz jakiego parametru użyć, aby wymusić ekran
               klasycznego chipsetu (GFX=AGA / -gfx=AGA).

   REC <file>  Nagrywa demo rozgrywki do wskazanego pliku
               (np. "raptor REC demo1.dem").

   PLAY <file> Odtwarza wcześniej nagrany plik demo
               (np. "raptor PLAY demo1.dem").


Uruchamianie z Shell/CLI
------------------------

Otwórz okno Shell (CLI), przejdź do katalogu gry i uruchom ją, np.:

   cd Games:Raptor
   raptor

Parametry działają z myślnikiem lub bez niego i nie uwzględniają wielkości liter:

   raptor NOMUSIC

Łączenie parametrów:

   raptor -nomusic REC demo1.dem


Uruchamianie z ikony Workbench
------------------------------

Grę można uruchomić dwukrotnym kliknięciem jej ikony w Workbench.
Parametry są przekazywane za pomocą ToolTypes ikony:

1. Kliknij ikonę raptor raz, a następnie wybierz z menu Workbench
   „Icons / Information...” (albo naciśnij Right Amiga + I).

2. W oknie informacji o ikonie dodaj żądane ToolTypes, po jednym
   w każdym wierszu, np.:

      NOSOUND
      (MUSIC=CAMD)
      GFX=AGA
      MOUSE=OFF
      (NOMOUSE)
      JOYSTICK=OFF
      (NOJOY)

   ToolType ujęty w nawiasy jest NIEAKTYWNY (ignorowany przez grę) —
   to wygodny sposób przechowywania opcji w ikonie bez jej włączania.
   Uwzględniane są wyłącznie aktywne formy bez nawiasów.

3. Kliknij „Save” i dwukrotnie kliknij ikonę, aby uruchomić grę.

Uwaga: aby uruchomić grę bez dźwięku z Workbench, dodaj do ikony
ToolType NOSOUND (NOMUSIC — albo MUSIC=OFF — pozostawia włączone efekty
dźwiękowe). Muzyka jest domyślnie wyłączona (MUSIC=OFF). MUSIC=ADLIB
wybiera wbudowaną emulację AdLib/OPL3, MUSIC=CAMD wybiera muzykę MIDI,
MUSIC=MHI muzykę MP3 przez MHI driver, a MUSIC=WAVE predekodowaną
muzykę WAV z drawera WAVE/.

Uwaga: MOUSE=OFF / NOMOUSE wyłącza mysz, a JOYSTICK=OFF / NOJOY wyłącza
joystick (obie opcje domyślnie ON). Jawna forma =wartość ma pierwszeństwo
przed starszą flagą. Wyłączenie urządzenia wejściowego to opcja
wydajnościowa/diagnostyczna: przy wyłączonej myszy okno nie rejestruje
żadnych zdarzeń myszy, a przy wyłączonym joysticku port gier nie jest
w ogóle odpytywany.

Uwaga: po uruchomieniu z ikony Workbench gra nie generuje żadnego
wyjścia konsolowego (nie otwiera w ogóle okna konsoli). Aby zobaczyć
komunikaty startowe, uruchom grę z Shell/CLI.


Obraz na PiStorm / Emu68
------------------------

Wyjście obrazu zostało zweryfikowane jako poprawnie działające na
PiStorm/Emu68 zarówno przez AGA, jak i RTG. Port najpierw próbuje użyć
ekranu RTG 320x200x8; jeśli ta rozdzielczość nie występuje na liście
trybów drivera, przełącza się na ekran 320x240x8 (z czarnym pasem
wysokości 40 wierszy na dole). Jeśli żaden z trybów nie jest dostępny,
pojawi się angielski requester z prośbą o skonfigurowanie odpowiedniego
trybu RTG albo uruchomienie gry z ekranem klasycznego chipsetu (GFX=AGA).

Notatka o wyświetlaniu (PAL/NTSC): W trybie PAL na dole ekranu może być
widoczny czarny pasek, ponieważ gra używa obszaru obrazu 320x200. Aby
pionowo wypełnić ekran, przed uruchomieniem wybierz NTSC w Amiga Early
Startup; gra otworzy się wtedy na pełnym ekranie w rozdzielczości 320x200.

Uwaga: środkowy przycisk myszy jest ignorowany. Na niektórych komputerach
(w szczególności A1200 + PiStorm/Emu68, zarówno na ekranach RTG, jak i AGA)
powoduje on fałszywe naciśnięcia, które pomijają intro logos, natychmiast
kończą dema i zakłócają sterowanie. Zmiana aktywnej special weapon jest
zawsze dostępna za pomocą klawisza SPACE. Lewy i prawy przycisk myszy
działają normalnie we wszystkich trybach.


Uwagi dotyczące natywnego trybu AGA (PAL/NTSC)
----------------------------------------------

Natywny tryb AGA wybierany jest za pomocą `GFX=AGA`.

Gra zawsze renderuje stały obraz logiczny **320×200** od lewego górnego rogu ekranu. Na Amadze skonfigurowanej na PAL dolna część natywnego ekranu PAL może być czarna. Jest to zachowanie normalne i zamierzone.

`VIDEO=AUTO` (wartość domyślna) używa trybu wyświetlania AGA wybranego w systemie.  
`VIDEO=NTSC` żąda trybu NTSC jako domyślnego. Przy `GFX=AGA` ekran otwierany jest z `SA_DisplayID=NTSC_MONITOR_ID|LORES_KEY` (natywny NTSC low‑res), co daje ekran **320×200×8** w standardzie NTSC. Przy `GFX=RTG` opcja ta jest ignorowana i używane jest `AUTO`.

Sam parametr `VIDEO=NTSC` **nie zmienia** Amigi skonfigurowanej na PAL w NTSC i nie modyfikuje niczego w `DEVS:Monitors`. Jeśli chcesz, aby obraz 320×200 wypełniał cały ekran pionowo w timingach NTSC, Amiga musi być skonfigurowana/uruchomiona w trybie NTSC przed uruchomieniem gry.

Opcjonalnie: uruchamianie systemu AmigaOS 3.x w NTSC  
1. Zresetuj lub włącz Amigę.  
2. Przytrzymaj oba przyciski myszy, aby otworzyć Early Startup Control.  
3. W Display Options wybierz **NTSC**.  
4. Uruchom AmigaOS, a następnie Raptora z `GFX=AGA` (i opcjonalnie `VIDEO=NTSC`).

Jako ToolTypes w Workbenchu ustaw w osobnych liniach:

```text
GFX=AGA
VIDEO=AUTO
```


Sterowanie
----------

W tym porcie klawiatura, mysz i joystick działają jednocześnie — nie ma
potrzeby wybierania urządzenia w options. Mysz przejmuje sterowanie
statkiem po fizycznym poruszeniu nią (albo przytrzymaniu przycisku myszy);
klawiatura i joystick pozostają aktywne przez cały czas.

Uwaga: mysz i joystick można całkowicie wyłączyć parametrami
MOUSE=OFF / NOMOUSE oraz JOYSTICK=OFF / NOJOY (patrz „Parametry Command
Line” powyżej). Jest to opcja wydajnościowa/diagnostyczna: przy
wyłączonej myszy okno nie rejestruje żadnych zdarzeń myszy, a przy
wyłączonym joysticku port gier nie jest w ogóle odpytywany. Sterowanie
klawiaturą pozostaje bez zmian.

Klawiatura — podczas gry:

   Arrow keys       — Sterowanie statkiem
   Left CTRL        — Strzał z primary weapon
   Left ALT         — Strzał z special weapon
   SPACE            — Zmiana aktywnej special weapon
   Right SHIFT      — MegaFire (mega bomba)
   1 ... 0, "-"     — Bezpośredni wybór special weapon (zob. poniżej)
   P                — Pause
   F1               — Help
   ESC              — Przerwanie misji — powrót do main menu
   ALT + X          — Wyjście do systemu (z potwierdzeniem)
   BACKSPACE        — Tylko pełna wersja: dodaje Death Ray + energię,
                      resetuje wynik

Bezpośredni wybór special weapon (jeśli jest w inventory):

   1  Dumb Missile        6  Ground Missile
   2  Mini Gun            7  Bomb
   3  Turret              8  Energy Grab

   4  Missile Pods        9  Pulse Cannon
   5  Air Missile         0  Death Ray
   -  Forward Laser

Mysz — podczas gry:

   Mouse movement   — Sterowanie statkiem (pozycja kursora)
   Left button      — Strzał z primary weapon
   Right button     — Strzał z special weapon
   Middle button    — Zmiana aktywnej special weapon

Joystick / pad CD32 (port 1):

Wymaga lowlevel.library v40+ (standardowo dostępnej w AmigaOS 3.2).
Port jest przełączany w tryb game controller, dlatego obsługiwane są
standardowe joysticki 1- i 2-button oraz pady CD32.

   Stick / D-pad    — Sterowanie statkiem
   FIRE 1 (red)     — Strzał z primary weapon
   FIRE 2 (blue)    — Strzał z special weapon
   CD32 PLAY        — Strzał z special weapon

Uwagi:
- Zmiana special weapon i MegaFire są dostępne wyłącznie z klawiatury
  (SPACE / Right SHIFT) albo myszy (środkowy przycisk).
- Pause jest dostępne wyłącznie z klawiatury (P) — joystick Amigi nie
  ma przycisku Start.
- Bez lowlevel.library joystick jest niedostępny, ale gra normalnie
  działa z klawiaturą i myszą.

Sterowanie w menu (klawiatura / mysz / joystick):

   Arrow keys / joystick D-pad       — Nawigacja po opcjach
   ENTER lub SPACE                   — Wybór opcji
   Left mouse button                 — Wybór opcji (kliknięcie)
   FIRE 1 (red)                      — Wybór opcji
   ESC                               — Powrót / anulowanie
   FIRE 2 (blue) / CD32 PLAY         — Powrót / anulowanie
   F1                                — Help kontekstowy
   ALT + X                           — Wyjście do systemu
   Arrows / PgUp / PgDn / Home / End — Przewijanie okna pomocy
   BACKSPACE (w polach tekstowych)   — Usunięcie znaku
   CTRL + Y (w polach tekstowych)    — Wyczyszczenie całego pola


Znane ograniczenia
------------------

- Muzyka MIDI (MUSIC=CAMD) wymaga MIDI drivera albo CAMD software
  synthesizera podłączonego do klastra "out.0" — w przeciwnym razie
  muzyka będzie cicha, choć efekty dźwiękowe nadal będą działać. Uwaga
  dla użytkowników CaffeineOS: camd.library jest tam zainstalowane,
  ale domyślnie NIE jest skonfigurowane. Jeżeli CAMD nie może zostać
  zainicjalizowany albo nie ma działającego wyjścia MIDI, muzyka
  przechodzi na MUSIC=OFF (efekty dźwiękowe pozostają aktywne);
  wybierz jawnie MUSIC=ADLIB, MUSIC=MHI, MUSIC=WAVE lub MUSIC=OFF.
- Muzyka MP3 (MUSIC=MHI) wymaga zainstalowanego MHI decoder drivera
  w LIBS:MHI/ (Prisma Megamix, MAS Player, Prelude MPEGit albo hardware
  mpeg.device, np. Delfina). Dla klasycznych komputerów 68k nie ma
  software-only MHI decodera; jeśli MHI driver nie może zostać otwarty,
  muzyka przechodzi na MUSIC=OFF (efekty dźwiękowe pozostają aktywne).
  Utwory, których pliku MP3 brakuje w drawerze MP3/, pozostają celowo
  ciche.
- Głośność muzyki i efektów zmieniana w menu opcji jest zapisywana do
  amiga.cfg w katalogu gry (pliku tworzonego przy pierwszym uruchomieniu)
  i przywracana przy kolejnym starcie. Każdy backend muzyki ma własny
  klucz głośności (music_adlib, music_mhi, music_wave) oraz sfx_volume.
  Poziom detali nie jest zapisywany.
- Muzyka WAVE (MUSIC=WAVE) wymaga plików WAV w drawerze WAVE/ w
  wymaganym formacie (11025 Hz, stereo, 16-bit PCM); utwory, których
  pliku WAV brakuje, pozostają celowo ciche.
- Brak pause i wyjścia z menu bezpośrednio z joysticka (użyj klawiatury).
- Brak obsługi rumble / haptic.
- Systemowy pointer myszy Amigi jest ukrywany podczas działania gry
  (i przywracany przy wyjściu do systemu).
- Stała rozdzielczość 320x200 — gra zawsze otwiera własny ekran.


Testowane konfiguracje
----------------------
- Raspberry Pi 400 z PiMIGA — przetestowano w trybach AGA i RTG.
- Amiga 1200 z PiStorm i CaffeineOS — przetestowano AGA PAL,
  AGA NTSC oraz RTG.
- Amiga 1200 z Mediatorem, Blizzardem 1260, Voodoo3 oraz Prelude
  na clock porcie — przetestowano także odtwarzanie MHI.
- Amiga 2000 z TekMagic 68060 50 MHz, CyberVision 64/3D oraz
  Prisma MegaMix.
- Amiga 4000 z 68060 50 MHz, Picasso IV i AGA — przetestowano
  WAVE, MIDI/CAMD oraz MHI.
- WinUAE z konfiguracjami 68030 i 68060, z FPU i bez FPU,
  przetestowano w trybach AGA oraz RTG.
- Przetestowano na AmigaOS 3.1.4, 3.2 i 3.2.3.


Plany / pozostałe prace
-----------------------

- Dostrajanie i szlifowanie wydajności na prawdziwym sprzęcie 68k.
- Wersja 0.9.9-rc.1 jest ostatnią kompilacją przedpremierszą przed
  planowanym wydaniem.


Autorzy i kontakt
-----------------

   Autor portu Amiga:  Marcin "Raybeez" Bednarczyk (aka Cichy)
   Współpraca z AI:    Projekt powstał przy wsparciu narzędzi AI.
   Kontakt / feedback: cichy@cichy.com.pl
   GitHub repository:  https://github.com/RaybeezPL/raptor-amiga-port

   Szczególne podziękowania dla wszystkich użytkowników Amigi,
   którzy podtrzymują scenę przy życiu.


Licencja i podziękowania
------------------------

Ten port NIE zawiera plików danych gry. Potrzebujesz własnej, legalnej
kopii Raptor: Call of the Shadows. Obsługiwana jest wyłącznie pełna
wersja 1.2.

Port bazuje na projekcie skynettx/raptor. Podziękowania dla nukeykt
i wszystkich osób współtworzących zrekonstruowany kod Raptora,
a także dla społeczności Amigi oraz twórców narzędzi RTG i AHI.


Komponenty third-party wykorzystane w tym porcie:

- Game engine: rekonstrukcja skynettx/raptor, GNU GPL-2.0 (zob. plik
  LICENSE); zawiera kod wywodzący się z Chocolate Doom (GNU GPL-2.0+).
- Emulator dbopl OPL: DOSBox Team, GNU GPL-2.0+.
- TinySoundFont (include/TinySoundFont/tsf.h): Bernhard Schelling,
  licencja MIT (zgodna z GPL).
- Nagłówki interfejsów AHI i CAMD (src/amiga/devices/ahi.h,
  src/amiga/midi/, src/amiga/clib/camd_protos.h): dołączone bez zmian
  z oficjalnych, bezpłatnie rozpowszechnianych pakietów developerskich
  dostępnych w Aminet (AHI dev archive Martina Bloma; CAMD developer kit).
  Zawierają wyłącznie definicje interfejsów (struktury, stałe, prototypy)
  i służą do wywoływania odpowiednich bibliotek systemowych AmigaOS
  w czasie działania programu.
- Nagłówki interfejsu MHI (src/amiga/libraries/mhi.h,
  src/amiga/clib/mhi_protos.h): dołączone z oficjalnego MHI developer kit
  v1.2 (Aminet driver/audio/mhi_dev.lha, MHI autorstwa Thomasa Wenzela
  i Paula Qureshiego).
- src/amiga/proto/camd.h oraz src/amiga/inline/camd.h zostały napisane
  ręcznie na potrzeby tego portu (offsety LVO zweryfikowano względem
  oficjalnego camd_lib.fd).
- src/amiga/proto/mhi.h oraz src/amiga/inline/mhi.h zostały napisane
  ręcznie na potrzeby tego portu (offsety LVO i przypisania rejestrów
  zweryfikowano względem oficjalnego mhi_lib.fd z MHI developer kit).
- Akceleracja display blit (src/amiga/amiga_sdl_stubs.h): ścieżki RTG
  wywołują oficjalne API driverów Picasso96/CyberGraphX; inline stub
  CGX WritePixelArray został napisany ręcznie na potrzeby tego portu
  (offset LVO zweryfikowano względem oficjalnego CGraphX-DevKit VI
  cybergraphics_lib.fd).
- Konwerter AGA chunky-to-planar (C2P) w tym samym pliku jest autorskim
  kodem napisanym na potrzeby tego portu (GNU GPL-2.0), zweryfikowanym
  bitowo względem implementacji brute-force. Wykorzystuje wyłącznie
  dobrze znany, opublikowany algorytm transpozycji macierzy bitów 8x8 —
  nie używa żadnego kodu C2P third-party/demo-scene.


Zgodność z GNU License / informacja o kodzie źródłowym:
-------------------------------------------------------

Pełny odpowiadający kod źródłowy dla każdego binarnego wydania jest
dostępny w tym repozytorium oraz pod odpowiadającym mu tagiem.
Dla wersji 0.9.9-rc.1 zobacz:
https://github.com/RaybeezPL/raptor-amiga-port/tree/0.9.9-rc.1

Kod źródłowy jest dostępny już teraz w tym repozytorium. Finalne
archiwum Aminet będzie również zawierało właściwe informacje o
źródłach i licencji.
