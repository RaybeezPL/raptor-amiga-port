# Raptor – Analiza Portowania na AmigaOS 3.x (68060 + RTG)

## Sprzęt docelowy
- **Procesor:** Motorola 68060
- **Pamięć:** 32 MB Fast RAM
- **Grafika:** Karta RTG (Picasso96/CyberGraphX) z 4 MB VRAM
- **Kompilator:** m68k-amigaos-gcc (cross-compiler)

---

## 1. Struktura repozytorium – przegląd

Gra Raptor to klasyczna strzelanka top-down (shoot'em'up), oryginalnie napisana pod DOS,
przeportowana na nowoczesne systemy za pomocą SDL2. Kod to C/C++ (C++11).

### Główne moduły kodu źródłowego (`src/`):

| Moduł | Pliki | Opis |
|-------|-------|------|
| **Silnik graficzny** | `gfxapi.cpp/h`, `gfxapi_a.cpp` | Software renderer 320×200×8bit (chunky pixels) |
| **Warstwa wideo** | `i_video.cpp/h` | Inicjalizacja SDL2 Window/Renderer/Texture, wyświetlanie |
| **System plików GLB** | `glbapi.cpp/h` | Własny format zasobów (.GLB), szyfrowanie, cache |
| **Pamięć wirtualna** | `vmemapi.cpp/h` | Własny menedżer pamięci z LRU eviction |
| **Dźwięk** | `fx.cpp/h`, `dspapi.cpp/h` | Software mixer, 8-kanałowy DSP, callbacki SDL Audio |
| **Muzyka** | `musapi.cpp/h`, `i_oplmusic.cpp`, `opl3.cpp` | Format MUS, emulacja OPL3/Adlib |
| **MIDI (TinySoundFont)** | `mputsf.cpp` | Syntezator SF2 (header-only lib) |
| **MIDI (platformy)** | `mpuwinmm.cpp`, `mpualsa.cpp`, `mpucorea/m.cpp` | WinMM, ALSA, CoreAudio – **nie dotyczy Amigi** |
| **Wejście** | `kbdapi.cpp`, `joyapi.cpp`, `ptrapi.cpp`, `input.cpp` | Klawiatura, joystick, mysz – SDL2 Events |
| **System okien/GUI** | `swdapi.cpp`, `windows.cpp` | In-game dialogi i UI z danymi z GLB |
| **Logika gry** | `rap.cpp`, `enemy.cpp`, `shots.cpp`, `tile.cpp`, `bonus.cpp`, itp. | Czysta logika, minimalne zależności od platformy |
| **Zapis/odczyt** | `loadsave.cpp/h` | Serializacja z explicit byte-order |
| **Demo** | `demo.cpp/h` | Nagrywanie/odtwarzanie gameplay |
| **Ustawienia** | `prefapi.cpp/h` | Odczyt/zapis plików INI (czyste C, bez SDL) |
| **Animacje intro** | `intro.cpp`, `movie.cpp/h`, `movie_a.cpp` | Animowane sekwencje cutscene |
| **Textscreen (setup)** | `include/textscreen/` | Osobna biblioteka UI setupu – SDL2-based |

---

## 2. Endianness (Little Endian → Big Endian)

### 2.1 Aktualny stan – DOBRA WIADOMOŚĆ

Kod **już posiada rozbudowaną obsługę endianness**. W pliku `src/entypes.h`:

```c
#include "SDL_endian.h"
#define LE_USHORT(x) SDL_SwapLE16(x)
#define LE_SHORT(x)  (signed short) SDL_SwapLE16(x)
#define LE_ULONG(x)  SDL_SwapLE32(x)
#define LE_LONG(x)   (signed int) SDL_SwapLE32(x)
```

Na systemach Big Endian (Amiga 68k), `SDL_SwapLE16/32` automatycznie wykonują byte-swap.
Na Little Endian (PC) są no-op.

### 2.2 Gdzie są używane makra LE_*

Makra `LE_LONG`, `LE_SHORT`, itp. są używane **wszędzie** gdzie kod odczytuje dane binarne
z plików `.GLB` (zasoby gry w formacie Little Endian DOS):

