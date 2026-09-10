<p align="center">
  <a href="https://voicelala.com/">
    <img src="docs/assets/rvc-cpp-banner.png" alt="RVC.cpp — Native voice conversion by VoiceLala" width="100%">
  </a>
</p>

<h1 align="center">RVC.cpp</h1>
<p align="center"><strong>La conversion vocale de type RVC en C++ pour vos applications.</strong></p>

<p align="center">
  <a href="https://github.com/VoiceLala/rvc-cpp/actions/workflows/ci.yml"><img src="https://github.com/VoiceLala/rvc-cpp/actions/workflows/ci.yml/badge.svg" alt="Windows CPU build"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-8b5cf6" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-17-3b82f6" alt="C++17">
  <img src="https://img.shields.io/badge/status-experimental-f59e0b" alt="Experimental">
  <a href="https://voicelala.com/"><img src="https://img.shields.io/badge/VoiceLala-visit%20website-06b6d4" alt="Visit VoiceLala"></a>
</p>

[English](README.md) · [简体中文](README.zh-CN.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [Deutsch](README.de.md) · [Français](README.fr.md) · [Español](README.es.md) · [Português](README.pt.md)

---

RVC.cpp est une bibliothèque de conversion vocale locale fondée sur C++17 et ONNX Runtime. Elle relie un encodeur de contenu, un modèle F0 et un synthétiseur de type RVC via une ABI C. Python n’est pas nécessaire à l’exécution de l’inférence.

Il s’agit d’une implémentation indépendante, et non d’une version C++ officielle de RVC Project ni d’un portage complet du projet Python. Les symboles C, cibles CMake et exécutables conservent le nom `dvc` pour préserver la compatibilité. La documentation technique détaillée est actuellement principalement en chinois.

## Fonctionnalités

- **ABI C** : chemins de modèles UTF-8, codes d’erreur explicites et intégration FFI.
- **Hors ligne et en flux** : clips entiers ou blocs fixes avec contexte, alignement SOLA et fondu enchaîné.
- **Réglages vocaux** : hauteur, identifiant du locuteur, intensité du bruit et graine aléatoire.
- **Instances indépendantes** : sessions de modèles et état propres à chaque contexte. Intégration CMake et exemple WAV inclus.

## Chaîne de traitement

Le contenu et la F0 sont extraits de l’audio puis transmis au modèle de synthèse vocale. Le mode en flux ajoute historique, alignement SOLA et fondu enchaîné. Consultez le [contrat des modèles](docs/models.md) pour les tenseurs attendus.

## Compatibilité

**0.1.0-dev est expérimental.** La plateforme validée est Windows x64 sur CPU. DirectML est une option de compilation dont l’exécution GPU reste à valider. Les tests sur modèles synthétiques passent ; la qualité des voix réelles, les performances prolongées en temps réel et le fonctionnement GPU restent à vérifier. Voir la [validation](docs/validation.md).

Linux/macOS, CUDA, l’entraînement, le chargement direct de `.pth`, la recherche FAISS `.index` et la gestion des périphériques audio ne sont pas pris en charge. Aucun serveur HTTP, WebSocket ou gRPC n’est inclus. Les modèles doivent respecter les entrées et sorties documentées ; tous les exports RVC ne sont pas interchangeables.

## Démarrage rapide

Prérequis : CMake 3.24+, un compilateur C++17 et un SDK ONNX Runtime contenant `include/` et `lib/`. Exécutez les commandes dans un terminal développeur Visual Studio x64.

```sh
git clone https://github.com/VoiceLala/rvc-cpp.git
cd rvc-cpp
```

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DONNXRUNTIME_ROOT=C:/sdk/onnxruntime
cmake --build build
Copy-Item C:/sdk/onnxruntime/lib/onnxruntime.dll build/
ctest --test-dir build --output-on-failure
```

Cette compilation utilise le CPU. Les tests par défaut ne chargent pas de modèles de voix réelles. Pour les tests complets, DirectML et l’installation, voir le [guide de compilation](docs/build.md). Procurez-vous séparément trois modèles compatibles.

```powershell
./build/dvc_wav.exe voice.onnx content.onnx pitch.onnx 40000 768 input.wav output.wav
```

`40000` désigne la fréquence de sortie du modèle vocal, et `768` la dimension des caractéristiques de contenu. Ces valeurs doivent correspondre au modèle. L’exemple accepte du WAV mono PCM16 / float32 jusqu’à 30 secondes et écrit du PCM16 sans écraser les fichiers existants. Valeurs par défaut : CPU et locuteur 0.

## Intégration

Incluez `dvc/dvc.h`, initialisez les configurations avec les fonctions par défaut, créez un contexte et chargez les modèles. Appelez `dvc_convert` ou `dvc_process`, puis libérez le contexte. Les erreurs sont fournies par des codes de statut et `dvc_last_error`, local au thread. Sérialisez les appels sur une même instance. `dvc_process` exige exactement `config.block_size` échantillons par appel. L’inférence alloue de la mémoire et s’exécute de façon synchrone : utilisez un thread de travail plutôt que le rappel du périphérique audio. [API](docs/api.md).

## Licence et contributions

Licence [MIT](LICENSE). Les composants tiers conservent leurs licences ; voir les [mentions tierces](THIRD_PARTY_NOTICES.md). Les poids des modèles sont obtenus séparément et restent soumis à leurs licences respectives. Contributions : [CONTRIBUTING.md](CONTRIBUTING.md).

Merci à [RVC Project](https://github.com/RVC-Project/Retrieval-based-Voice-Conversion-WebUI) et à [ONNX Runtime](https://github.com/microsoft/onnxruntime).

## Découvrir VoiceLala

De nouvelles voix et des effets sonores pour vos jeux, streams et conversations. [Visiter VoiceLala →](https://voicelala.com/)

