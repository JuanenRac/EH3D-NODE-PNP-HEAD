<p align="center">
  <img src="images/EH3D_BANNER.svg" alt="EH3D-NODE-PNP-HEAD banner" width="100%">
</p>

# 🛠️ EH3D-NODE-PNP-HEAD

<p align="center"><a href="README.md">🇺🇸 English</a> | <a href="README_spa.md">🇪🇸 Español</a> | <a href="README_fra.md">🇫🇷 Français</a> | <a href="README_ita.md">🇮🇹 Italiano</a> | <a href="README_deu.md">🇩🇪 Deutsch</a> | <a href="README_zho.md">🇨🇳 中文</a> | <a href="README_jpn.md">🇯🇵 日本語</a> | 🇵🇹 <b>Português</b></p>

### O nó da cabeça de uma máquina de pick and place da EH3D: bicos, vácuo, sopro, luzes e bloqueios, escrito e testado contra um simulador

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Stack-C%2B%2B17%20%2B%20Python-blue.svg" alt="C++17 and Python">
  <img src="https://img.shields.io/badge/Languages-8-00E5FF.svg" alt="Languages">
  <img src="https://img.shields.io/badge/Tests-103-2EA043.svg" alt="Tests">
  <img src="https://img.shields.io/badge/Version-0.0.4-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-functional-4CAF50.svg" alt="Maturity">
</p>

---

**Verificação de honestidade - o que funciona hoje:** um nó que funciona em simulação: lógica real e testada, sem firmware e sem hardware. As regras do nó correm num computador contra um simulador (103 testes passam), mas nada aqui foi compilado como firmware, gravado ou ligado a uma bomba, uma válvula, um sensor de pressão ou um eixo.

---

> **Estado de segurança (safety) e de segurança informática:** este nó não é um sistema de segurança. Os seus bloqueios e o seu estado seguro são uma segunda linha atrás da paragem, dos limites e dos bloqueios da máquina e do seu controlador, e nunca os substituem.

## 📋 Num relance

| | |
|---|---|
| Versão | `0.0.4` |
| Maturidade | Funcional |
| Testes | 103 |
| Tecnologias | C++17 (host tests), Python 3.10+ |
| Em hardware real | Ainda não executado em hardware |