- **System plików GLB** (`glbapi.cpp`): nagłówki `KEYFILE` – `LE_ULONG(key.offset)`, `LE_ULONG(key.filesize)`
- **Struktury grafik** (`gfxapi.cpp`, `gfxapi_a.cpp`): `GFX_PIC.width/height`, `GFX_SPRITE.offset/length` – dostęp przez `LE_LONG()`
- **Sprite'y wrogów** (`enemy.cpp`): cała struktura `SPRITE` (30+ pól int/short) – każde pole czytane z `LE_LONG()/LE_SHORT()`
- **Dane mapy** (`tile.cpp`): `MAZELEVEL`, `MAZEDATA` – `LE_SHORT(mapmem->map[loop].flats)`
- **Audio DSP** (`dspapi.cpp`): `dsp_t.format/freq/length` – `LE_SHORT(dsp->format)`
- **Muzyka** (`musapi.cpp`): `mushead_t.len/offset/channels` – `LE_USHORT(head->len)`
- **OPL/GenMIDI** (`i_oplmusic.cpp`): flagi instrumentów – `LE_USHORT(instrument->flags)`
- **System okien** (`swdapi.cpp`): dziesiątki pól – `LE_LONG(curfld->x)`, `LE_LONG(curfld->opt)`, itp.
- **Demo** (`demo.cpp`): `RECORD.px/py/playerpic` – `LE_SHORT()`
- **Czcionki** (`gfxapi.cpp`): `FONT.charofs[]` – `LE_SHORT(font->charofs[ch])`
- **GSS patches** (`gssapi.cpp`): pola formatu audio – `LE_SHORT(gss->bank)`

### 2.3 System zapisu/odczytu gier

Plik `loadsave.cpp` zawiera **jawną serializację byte-by-byte** – to wzorcowe rozwiązanie:

```c
static int SaveRead32(void) {
    int convert;
    convert = SaveRead8();
    convert |= SaveRead8() << 8;
    convert |= SaveRead8() << 16;
    convert |= SaveRead8() << 24;
    return convert;
}
```

To jest **w pełni przenośne** i poprawnie działa na Big Endian.

### 2.4 Unia ITEM_ID w glbapi.cpp

Kod posiada już obsługę BE dla unii `ITEM_ID`:

```c
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
typedef struct { uint16_t filenum; uint16_t itemnum; } ITEM_ID;
#else
typedef struct { uint16_t itemnum; uint16_t filenum; } ITEM_ID;
#endif
```

### 2.5 Potencjalne problemy endianness

1. **Brak wyraźnych problemów** – kod jest dobrze przygotowany. Jednakże potrzebna jest
   **dokładna weryfikacja** każdego miejsca, gdzie dane z GLB są odczytywane bez makr LE_*.
   Szczególnie:
   - Dane grafik czysto bajtowych (palety, piksele 8-bit) – tu nie ma problemu
   - Tablica `FLATS` w `rap.h` – pola `linkflat` (int), `bonus` (short), `bounty` (short) – 
     w `tile.cpp` dostęp jest przez `LE_SHORT(lib[...].bounty)` ✓
   
2. **Wewnętrzna pamięć**: Dane ze struktur `SPRITE`, `CSPRITE`, `MAZELEVEL` itp. **nie są 
   konwertowane in-place** – wartości LE są swapowane przy każdym odczycie. Na 68060 operacja
   byte-swap jest tania (instrukcja `ROL`/`ROR`), ale jest to drobny narzut wydajnościowy.
   Alternatywnie można dodać jednorazową konwersję po załadowaniu.

3. **#pragma pack(push, 1)** – używany w `mushead_t`, `RECORD`, `genmidi_op_t`. 
   GCC na m68k wspiera `#pragma pack`, ale trzeba upewnić się, że alignment jest poprawny.

### 2.6 Podsumowanie endianness

