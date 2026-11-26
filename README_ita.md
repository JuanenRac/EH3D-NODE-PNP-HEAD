<p align="center">
  <img src="images/EH3D_BANNER.svg" alt="EH3D-NODE-PNP-HEAD banner" width="100%">
</p>

# 🛠️ EH3D-NODE-PNP-HEAD

<p align="center"><a href="README.md">🇺🇸 English</a> | <a href="README_spa.md">🇪🇸 Español</a> | <a href="README_fra.md">🇫🇷 Français</a> | 🇮🇹 <b>Italiano</b> | <a href="README_deu.md">🇩🇪 Deutsch</a> | <a href="README_zho.md">🇨🇳 中文</a> | <a href="README_jpn.md">🇯🇵 日本語</a> | <a href="README_por.md">🇵🇹 Português</a></p>

### Il nodo della testa di una macchina pick and place EH3D: ugelli, vuoto, soffio, luci e interblocchi, scritto e provato contro un simulatore

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Stack-C%2B%2B17%20%2B%20Python-blue.svg" alt="C++17 and Python">
  <img src="https://img.shields.io/badge/Languages-8-00E5FF.svg" alt="Languages">
  <img src="https://img.shields.io/badge/Tests-98-2EA043.svg" alt="Tests">
  <img src="https://img.shields.io/badge/Version-0.0.3-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-functional-4CAF50.svg" alt="Maturity">
</p>

---

**Verifica di onestà - cosa funziona oggi:** un nodo che funziona in simulazione: logica reale e provata, senza firmware e senza hardware. Le regole del nodo girano su un computer contro un simulatore (98 test superati), ma nulla qui è stato compilato come firmware, scritto su una scheda o collegato a una pompa, una valvola, un sensore di pressione o un asse.

---

> **Stato di sicurezza funzionale e informatica:** questo nodo non è un sistema di sicurezza. I suoi interblocchi e il suo stato sicuro sono una seconda linea dietro l'arresto, i limiti e gli interblocchi della macchina e del suo controllore, e non li sostituiscono mai.

## 📋 In breve

| | |
|---|---|
| Versione | `0.0.3` |
| Maturità | Funzionale |
| Test | 98 |
| Stack tecnologico | C++17 (host tests), Python 3.10+ |
| Su hardware reale | Non ancora eseguito su hardware |

