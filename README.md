# REC HUD — overlay di registrazione per OBS Studio

[English](README.en.md) | [Italiano](README.md)

Plugin nativo per OBS che mostra **un indicatore "REC" sempre in primo piano sul tuo desktop**, con la **durata della registrazione** (attivabile/disattivabile).

Pensato per il caso in cui stai registrando un gioco in **fullscreen** e OBS è nascosto: l'overlay resta visibile a te, sopra tutto, e non finisce nella registrazione.

## Funzionalità

- Pallino rosso pulsante + etichetta `REC` quando la registrazione è attiva
- **Durata** `MM:SS` (o `HH:MM:SS`) con toggle nelle impostazioni
- Indicatore `PAUSED` quando metti in pausa (il cronometro si **congela** e riprende da dove si era)
- Finestra frameless, trasparente, sempre in primo piano
- **Click-through**: il mouse passa attraverso, quindi non interferisce col gioco
- Posizionabile trascinandolo, oppure ancorato a un angolo dello schermo
- Personalizzazione di colori, dimensione testo, raggio angoli, margine
- Impostazioni salvate nel **profilo OBS** corrente
- Interfaccia e menu tradotti in italiano

## Requisiti

- Windows 10/11 x64
- [Visual Studio](https://visualstudio.microsoft.com/) 2019/2022/2026 — durante l'installazione seleziona il carico **"Sviluppo desktop con C++"**
- [CMake](https://cmake.org/download/) 3.28+ (`winget install Kitware.CMake`)
- **Non serve Git**: gli script scaricano il template ufficiale come archivio zip

Al primo avvio CMake scarica automaticamente le dipendenze (libobs, obs-frontend-api, Qt6): ~1 GB e qualche minuto.

## Compilazione

```powershell
# 1. Prepara il workspace (scarica obs-plugintemplate e applica le nostre fonti)
.\scripts\setup.ps1

# 2. Compila (rileva da solo il generator Visual Studio e la cartella di cmake)
.\scripts\build.ps1

# 3. Installa
.\scripts\install.ps1
```

### Se PowerShell blocca gli script

Se `.\scripts\setup.ps1` restituisce *"L'esecuzione di script è disabilitata nel sistema in uso"*:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

oppure, senza modificare le impostazioni, esegui ogni volta con:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup.ps1
```

### Dove viene installato

Per default in `C:\ProgramData\obs-studio\plugins\obs-rec-hud\`, che **non richiede privilegi di amministratore**:

```
bin\64bit\obs-rec-hud.dll
data\locale\en-US.ini
data\locale\it-IT.ini
```

Per installarlo nella cartella di OBS (richiede un PowerShell elevato):

```powershell
.\scripts\install.ps1 -Location InstallDir
```

## Uso

1. Avvia OBS Studio
2. **Strumenti → Impostazioni REC HUD...**
3. Scheda **Generale**:
   - **Mostra la durata della registrazione** → attiva/disattiva il cronometro
   - **Sempre in primo piano** → l'overlay resta sopra il gioco
   - **Clic trasparente** → il mouse passa attraverso l'overlay
   - **Sempre visibile** → utile per posizionarlo a mano
4. Scheda **Posizione** → scegli l'angolo (default: alto a destra)
5. Avvia la registrazione: l'overlay compare e il timer parte da `00:00`

Per spostarlo a mano: disattiva **Clic trasparente**, avvia una registrazione e trascina l'overlay. La posizione viene salvata nel profilo. Per tornare all'ancoraggio automatico usa **Strumenti > REC HUD: ripristina posizione** (oppure seleziona un angolo nella scheda *Posizione*).

## Diagnostica

Il plugin scrive alcune righe nel log di OBS
(`%APPDATA%\obs-studio\logs\*.txt`):

```
[rec-hud] loaded
[rec-hud] tools menu entries added
[rec-hud] recording started, overlay shown
[rec-hud] recording stopped
```

Se l'overlay non compare, controlla che `Enabled` sia attivo nelle impostazioni e che non ci sia una posizione manuale salvata che lo nasconda fuori schermo: usa **Ripristina posizione**.

## Note tecniche

- L'overlay è una finestra Qt `Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint` creata dal plugin stesso: è una vera integrazione in OBS (parte automaticamente, sparisce alla chiusura).
- Per lo stato si usano gli eventi frontend `OBS_FRONTEND_EVENT_RECORDING_STARTED/STOPPED/PAUSED/UNPAUSED`, più un timer da 1 s come rete di sicurezza (hotkey, cambio profilo, reload del plugin).
- L'ancoraggio all'angolo è **differito di un tick**: dimensionare la finestra e riposizionarla nello stesso ciclo fa sì che Windows applichi il resize dopo il move, spingendo la finestra fuori schermo quando la label si allunga.
- Il cronometro usa un accumulatore: `QElapsedTimer::restart()` azzera il contatore, quindi il tempo trascorso durante le pause viene sommato a parte.
- **Non viene mai catturato** nella registrazione, a meno che tu non catturi specificamente quella finestra (Window Capture / "Acquisisci finestra").
- Le impostazioni sono per-profilo: ogni profilo OBS ha la sua configurazione.

## Struttura del progetto

```
CMakeLists.txt          build (derivato da obs-plugintemplate)
buildspec.json          nome, versione e dipendenze
CMakePresets.json       preset per uso con l'IDE / CMake GUI
src/
  plugin-main.cpp       entry point del modulo
  rec-hud.{h,cpp}       controller: eventi OBS, polling, persistenza
  rec-hud-config.h      struct di configurazione
  rec-hud-overlay.*     finestra overlay (QPainter)
  rec-hud-settings.*    finestra di impostazione
data/locale/            en-US.ini, it-IT.ini
scripts/                setup.ps1, build.ps1, install.ps1
```

## Licenza

GPL-2.0-or-later, come OBS Studio.