| Obszar | Status | Uwagi |
|--------|--------|-------|
| Nagłówki GLB (KEYFILE) | ✅ OK | Pełna konwersja LE_ |
| Dane graficzne (piksele) | ✅ OK | 8-bit, endianness nie dotyczy |
| Palety kolorów | ✅ OK | 3 bajty na kolor (R,G,B) |
| Struktury sprite'ów | ✅ OK | Dostęp przez LE_LONG/LE_SHORT |
| Dane mapy | ✅ OK | Dostęp przez LE_SHORT |
| Audio patches (DSP/GSS) | ✅ OK | Nagłówki przez LE_SHORT/LE_LONG, dane 8-bit |
| Muzyka MUS | ✅ OK | Nagłówek przez LE_USHORT, dane strumieniowe bajtowe |
| Zapis/odczyt gry | ✅ OK | Jawna serializacja LE byte-by-byte |
| Demo recordings | ✅ OK | Pola przez LE_SHORT |
| System okien (SWD) | ✅ OK | Wszystkie pola przez LE_LONG |

---

## 3. Zależności – biblioteki zewnętrzne

### 3.1 SDL2 – **główna zależność** (KRYTYCZNA)

SDL2 jest używane w **prawie każdym module**:

| Użycie SDL2 | Pliki | Amigowe zamienniki |
|--------------|-------|-------------------|
| Video (Window, Renderer, Texture, Surface) | `i_video.cpp` | **Picasso96/CyberGraphX API** lub **SDL2 Amiga port** |
| Audio (SDL_AudioDeviceID, callback mixing) | `fx.cpp`, `mputsf.cpp` | **AHI (Audio Hardware Interface)** lub SDL2 Amiga |
| Input Events (keyboard, mouse, joystick) | `kbdapi.cpp`, `joyapi.cpp`, `ptrapi.cpp`, `input.cpp` | **input.device, gameport.device, Intuition** lub SDL2 Amiga |
| Timer (SDL_GetTicks) | `gfxapi.cpp`, `musapi.cpp` | **timer.device** lub `ReadEClock()` |
| Endianness (SDL_SwapLE*) | `entypes.h` | Prosta re-implementacja (patrz niżej) |
| Filesystem (SDL_GetPrefPath) | `loadsave.cpp` | `PROGDIR:` / `ENV:` |
| Init/Quit subsystems | wielu | Natywne odpowiedniki Amiga |
| SDL_opengl.h | `i_video.cpp` | **Nie używane aktywnie** – do usunięcia |

#### Strategia zamiany SDL2 – **dwa podejścia**:

**Podejście A: Użycie portu SDL2 dla AmigaOS 3.x**
- Istnieje port SDL2 dla m68k AmigaOS (np. w bebbo GCC toolchain)
- **Zalety**: Minimalna modyfikacja kodu, szybki start
- **Wady**: Dodatkowa warstwa abstrakcji, możliwe problemy z wydajnością, port SDL2 
  może nie być kompletny lub stabilny

**Podejście B: Natywne API AmigaOS (ZALECANE)**
- Wymaga więcej pracy, ale daje najlepszą wydajność i kontrolę
- Warstwa HAL (Hardware Abstraction Layer) ukryta za istniejącymi interfejsami
  (`GFX_InitVideo`, `I_InitGraphics`, `SND_InitSound`, itp.)
- Architektura kodu to ułatwia – gra ma czyste API boundaries

**Podejście C: Hybrydowe** 
- Zacząć od SDL2 Amiga port → stopniowo zastępować natywnym API
- **Rekomendowane jako strategia startowa**

### 3.2 TinySoundFont (`include/TinySoundFont/tsf.h`)

Header-only C library do syntezy MIDI z plików SoundFont (.sf2).

- **Status**: Powinno się skompilować na m68k-amigaos-gcc bez zmian
- **Problem**: Wymaga float/double – na 68060 z FPU to OK, ale może być wolne
  w porównaniu z OPL3 emulation (integer-based)
- **Rekomendacja**: Na początek użyć trybu OPL3/Adlib (emulacja w `opl3.cpp` – czyste integer),
  TinySoundFont odłożyć na później

### 3.3 Textscreen (`include/textscreen/`)

Biblioteka tekstowego UI (z projektu Chocolate Doom), używana tylko przez **program setupu** (`raptorsetup`).

- **Status**: Zależna od SDL2, pisana w C
- **Rekomendacja**: Odłożyć na późniejszą fazę. Konfigurację można ręcznie edytować w SETUP.INI

