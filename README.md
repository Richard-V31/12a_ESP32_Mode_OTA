# ESP32 – Mise à jour OTA (Over-The-Air)

Programme de base pour ESP32 permettant de **téléverser un nouveau programme par WiFi**, sans câble USB, grâce à la bibliothèque **ArduinoOTA**.
Il sert de squelette : il suffit d'y ajouter votre propre code (capteurs, moteurs, etc.).

---

## Fonctionnalités

- Connexion WiFi en mode station (l'ESP32 se connecte à votre routeur).
- Redémarrage automatique si le WiFi n'est pas connecté au bout de **20 secondes**.
- Nom réseau mDNS personnalisable (`<nom>.local`).
- Mises à jour OTA **protégées par mot de passe**.
- Suivi de la mise à jour sur le moniteur série : début, progression en %, fin et messages d'erreur détaillés.
- Identifiants (WiFi, mot de passe OTA) séparés du code dans `arduino_secrets.h`.

---

## Matériel et logiciels nécessaires

- Une carte **ESP32** (4 Mo de flash ou plus recommandé).
- **Arduino IDE** avec le support des cartes ESP32 (paquet *esp32* d'Espressif).
- Un réseau WiFi 2,4 GHz.
- Un ordinateur sur **le même réseau local** que l'ESP32.

Bibliothèques utilisées (toutes incluses dans le paquet ESP32, rien à installer) :

| Bibliothèque   | Rôle                                   |
|----------------|----------------------------------------|
| `WiFi.h`       | Gestion du WiFi                        |
| `ESPmDNS.h`    | Nom réseau `esp32-xxx.local`           |
| `WiFiUdp.h`    | Transport UDP utilisé par l'OTA        |
| `ArduinoOTA.h` | Réception des mises à jour par réseau  |

---

## Structure du projet

```
ESP32_OTA/
├── ESP32_OTA.ino        # Programme principal
├── arduino_secrets.h    # Identifiants (à créer, ne pas partager)
└── README.md
```

---

## Configuration

Créez un onglet / fichier **`arduino_secrets.h`** dans le dossier du croquis :

```cpp
#define SECRET_WIFI_SSID      "NomDeVotreWiFi"
#define SECRET_WIFI_PASSWORD  "MotDePasseWiFi"
#define SECRET_OTA_HOSTNAME   "esp32-monprojet"
#define SECRET_OTA_PASSWORD   "MotDePasseOTA"
```

| Constante              | Description                                              |
|------------------------|----------------------------------------------------------|
| `SECRET_WIFI_SSID`     | Nom du réseau WiFi                                       |
| `SECRET_WIFI_PASSWORD` | Mot de passe du WiFi                                     |
| `SECRET_OTA_HOSTNAME`  | Nom de la carte sur le réseau (apparaît dans l'IDE)      |
| `SECRET_OTA_PASSWORD`  | Mot de passe demandé par l'IDE à chaque mise à jour OTA  |

> ⚠️ Si le projet est publié (GitHub…), ajoutez `arduino_secrets.h` au fichier `.gitignore`.

---

## Schéma de partition

L'OTA nécessite **deux zones d'application** dans la mémoire flash (l'une exécute le programme pendant que l'autre reçoit la nouvelle version).

Dans **Outils > Partition Scheme**, choisissez par exemple :
- `Default 4MB with spiffs`
- `Minimal SPIFFS (1.9MB APP with OTA)`

Évitez les schémas « No OTA » ou « Huge APP ».

---

## Utilisation

### 1. Premier téléversement – par câble USB (obligatoire)
1. Branchez l'ESP32 en USB.
2. Sélectionnez la carte, le port série et le schéma de partition.
3. Téléversez le programme.
4. Ouvrez le moniteur série à **115200 bauds** : l'adresse IP et `OTA pret.` doivent s'afficher.

### 2. Téléversements suivants – par WiFi
1. Dans **Outils > Port**, choisissez le **port réseau** portant le nom défini dans `SECRET_OTA_HOSTNAME` (avec son adresse IP).
2. Téléversez normalement.
3. Saisissez le **mot de passe OTA** lorsque l'IDE le demande.
4. L'ESP32 redémarre automatiquement avec le nouveau programme.

> Chaque nouveau programme envoyé doit **conserver le code OTA**, sinon il faudra revenir au câble USB.

---

## Ajouter votre propre code

```cpp
void setup() {
  Serial.begin(115200);
  connexionWiFi();
  setupOTA();
  // ... votre initialisation
}

void loop() {
  ArduinoOTA.handle();   // OBLIGATOIRE
  // ... votre code
}
```

Règles importantes :
- `ArduinoOTA.handle()` doit être appelé **très souvent** dans `loop()`.
- **Pas de `delay()` long** : utilisez `millis()` pour temporiser, sinon les demandes OTA ne sont pas traitées.
- Si vous utilisez LittleFS/SPIFFS, fermez-le dans `onStart()` (`LittleFS.end();`) avant une mise à jour du système de fichiers.

Exemple de temporisation sans `delay()` :

```cpp
unsigned long dernier = 0;

void loop() {
  ArduinoOTA.handle();
  if (millis() - dernier >= 1000) {   // toutes les secondes
    dernier = millis();
    // action périodique
  }
}
```

---

## Messages du moniteur série

| Message                         | Signification                               |
|---------------------------------|---------------------------------------------|
| `Connecte ! IP : x.x.x.x`       | WiFi connecté                               |
| `Echec WiFi, redemarrage...`    | Pas de WiFi après 20 s, l'ESP32 redémarre   |
| `OTA pret.`                     | Prêt à recevoir une mise à jour             |
| `OTA : xx%`                     | Mise à jour en cours                        |
| `OTA : termine, redemarrage`    | Mise à jour réussie                         |
| `Mot de passe refuse`           | Mauvais mot de passe OTA                    |
| `Echec au demarrage`            | Pas assez de place / mauvais schéma de partition |
| `Echec de connexion`            | Problème réseau entre le PC et l'ESP32      |
| `Echec de reception`            | Transfert interrompu                        |
| `Echec de finalisation`         | Fichier reçu incomplet ou corrompu          |

---

## Dépannage

- **Le port réseau n'apparaît pas** : vérifiez que le PC et l'ESP32 sont sur le même réseau, redémarrez l'IDE, autorisez l'IDE dans le pare-feu (port UDP/TCP **3232**). Le réseau « invité » de certaines box isole les appareils.
- **`Echec au demarrage`** : le schéma de partition ne contient pas de zone OTA ou le programme est trop gros.
- **L'OTA fonctionne une fois puis plus jamais** : le nouveau programme ne contient pas `ArduinoOTA.handle()` ou utilise des `delay()` trop longs.
- **Redémarrages en boucle** : identifiants WiFi incorrects ou réseau 5 GHz uniquement (l'ESP32 ne gère que le 2,4 GHz).

---

## Paramètres techniques

| Paramètre          | Valeur      |
|--------------------|-------------|
| Vitesse série      | 115200 bauds |
| Port OTA           | 3232        |
| Délai max WiFi     | 20 s        |
| Mode WiFi          | Station (`WIFI_STA`) |
