<p align="center">
  <img src="images/EH3D_BANNER.svg" alt="EH3D-NODE-PNP-HEAD banner" width="100%">
</p>

# 🛠️ EH3D-NODE-PNP-HEAD

<p align="center"><a href="README.md">🇺🇸 English</a> | <a href="README_spa.md">🇪🇸 Español</a> | 🇫🇷 <b>Français</b> | <a href="README_ita.md">🇮🇹 Italiano</a> | <a href="README_deu.md">🇩🇪 Deutsch</a> | <a href="README_zho.md">🇨🇳 中文</a> | <a href="README_jpn.md">🇯🇵 日本語</a> | <a href="README_por.md">🇵🇹 Português</a></p>

### Le nœud de la tête d'une machine de placement EH3D : buses, vide, jet d'air, éclairages et verrouillages, écrit et essayé contre un simulateur

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Stack-C%2B%2B17%20%2B%20Python-blue.svg" alt="C++17 and Python">
  <img src="https://img.shields.io/badge/Languages-8-00E5FF.svg" alt="Languages">
  <img src="https://img.shields.io/badge/Tests-85-2EA043.svg" alt="Tests">
  <img src="https://img.shields.io/badge/Version-0.0.2-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-FFB020.svg" alt="Maturity">
</p>

---

**Vérification d'honnêteté - ce qui tourne aujourd'hui :** un squelette avec une logique réelle et testée et sans matériel. Les règles du nœud s'exécutent sur un ordinateur contre un simulateur (85 tests réussis), mais rien ici n'a été compilé en firmware, flashé ni relié à une pompe, une vanne, un capteur de pression ou un axe.

---

> **État de la sûreté et de la sécurité :** ce nœud n'est pas un système de sécurité. Ses verrouillages et son état sûr sont une seconde ligne derrière l'arrêt, les limites et les verrouillages de la machine et de son contrôleur, et ne les remplacent jamais.

## 📋 En un coup d'œil

| | |
|---|---|
| Version | `0.0.2` |
| Maturité | Échafaudage |
| Tests | 85 |
| Pile technique | C++17 (host tests), Python 3.10+ |
| Sur du matériel réel | Pas encore exécuté sur du matériel |