### 3.4 Platformowe biblioteki MIDI

| Plik | Platforma | Potrzebne na Amidze? |
|------|-----------|---------------------|
| `mpuwinmm.cpp` | Windows (WinMM) | ❌ Nie |
| `mpualsa.cpp` | Linux (ALSA) | ❌ Nie |
| `mpucorea.cpp` | macOS (CoreAudio) | ❌ Nie |
| `mpucorem.cpp` | macOS (CoreMIDI) | ❌ Nie |
| `mputsf.cpp` | Wszystkie (TinySoundFont) | ✅ Tak (opcjonalnie) |

### 3.5 Standardowa biblioteka C

Kod intensywnie używa: `stdio.h`, `stdlib.h`, `string.h`, `stdint.h`, `math.h`, `ctype.h`, 
`limits.h`, `errno.h`, `time.h`. Wszystkie dostępne w m68k-amigaos-gcc (newlib lub clib2).

### 3.6 Specyficzne wywołania POSIX/Win32

| Wywołanie | Plik | Amigowe zamienniki |
|-----------|------|-------------------|
| `access()` | `glbapi.cpp`, `loadsave.cpp`, `prefapi.cpp` | Dostępne w clib2/newlib |
| `unistd.h` | `glbapi.cpp`, `rap.cpp`, `loadsave.cpp` | Częściowo w clib2 |
| `ftruncate()/fileno()` | `prefapi.cpp` | Dostępne w clib2 |
| `strupr()` | `glbapi.cpp` | Już zaimplementowane inline dla __GNUC__ ✓ |
| `ltoa()` | `prefapi.cpp` | Już zaimplementowane inline dla __GNUC__ ✓ |
| `PATH_MAX` | wielu | Zdefiniować jeśli brak (np. 256) |
| `SDL_GetPrefPath()` | `loadsave.cpp` | Zastąpić stałą ścieżką (`PROGDIR:`) |

---

## 4. Główne wyzwania portowania – lista priorytetowa

### 🔴 KRYTYCZNE (blokujące kompilację i uruchomienie)

1. **Zastąpienie/dostarczenie SDL2**
   - Albo skompilować SDL2 dla AmigaOS 3.x (istnieją porty)
   - Albo stworzyć warstwę abstrakcji z natywnymi API AmigaOS
   - Na początek: **stworzyć stub/wrapper pliki SDL** z natywną implementacją Amiga

2. **Warstwa wideo (`i_video.cpp`)**
   - Zamienić SDL_Window/Renderer/Texture na ekran RTG (Picasso96)
   - Gra renderuje do bufora 320×200×8bit → `WriteChunkyPixels()` lub `WriteLUTPixelArray()`
   - Paleta 256 kolorów → `LoadRGB32()` lub `SetRGB32()`
   - VSync / retrace → `WaitTOF()` lub przerwanie VERTB

3. **Warstwa audio (`fx.cpp`)**
   - Zamienić `SDL_OpenAudioDevice()` + callback na AHI
   - Mixer software'owy (DSP_Mix, MUS_Mix) pozostaje bez zmian
   - Potrzebny AHI double-buffering lub callback mode

4. **Timer**
   - `SDL_GetTicks()` → `ReadEClock()` (50-sza precyzja) lub `timer.device` (mikrosekund)
   - `SDL_Init(SDL_INIT_TIMER)` → `OpenDevice("timer.device", ...)`

### 🟡 WAŻNE (potrzebne do grania)

5. **Warstwa wejścia**
   - Klawiatura: `SDL_KEYDOWN/UP` → `IDCMP_RAWKEY` (IntuiMessage) lub `input.device`
   - Mysz: `SDL_MOUSEMOTION/BUTTON` → `IDCMP_MOUSEMOVE` / `IDCMP_MOUSEBUTTONS`
   - Joystick: `SDL_GameController` → `gameport.device` lub `lowlevel.library`
   - Mapowanie SDL scancode→DOS scancode już istnieje w `kbdapi.cpp` – potrzeba Amiga→DOS