**Ir para:** [Visão geral](#-visão-geral) · [Arquitetura](#-arquitetura) · [Início rápido](#-início-rápido) · [Testes](#-testes) · [Estrutura do repositório](#-estrutura-do-repositório) · [Projetos relacionados](#-projetos-relacionados)

## 🚦 Estado do projeto

Cada linha é uma parte deste repositório e a luz diz até onde chega hoje. É escrita com o que o repositório consegue demonstrar (testes, compilações, execuções), não com o que está previsto.

🟢 Feito e verificado · 🟡 Funciona, com limites conhecidos · 🟠 Feito a meio ou ainda por verificar · 🔴 Por fazer ou a não funcionar

| Área | Estado | O que significa aqui |
|---|---|---|
| Lógica e simulador | 🟡 | As regras do nó correm contra um simulador com semente fixa; 103 testes passam num computador. |
| Contrato de rede | 🟡 | As palavras do nó estão escritas em docs/CONTRACT.md e um teste falha quando a página e as respostas diferem; o plugin do servidor ainda não usa este nó. |
| Firmware | 🟠 | O firmware da placa ainda não está escrito: só foram importados os auxiliares simples da base partilhada. |
| Hardware real | 🔴 | Nada correu em uma bomba, uma válvula, um sensor de pressão ou um eixo. |

## 🎯 Visão geral

* **Um nó, uma cabeça:** recolhe e larga peças com o vácuo de cada bico, distingue uma peça perdida de uma que nunca lá esteve, move o eixo Z e a rotação sob bloqueios e passa sozinho a um estado seguro se perder o servidor.
* **Duas formas de medir o vácuo e um bloqueio dos eixos:** cada bico tem um sensor de pressão (filtrado, calibrado, com banda) ou um pressóstato (com antirressalto), à escolha em `head.cfg`, com alarmes de peça perdida, vácuo não atingido e sensor fora de gama ou congelado; a cabeça diz também quando todos os bicos estão levantados à altura segura, para a máquina se poder mover em X e Y, e recusa baixar um bico enquanto isso.

## 🧩 Papel no EH3D

O programa de pick and place planeia os trabalhos; este nó executará uma recolha e uma largada num bico quando uma máquina real for ligada. Hoje só existe como lógica e simulador.

## 🏛️ Arquitetura

<p align="center">
  <img src="images/architecture.svg" alt="Arquitetura de EH3D-NODE-PNP-HEAD: quem o usa, o que contém e de que depende" width="100%">
</p>

Como as partes deste repositório se encaixam: quem o usa, o que contém e de que depende. A imagem é desenhada a partir de `docs/architecture.json` e verificada com o código.

## 🔒 Segurança e proteção

As regras de segurança deste nó; o teste que mostra cada uma está nomeado em docs/REQUIREMENTS.md:

* Um nó sem token recusa todo o pedido que mova ou altere algo; o token nunca é mostrado, reescrito nem registado.
* Se não ouvir o servidor, o nó passa sozinho a um estado seguro e não move nada até uma pessoa confirmar. Enquanto uma ordem espera (um movimento longo, uma recolha longa), o nó continua à escuta: o servidor tem de continuar a perguntar, e uma ligação perdida a meio pára o movimento. O nó diz quanto é esse tempo sem que lho perguntem: `link_lost_ms` em `GET /api/status`, o valor das suas definições.
* Cada limite é uma definição com um valor por omissão prudente, e um valor errado nunca afrouxa um: é recusado e fica o valor por omissão.

## 🔌 Interfaces

O servidor, os programas e os telemóveis falam com o nó por HTTP como os outros nós da suite (JSON e um token bearer). A página [docs/CONTRACT.md](docs/CONTRACT.md) é o contrato e um teste mantém-na igual às respostas.

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

## ⚙️ Configuração

Cada valor afinado está em `head.cfg`; `head.cfg.example` mostra-os todos com os valores por omissão e os intervalos. Um valor fora do intervalo, ou uma chave que ninguém conhece, é assinalado e fica o valor por omissão.

## 🚀 Início rápido

Precisa de um compilador de C++17, CMake 3.16 ou mais recente e Python 3.10 ou mais recente; não há mais nada a instalar.

```bash
cmake -S tests -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
python -I tools/ci_validate.py
python -I -m unittest discover -s tools -p "test_*.py"
```

## 🧪 Testes

103 testes: testes de C++ no computador com semente fixa (executam o simulador e as palavras da rede e comparam a página do contrato com as respostas) e testes de Python do repositório. A CI também executa os de C++ com AddressSanitizer e UBSan.

## 🏗️ Compilação e publicação

A versão vive em `eh3d.project.json` e cada versão tem a sua entrada em [CHANGELOG.md](CHANGELOG.md). Ainda não há compilação de firmware; quando existir será compilado apenas no GitHub Actions. O software é GPL-3.0-or-later; as peças mecânicas do nó serão publicadas com CC-BY-SA e a eletrónica com CERN-OHL-S (ver [docs/HARDWARE.md](docs/HARDWARE.md)).

## 📂 Estrutura do repositório

<p align="center">
  <img src="images/repository-structure.svg" alt="Estrutura do repositório EH3D-NODE-PNP-HEAD" width="100%">
</p>

<details>
<summary>Versão em texto da árvore</summary>

```text
EH3D-NODE-PNP-HEAD/
├── core/
│   ├── head_config.hpp cfg_text.hpp                                                                                                         o ficheiro de definições e os seus valores seguros
│   ├── head.hpp                                                                                                                             bicos, bomba e eixos; vácuo analógico ou digital; o bloqueio X/Y
│   ├── head_node.hpp version.hpp                                                                                                            as palavras do nó na rede
│   ├── head_sim.hpp                                                                                                                         uma cabeça no computador: bomba, válvulas, peças, sensores e pressóstatos
│   └── json.hpp auth.hpp semver.hpp backoff.hpp task_health.hpp stored_load.hpp memory_budget.hpp net_text.hpp node_id.hpp civil_time.hpp   auxiliares simples partilhados pelos nós, mantidos iguais por uma ferramenta
├── tests/
│   ├── check.hpp CMakeLists.txt                                                                                                             a estrutura dos testes e como se compilam
│   ├── test_config.cpp                                                                                                                      o ficheiro de definições, o filtro e a calibração
│   ├── test_head.cpp                                                                                                                        a cabeça com bomba e sensores simulados: recolhas, perdas, bloqueios, estado seguro
│   ├── test_sensor.cpp                                                                                                                      vácuo analógico e digital, alarmes e o bloqueio X/Y
│   └── test_node.cpp                                                                                                                        as palavras da rede, o token, o estado seguro e a página do contrato
├── docs/
│   ├── CONTRACT.md                                                                                                                          o que o nó diz na rede (e na cadeia)
│   ├── REQUIREMENTS.md                                                                                                                      cada requisito e o teste que o demonstra
│   ├── SCOPE.md STATUS.md NOT_YET.md                                                                                                        para que serve, até onde vai, o que espera
│   ├── HARDWARE.md                                                                                                                          peças preliminares, testes em bancada e licenças
│   └── architecture.json structure.json                                                                                                     os dados das duas imagens
├── tools/                                                                                                                                   a verificação do repositório, os seus testes e as ferramentas das imagens
├── images/                                                                                                                                  banner e as imagens de Arquitetura e Estrutura do repositório
├── .github/                                                                                                                                 fluxos de CI e do painel, modelos de issues e pull requests
├── eh3d.project.json                                                                                                                        manifesto: versão, papel, maturidade e comando de compilação
├── head.cfg.example                                                                                                                         todas as definições com os valores por omissão e intervalos
├── VERSION                                                                                                                                  a versão do repositório
├── CHANGELOG.md                                                                                                                             o que mudou em cada versão
├── AGENTS.md                                                                                                                                regras para quem altera o repositório
├── test.sh test.bat                                                                                                                         executar os testes do computador
├── SECURITY.md SUPPORT.md                                                                                                                   como comunicar uma vulnerabilidade e onde perguntar
└── CONTRIBUTING.md CODE_OF_CONDUCT.md LICENSE                                                                                               como contribuir, o código de conduta e a licença
```

</details>

## 🗺️ Roteiro e limites conhecidos

**Limites conhecidos hoje**

* Não há firmware, saída de bomba ou válvula, leitura de pressão nem controlador de eixos escritos; o vácuo e o ruído simulados são inventados. Como o controlador da máquina sabe que os bicos estão levantados não está decidido.

**A seguir**

* O firmware ESP-IDF sobre a base partilhada, as saídas da bomba e das válvulas e a leitura dos sensores de pressão, e um primeiro teste em bancada de um bico.

## 🌍 Idiomas

O README e os textos existem em inglês, espanhol, francês, italiano, alemão, chinês, japonês e português europeu.

## 🔗 Projetos relacionados

* [EH3D-PHOENIX-PNP](https://github.com/JuanenRac/EH3D-PHOENIX-PNP): o programa de pick and place que planeia os trabalhos e descreve este nó
* [EH3D-SERVER](https://github.com/JuanenRac/EH3D-SERVER): o servidor cujo plugin das máquinas de pick and place ficará atrás deste nó
* [EH3D-COMMON](https://github.com/JuanenRac/EH3D-COMMON): a base de firmware de que este nó tira os auxiliares simples
* [EH3D-NODE-PNP-FEEDER](https://github.com/JuanenRac/EH3D-NODE-PNP-FEEDER): o nó dos alimentadores que trazem as peças

## 📚 Documentação

* [As convenções da suite](https://github.com/JuanenRac/EH3D-DOCS/blob/main/docs/CONVENTIONS.md)
* [O contrato do nó](docs/CONTRACT.md)
* [Requisitos e o teste de cada um](docs/REQUIREMENTS.md)
* [Âmbito](docs/SCOPE.md), [estado](docs/STATUS.md) e [o que ainda não está feito](docs/NOT_YET.md)
* [Hardware: peças preliminares, testes em bancada e licenças](docs/HARDWARE.md)
* [Registo de alterações](CHANGELOG.md)

## 🤝 Contribuir e comunidade

* Antes de um pull request, execute `python -I tools/ci_validate.py` e os testes da base: tudo tem de passar. Consulte [CONTRIBUTING.md](CONTRIBUTING.md) e o [código de conduta](CODE_OF_CONDUCT.md).
* O README existe em oito idiomas: uma alteração ao texto faz-se nos oito ficheiros, e as duas imagens são desenhadas a partir de `docs/architecture.json` e `docs/structure.json` com `tools/make_architecture.py` e `tools/make_structure.py`.
* Comunique uma vulnerabilidade em privado: [SECURITY.md](SECURITY.md). Perguntas, ideias e relatórios: electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENÇA

Software: GPL-3.0-or-later. Ver [LICENSE](LICENSE). Hardware: as peças mecânicas estão previstas com CC-BY-SA e a eletrónica com CERN-OHL-S (ver [docs/HARDWARE.md](docs/HARDWARE.md)).