**Aller à  :** [Présentation](#-présentation) · [Architecture](#-architecture) · [Démarrage rapide](#-démarrage-rapide) · [Tests](#-tests) · [Structure du dépôt](#-structure-du-dépôt) · [Projets liés](#-projets-liés)

## 🚦 État du projet

Chaque ligne est une partie de ce dépôt et le feu dit où elle en est aujourd'hui. Elle est écrite d'après ce que le dépôt peut montrer (tests, compilations, exécutions), pas d'après ce qui est prévu.

🟢 Fait et vérifié · 🟡 Fonctionne, avec des limites connues · 🟠 À moitié fait ou pas encore vérifié · 🔴 Pas fait ou ne fonctionne pas

| Domaine | État | Ce que cela veut dire ici |
|---|---|---|
| Logique et simulateur | 🟡 | Les règles du nœud s'exécutent contre un simulateur à graine fixe ; 85 tests réussissent sur un ordinateur. |
| Contrat réseau | 🟡 | Les mots du nœud sont écrits dans docs/CONTRACT.md et un test échoue quand la page et les réponses diffèrent ; le plugin du serveur n'utilise pas encore ce nœud. |
| Firmware | 🟠 | Le firmware de la carte n'est pas encore écrit : seuls les utilitaires simples de la base commune sont importés. |
| Matériel réel | 🔴 | Rien n'a tourné sur une pompe, une vanne, un capteur de pression ou un axe. |

## 🎯 Présentation

* **Un nœud, une tête :** il prend et dépose des pièces avec le vide de chaque buse, distingue une pièce perdue d'une pièce jamais là, déplace l'axe Z et la rotation sous verrouillages et passe seul à un état sûr s'il perd le serveur.
* **Deux façons de mesurer le vide et un verrou des axes :** chaque buse a un capteur de pression (filtré, étalonné, avec une bande) ou un vacuostat (anti-rebond), au choix dans `head.cfg`, avec des alarmes pour pièce perdue, vide non atteint et capteur hors plage ou figé ; la tête dit aussi quand toutes les buses sont relevées à la hauteur sûre, pour que la machine puisse se déplacer en X et Y, et refuse d'abaisser une buse pendant ce temps.

## 🧩 Rôle dans EH3D

Le programme de placement planifie les travaux ; ce nœud exécutera une prise et une dépose sur une buse quand une machine réelle sera reliée. Aujourd'hui il n'existe que comme logique et simulateur.

## 🏛️ Architecture

<p align="center">
  <img src="images/architecture.svg" alt="Architecture de EH3D-NODE-PNP-HEAD : qui l'utilise, ce qu'il contient et de quoi il dépend" width="100%">
</p>

Comment les parties de ce dépôt s'articulent : qui l'utilise, ce qu'il contient et de quoi il dépend. L'image est dessinée à partir de `docs/architecture.json` et vérifiée par rapport au code.

## 🔒 Sécurité et protection

Les règles de sûreté et de sécurité de ce nœud ; le test qui montre chacune est nommé dans docs/REQUIREMENTS.md :

* Un nœud sans jeton refuse toute requête qui déplace ou change quelque chose ; le jeton n'est jamais affiché, réécrit ni journalisé.
* Quand le serveur n'est plus entendu, le nœud passe seul à un état sûr et ne déplace rien tant qu'une personne n'a pas acquitté.
* Chaque limite est un réglage à valeur par défaut prudente, et une valeur erronée n'en relâche jamais une : elle est refusée et la valeur par défaut reste.

## 🔌 Interfaces

Le serveur, les programmes et les téléphones parlent au nœud en HTTP comme les autres nœuds de la suite (JSON et un jeton bearer). La page [docs/CONTRACT.md](docs/CONTRACT.md) est le contrat et un test la garde identique aux réponses.

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

## ⚙️ Configuration

Chaque valeur ajustée est dans `head.cfg` ; `head.cfg.example` les montre toutes avec leurs valeurs par défaut et leurs plages. Une valeur hors plage, ou une clé inconnue, est signalée et la valeur par défaut reste.

## 🚀 Démarrage rapide

Il faut un compilateur C++17, CMake 3.16 ou plus récent et Python 3.10 ou plus récent ; rien d'autre à installer.

```bash
cmake -S tests -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
python -I tools/ci_validate.py
python -I -m unittest discover -s tools -p "test_*.py"
```

## 🧪 Tests

85 tests : tests C++ sur ordinateur à graine fixe (ils font tourner le simulateur et les mots du réseau et comparent la page du contrat aux réponses) et tests Python du dépôt. La CI exécute aussi les tests C++ avec AddressSanitizer et UBSan.

## 🏗️ Compilation et publication

La version vit dans `eh3d.project.json` et chaque version a son entrée dans [CHANGELOG.md](CHANGELOG.md). Il n'y a pas encore de compilation du firmware ; quand il existera, il sera compilé seulement dans GitHub Actions. Le logiciel est sous GPL-3.0-or-later ; les pièces mécaniques du nœud seront publiées sous CC-BY-SA et son électronique sous CERN-OHL-S (voir [docs/HARDWARE.md](docs/HARDWARE.md)).

## 📂 Structure du dépôt

<p align="center">
  <img src="images/repository-structure.svg" alt="Structure du dépôt EH3D-NODE-PNP-HEAD" width="100%">
</p>

<details>
<summary>Version texte de l'arborescence</summary>

```text
EH3D-NODE-PNP-HEAD/
├── core/
│   ├── head_config.hpp cfg_text.hpp                                                                                                         le fichier de réglages et ses valeurs sûres
│   ├── head.hpp                                                                                                                             buses, pompe et axes ; vide analogique ou numérique ; le verrou X/Y
│   ├── head_node.hpp version.hpp                                                                                                            les mots du nœud sur le réseau
│   ├── head_sim.hpp                                                                                                                         une tête sur ordinateur : pompe, vannes, pièces, capteurs et pressostats
│   └── json.hpp auth.hpp semver.hpp backoff.hpp task_health.hpp stored_load.hpp memory_budget.hpp net_text.hpp node_id.hpp civil_time.hpp   utilitaires simples partagés par les nœuds, gardés identiques par un outil
├── tests/
│   ├── check.hpp CMakeLists.txt                                                                                                             le cadre des tests et leur compilation
│   ├── test_config.cpp                                                                                                                      le fichier de réglages, le filtre et l'étalonnage
│   ├── test_head.cpp                                                                                                                        la tête avec pompe et capteurs simulés : prises, pertes, verrouillages, état sûr
│   ├── test_sensor.cpp                                                                                                                      vide analogique et numérique, alarmes et verrou X/Y
│   └── test_node.cpp                                                                                                                        les mots du réseau, le jeton, l'état sûr et la page du contrat
├── docs/
│   ├── CONTRACT.md                                                                                                                          ce que dit le nœud sur le réseau (et sur la chaîne)
│   ├── REQUIREMENTS.md                                                                                                                      chaque exigence et le test qui la montre
│   ├── SCOPE.md STATUS.md NOT_YET.md                                                                                                        à quoi il sert, où il en est, ce qui attend
│   ├── HARDWARE.md                                                                                                                          pièces préliminaires, essais sur banc et licences
│   └── architecture.json structure.json                                                                                                     les données des deux images
├── tools/                                                                                                                                   le contrôle du dépôt, ses tests et les outils des images
├── images/                                                                                                                                  bannière et images d'Architecture et de Structure du dépôt
├── .github/                                                                                                                                 workflows de CI et du tableau de bord, modèles d'issues et de pull requests
├── eh3d.project.json                                                                                                                        manifeste : version, rôle, maturité et commande de compilation
├── head.cfg.example                                                                                                                         tous les réglages avec leurs valeurs par défaut et plages
├── VERSION                                                                                                                                  la version du dépôt
├── CHANGELOG.md                                                                                                                             ce qui a changé dans chaque version
├── AGENTS.md                                                                                                                                règles pour quiconque modifie le dépôt
├── test.sh test.bat                                                                                                                         lancer les tests sur ordinateur
├── SECURITY.md SUPPORT.md                                                                                                                   signaler une vulnérabilité et poser des questions
└── CONTRIBUTING.md CODE_OF_CONDUCT.md LICENSE                                                                                               contribuer, code de conduite et licence
```

</details>

## 🗺️ Feuille de route et limites connues

**Limites connues aujourd'hui**

* Ni firmware, ni sortie de pompe ou de vanne, ni lecture de pression, ni pilote d'axe ne sont écrits ; le vide et le bruit simulés sont inventés. La façon dont le contrôleur de la machine apprend que les buses sont relevées n'est pas décidée.

**Ensuite**

* Le firmware ESP-IDF sur la base commune, les sorties de la pompe et des vannes et la lecture des capteurs de pression, puis un premier essai sur banc d'une buse.

## 🌍 Langues

Le README et les textes existent en anglais, espagnol, français, italien, allemand, chinois, japonais et portugais européen.

## 🔗 Projets liés

* [EH3D-PHOENIX-PNP](https://github.com/JuanenRac/EH3D-PHOENIX-PNP) : le programme de placement qui planifie les travaux et décrit ce nœud
* [EH3D-SERVER](https://github.com/JuanenRac/EH3D-SERVER) : le serveur dont le plugin des machines de placement sera derrière ce nœud
* [EH3D-COMMON](https://github.com/JuanenRac/EH3D-COMMON) : la base de firmware dont ce nœud tire ses utilitaires simples
* [EH3D-NODE-PNP-FEEDER](https://github.com/JuanenRac/EH3D-NODE-PNP-FEEDER) : le nœud des alimentateurs qui amènent les pièces

## 📚 Documentation

* [Les conventions de la suite](https://github.com/JuanenRac/EH3D-DOCS/blob/main/docs/CONVENTIONS.md)
* [Le contrat du nœud](docs/CONTRACT.md)
* [Exigences et le test de chacune](docs/REQUIREMENTS.md)
* [Périmètre](docs/SCOPE.md), [état](docs/STATUS.md) et [ce qui n'est pas encore fait](docs/NOT_YET.md)
* [Matériel : pièces préliminaires, essais sur banc et licences](docs/HARDWARE.md)
* [Journal des modifications](CHANGELOG.md)

## 🤝 Contribuer et communauté

* Avant une pull request, exécutez `python -I tools/ci_validate.py` et les tests de la base : tout doit réussir. Voir [CONTRIBUTING.md](CONTRIBUTING.md) et le [code de conduite](CODE_OF_CONDUCT.md).
* Le README existe en huit langues : une modification du texte se fait dans les huit fichiers, et les deux images sont dessinées à partir de `docs/architecture.json` et `docs/structure.json` par `tools/make_architecture.py` et `tools/make_structure.py`.
* Signalez une vulnérabilité en privé : [SECURITY.md](SECURITY.md). Questions, idées et rapports : electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTEUR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENCE

Logiciel : GPL-3.0-or-later. Voir [LICENSE](LICENSE). Matériel : les pièces mécaniques sont prévues sous CC-BY-SA et l'électronique sous CERN-OHL-S (voir [docs/HARDWARE.md](docs/HARDWARE.md)).
