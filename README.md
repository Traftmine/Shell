# 🐚 Mini-Shell en C

Ce projet est un interpréteur de commandes (shell) minimaliste développé en C. Il permet d'exécuter des commandes en mode interactif, tout en affichant une invite dynamique avec des informations système.

## 🚀 Fonctionnalités

- Invite dynamique personnalisée :
  - Affiche le nom d’utilisateur, la machine, le dossier courant et le code de retour
  - Code couleur : vert si succès (`0`), rouge sinon
- Historique de commandes grâce à la bibliothèque `readline`
- Gestion propre de `Ctrl+D` (EOF)
- Parsing et évaluation d'expressions shell
- Affichage de l’arbre syntaxique (si activé)
- Mode interactif ou non interactif

## 🛠️ Dépendances

Assurez-vous d’avoir les bibliothèques suivantes :

- `libreadline-dev`
- `bison`
- `flex`

### Installation des dépendances sous Debian/Ubuntu :

```bash
sudo apt update
sudo apt install libreadline-dev bison flex
```

## 🔧 Compilation
Clonez le dépôt et compilez le projet avec `make` (si un Makefile est fourni) :

```bash
git clone https://github.com/Traftmine/Shell.git
cd Shell
make
```

Sinon, compilez manuellement :

```bash
gcc Shell.c Display.c Evaluation.c -o shell -lreadline
```

## 🧪 Utilisation

Lancez le shell avec : 
```bash
./shell
```

Exemples :

```bash
ls -l
echo "Bonjour"
false
```

Si une commande réussit, le prompt affichera :
```bash
user@machine dossier(0) %
```
Sinon :
```bash
user@machine dossier(1) %
```

## 📁 Structure du projet
```bash
Shell/
├── Shell.c           # Programme principal
├── Shell.h           # Structure globale
├── Display.c/.h      # Affichage de l'arbre syntaxique
├── Evaluation.c/.h   # Évaluation des commandes
├── lexer.l           # Analyse lexicale (si utilisé)
├── parser.y          # Analyse syntaxique (si utilisé)
├── Makefile          # Compilation
└── README.md         # Ce fichier
```

## 👨‍💻 Auteurs

Projet développé à but pédagogique par [@Traftmine](https://github.com/Traftmine).

## 📜 Licence

Ce projet est distribué sous licence MIT. Voir `LICENSE` pour plus d'informations.