6. **Ścieżki plików i system plików**
   - `\` → `/` (lub `:`) w separatorach ścieżek  
   - `strrchr(exePath, '\\')` w `glbapi.cpp` → obsłużyć też `/` i `:`
   - `SDL_GetPrefPath()` → `PROGDIR:` lub `S:Raptor/`
   - Case sensitivity: pliki `.GLB` vs `.glb` – AmigaOS jest case-insensitive, OK

7. **Wydajność na 68060**
   - 320×200×8bit software rendering powinno być OK
   - Software mixer 44100 Hz stereo → rozważyć obniżenie do 22050 Hz
   - OPL3 emulacja (`opl3.cpp`) – wymaga profilowania
   - TinySoundFont – znaczne użycie float, na 68060 FPU to ~10-20 MFLOPS
   - 68060 @50MHz ≈ ~100 MIPS – powinno wystarczyć przy optymalizacji

### 🟢 MNIEJ WAŻNE (mogą poczekać)

8. **Setup program (`raptorsetup`)**
   - Wymaga textscreen library (SDL2-based)
   - Odłożyć – konfiguracja przez ręczną edycję SETUP.INI

9. **SDL_ShowSimpleMessageBox()**
   - Zamienić na `EasyRequestArgs()` lub `printf()`

10. **Inne platformowe szczegóły**
    - `#ifdef __ANDROID__` / `#ifdef _WIN32` → dodać `#ifdef __AMIGA__`
    - `SDL_free()` → `free()`
    - `SDL_RWops` (Android file copy) → nie dotyczy

---

## 5. Architektura portu – rekomendowana struktura

```
src/
├── [istniejące pliki – bez zmian lub minimalne zmiany]
├── amiga/
│   ├── amiga_video.cpp     # Implementacja I_InitGraphics, I_FinishUpdate (RTG)
│   ├── amiga_audio.cpp     # Implementacja SND_InitSound via AHI
│   ├── amiga_input.cpp     # Klawiatura, mysz, joystick via Intuition/input.device
│   ├── amiga_timer.cpp     # SDL_GetTicks() replacement via timer.device
│   ├── amiga_system.cpp    # Inicjalizacja AmigaOS, ścieżki, czyszczenie
│   └── amiga_sdl_stubs.h   # Minimalne definicje SDL_SwapLE*, SDL_BYTEORDER, itp.
```

### Faza 1: Kompilacja i link (cel: uruchomienie programu)
- Stworzyć stubs SDL lub użyć SDL2 Amiga port
- Skompilować wszystkie pliki .cpp
- Zlinkowac z wymaganymi bibliotekami AmigaOS

### Faza 2: Wyświetlanie (cel: ekran tytułowy)
- Implementacja warstwy wideo (RTG)
- Test palety i wyświetlania sprite'ów

### Faza 3: Input + Audio (cel: grywalna gra)
- Klawiatura i mysz
- Mixer audio przez AHI
- Muzyka OPL3

### Faza 4: Optymalizacja
- Profilowanie wydajności
- Optymalizacja krytycznych pętli (mixer, renderer)
- Opcjonalne: 68060-specific assembly dla hot loops

---

## 6. Notatki dotyczące kompilatora m68k-amigaos-gcc

### Wymagane flagi:
```
-m68060           # Generuj kod dla 68060
-m68881           # Użyj FPU (68060 ma zintegrowane FPU)  
-O2               # Optymalizacja (nie O3 – może generować zbyt duży kod)
-fomit-frame-pointer  # Wolny rejestr a6
-noixemul         # Nie linkuj z ixemul (UNIX emulation), użyj natywnego libc
```

### Potencjalne problemy kompilacji:
- `long long` w `rap.cpp` (`wrand()`) – m68k-amigaos-gcc wspiera to
- `bool` / `true` / `false` – wymaga C++11 lub `<stdbool.h>`
- `auto` keyword (C++11) – `auto chan = &dsp_channels[i];` w `dspapi.cpp`
- `#pragma once` – wspierane przez GCC
- `#pragma pack(push, 1)` – wspierane, ale sprawdzić poprawność na m68k
- Default argument values w C++ (`void I_SetPalette(uint8_t *doompalette, int start = 0)`) – OK w C++

---

*Dokument przygotowany: lipiec 2026*
*Wersja kodu źródłowego: upstream skynettx/raptor*