**Vai a:** [Panoramica](#-panoramica) · [Architettura](#-architettura) · [Avvio rapido](#-avvio-rapido) · [Test](#-test) · [Struttura del repository](#-struttura-del-repository) · [Progetti correlati](#-progetti-correlati)

## 🚦 Stato del progetto

Ogni riga è una parte di questo repository e il semaforo dice a che punto è oggi. È scritta in base a ciò che il repository può mostrare (test, compilazioni, esecuzioni), non a ciò che è previsto.

🟢 Fatto e verificato · 🟡 Funziona, con limiti noti · 🟠 Fatto a metà o non ancora verificato · 🔴 Non fatto o non funzionante

| Area | Stato | Cosa significa qui |
|---|---|---|
| Logica e simulatore | 🟡 | Le regole del nodo girano contro un simulatore con seme fisso; 98 test superati su un computer. |
| Contratto di rete | 🟡 | Le parole del nodo sono scritte in docs/CONTRACT.md e un test fallisce quando pagina e risposte differiscono; il plugin del server non usa ancora questo nodo. |
| Firmware | 🟠 | Il firmware della scheda non è ancora scritto: sono importati solo gli aiuti semplici della base condivisa. |
| Hardware reale | 🔴 | Nulla è girato su una pompa, una valvola, un sensore di pressione o un asse. |

## 🎯 Panoramica

* **Un nodo, una testa:** preleva e rilascia i pezzi con il vuoto di ogni ugello, distingue un pezzo perso da uno mai presente, muove l'asse Z e la rotazione sotto interblocchi e passa da solo a uno stato sicuro se perde il server.
* **Due modi di rilevare il vuoto e un blocco degli assi:** ogni ugello ha un sensore di pressione (filtrato, calibrato, con banda) o un vuotostato (con antirimbalzo), a scelta in `head.cfg`, con allarmi per pezzo perso, vuoto non raggiunto e sensore fuori range o bloccato; la testa dice anche quando tutti gli ugelli sono sollevati all'altezza sicura, così la macchina può muoversi in X e Y, e rifiuta di abbassare un ugello nel frattempo.

## 🧩 Ruolo in EH3D

Il programma di pick and place pianifica i lavori; questo nodo eseguirà un prelievo e un rilascio su un ugello quando verrà collegata una macchina reale. Oggi esiste solo come logica e simulatore.

## 🏛️ Architettura

<p align="center">
  <img src="images/architecture.svg" alt="Architettura di EH3D-NODE-PNP-HEAD: chi lo usa, che cosa contiene e da che cosa dipende" width="100%">
</p>

Come si incastrano le parti di questo repository: chi lo usa, che cosa contiene e da che cosa dipende. L'immagine è disegnata da `docs/architecture.json` e verificata rispetto al codice.

## 🔒 Sicurezza e protezione

Le regole di sicurezza di questo nodo; il test che mostra ciascuna è nominato in docs/REQUIREMENTS.md:

* Un nodo senza token rifiuta ogni richiesta che muove o cambia qualcosa; il token non viene mai mostrato, riscritto né registrato.
* Se non sente il server, il nodo passa da solo a uno stato sicuro e non muove nulla finché una persona non conferma. Mentre un ordine attende (un movimento lungo, un prelievo lungo) il nodo continua ad ascoltare: il server deve continuare a chiedere, e un collegamento perso a metà ferma il movimento. Il nodo dice quanto dura questo tempo senza che glielo si chieda: `link_lost_ms` in `GET /api/status`, il valore delle sue impostazioni.
* Ogni limite è un'impostazione con un valore predefinito prudente, e un valore sbagliato non ne allenta mai uno: viene rifiutato e resta il valore predefinito.

## 🔌 Interfacce

Il server, i programmi e i telefoni parlano al nodo via HTTP come gli altri nodi della suite (JSON e un token bearer). La pagina [docs/CONTRACT.md](docs/CONTRACT.md) è il contratto e un test la tiene uguale alle risposte.

```text
GET  /api/status                         the head and all its nozzles (always answered)
GET  /api/nozzle?n=N                     one nozzle
POST /api/nozzle/pick?n=N                open the valve; answer when the vacuum says if a part is held
POST /api/nozzle/place?n=N               release the part with a puff (only low enough)
POST /api/nozzle/blow?n=N                a puff alone (not while a part is held)
POST /api/z?n=N&mm=H                     move the nozzle to the height H
POST /api/rotate?n=N&deg=D               turn the nozzle (only high enough)
POST /api/light?up=P&down=Q              the two lights, in percent
POST /api/stop                           stop every move (the vacuum is not touched)
POST /api/ack[?n=N]                      clear a fault that is gone (or the safe state, without n)
POST /api/vacuum/calibrate?n=N&kind=zero|scale[&kpa=K]   if the settings allow it
POST /api/xy?moving=1|0                    the machine asks to move in X and Y (only with every nozzle raised) or says it stopped
```

## ⚙️ Configurazione

Ogni valore calibrato è in `head.cfg`; `head.cfg.example` li mostra tutti con valori predefiniti e intervalli. Un valore fuori intervallo, o una chiave sconosciuta, viene segnalato e resta il valore predefinito.

## 🚀 Avvio rapido

Servono un compilatore C++17, CMake 3.16 o successivo e Python 3.10 o successivo; non c'è altro da installare.

```bash
cmake -S tests -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
python -I tools/ci_validate.py
python -I -m unittest discover -s tools -p "test_*.py"
```

## 🧪 Test

98 test: test C++ su computer con seme fisso (eseguono il simulatore e le parole della rete e confrontano la pagina del contratto con le risposte) e test Python del repository. La CI esegue i test C++ anche con AddressSanitizer e UBSan.

## 🏗️ Compilazione e rilascio

La versione vive in `eh3d.project.json` e ogni versione ha la sua voce in [CHANGELOG.md](CHANGELOG.md). Non c'è ancora una compilazione del firmware; quando esisterà sarà compilato solo in GitHub Actions. Il software è GPL-3.0-or-later; le parti meccaniche del nodo saranno rilasciate con CC-BY-SA e la sua elettronica con CERN-OHL-S (vedi [docs/HARDWARE.md](docs/HARDWARE.md)).

## 📂 Struttura del repository

<p align="center">
  <img src="images/repository-structure.svg" alt="Struttura del repository EH3D-NODE-PNP-HEAD" width="100%">
</p>

<details>
<summary>Versione testuale dell'albero</summary>

```text
EH3D-NODE-PNP-HEAD/
├── core/
│   ├── head_config.hpp cfg_text.hpp                                                                                                         il file delle impostazioni e i suoi valori sicuri
│   ├── head.hpp                                                                                                                             ugelli, pompa e assi; vuoto analogico o digitale; il blocco X/Y
│   ├── head_node.hpp version.hpp                                                                                                            le parole del nodo sulla rete
│   ├── head_sim.hpp                                                                                                                         una testa sul computer: pompa, valvole, pezzi, sensori e pressostati
│   └── json.hpp auth.hpp semver.hpp backoff.hpp task_health.hpp stored_load.hpp memory_budget.hpp net_text.hpp node_id.hpp civil_time.hpp   aiuti semplici condivisi dai nodi, tenuti uguali da uno strumento
├── tests/
│   ├── check.hpp CMakeLists.txt                                                                                                             la struttura delle prove e come si compilano
│   ├── test_config.cpp                                                                                                                      il file delle impostazioni, il filtro e la calibrazione
│   ├── test_head.cpp                                                                                                                        la testa con pompa e sensori simulati: prelievi, perdite, interblocchi, stato sicuro
│   ├── test_sensor.cpp                                                                                                                      vuoto analogico e digitale, allarmi e blocco X/Y
│   └── test_node.cpp                                                                                                                        le parole della rete, il token, lo stato sicuro e la pagina del contratto
├── docs/
│   ├── CONTRACT.md                                                                                                                          cosa dice il nodo sulla rete (e sulla catena)
│   ├── REQUIREMENTS.md                                                                                                                      ogni requisito e il test che lo dimostra
│   ├── SCOPE.md STATUS.md NOT_YET.md                                                                                                        a cosa serve, a che punto è, cosa aspetta
│   ├── HARDWARE.md                                                                                                                          parti preliminari, prove al banco e licenze
│   └── architecture.json structure.json                                                                                                     i dati delle due immagini
├── tools/                                                                                                                                   il controllo del repository, i suoi test e gli strumenti delle immagini
├── images/                                                                                                                                  banner e le immagini di Architettura e Struttura del repository
├── .github/                                                                                                                                 workflow di CI e della dashboard, modelli di issue e pull request
├── eh3d.project.json                                                                                                                        manifesto: versione, ruolo, maturità e comando di compilazione
├── head.cfg.example                                                                                                                         tutte le impostazioni con valori predefiniti e intervalli
├── VERSION                                                                                                                                  la versione del repository
├── CHANGELOG.md                                                                                                                             cosa è cambiato in ogni versione
├── AGENTS.md                                                                                                                                regole per chi modifica il repository
├── test.sh test.bat                                                                                                                         eseguire le prove sul computer
├── SECURITY.md SUPPORT.md                                                                                                                   come segnalare una vulnerabilità e dove chiedere
└── CONTRIBUTING.md CODE_OF_CONDUCT.md LICENSE                                                                                               come contribuire, codice di condotta e licenza
```

</details>

## 🗺️ Roadmap e limiti noti

**Limiti noti oggi**

* Non sono scritti firmware, uscita di pompa o valvola, lettura di pressione né driver degli assi; il vuoto e il rumore simulati sono inventati. Come il controllore della macchina sappia che gli ugelli sono alzati non è deciso.

**Prossimi passi**

* Il firmware ESP-IDF sulla base condivisa, le uscite della pompa e delle valvole e la lettura dei sensori di pressione, poi una prima prova al banco di un ugello.

## 🌍 Lingue

Il README e i testi esistono in inglese, spagnolo, francese, italiano, tedesco, cinese, giapponese e portoghese europeo.

## 🔗 Progetti correlati

* [EH3D-PHOENIX-PNP](https://github.com/JuanenRac/EH3D-PHOENIX-PNP): il programma di pick and place che pianifica i lavori e descrive questo nodo
* [EH3D-SERVER](https://github.com/JuanenRac/EH3D-SERVER): il server il cui plugin per le macchine pick and place starà dietro questo nodo
* [EH3D-COMMON](https://github.com/JuanenRac/EH3D-COMMON): la base di firmware da cui questo nodo prende gli aiuti semplici
* [EH3D-NODE-PNP-FEEDER](https://github.com/JuanenRac/EH3D-NODE-PNP-FEEDER): il nodo degli alimentatori che portano i pezzi

## 📚 Documentazione

* [Le convenzioni della suite](https://github.com/JuanenRac/EH3D-DOCS/blob/main/docs/CONVENTIONS.md)
* [Il contratto del nodo](docs/CONTRACT.md)
* [Requisiti e il test di ciascuno](docs/REQUIREMENTS.md)
* [Ambito](docs/SCOPE.md), [stato](docs/STATUS.md) e [cosa non è ancora fatto](docs/NOT_YET.md)
* [Hardware: parti preliminari, prove al banco e licenze](docs/HARDWARE.md)
* [Registro delle modifiche](CHANGELOG.md)

## 🤝 Contribuire e comunità

* Prima di una pull request esegui `python -I tools/ci_validate.py` e i test della base: tutto deve passare. Vedi [CONTRIBUTING.md](CONTRIBUTING.md) e il [codice di condotta](CODE_OF_CONDUCT.md).
* Il README esiste in otto lingue: una modifica al testo si fa in tutti e otto i file, e le due immagini sono disegnate da `docs/architecture.json` e `docs/structure.json` con `tools/make_architecture.py` e `tools/make_structure.py`.
* Segnala una vulnerabilità in privato: [SECURITY.md](SECURITY.md). Domande, idee e segnalazioni: electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTORE

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENZA

Software: GPL-3.0-or-later. Vedi [LICENSE](LICENSE). Hardware: le parti meccaniche sono previste con CC-BY-SA e l'elettronica con CERN-OHL-S (vedi [docs/HARDWARE.md](docs/HARDWARE.md)).
