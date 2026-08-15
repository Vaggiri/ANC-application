# TWS Noise Canceller (ANC-application)

Android application prototype for real-time Bluetooth microphone capture and playback using Kotlin + C++ (JNI) with Oboe for low-latency audio streaming.

## Overview

This project demonstrates an **Active Noise Cancellation pipeline scaffold** for True Wireless Stereo (TWS) devices:

- Android UI to start/stop processing
- Runtime permission handling for audio and Bluetooth
- Bluetooth SCO routing for voice communication audio path
- Native audio engine using Oboe input/output streams
- DSP insertion point in native callback (currently placeholder attenuation)

> Current DSP behavior is a placeholder (`output = input * 0.9`) and not a full ANC/noise suppression algorithm yet.

## Tech Stack

- **Android:** Kotlin, AppCompat, ConstraintLayout
- **Native:** C++17, JNI
- **Audio:** Google Oboe (`com.google.oboe:oboe:1.8.0`)
- **Build:** Gradle Kotlin DSL, AGP 8.1.4, Kotlin 1.9.0
- **CI:** GitHub Actions (assembleDebug on pushes/PRs to `main`)

## Project Structure

```text
ANC-application/
├── app/
│   ├── build.gradle.kts
│   └── src/main/
│       ├── AndroidManifest.xml
│       ├── java/com/example/twsnoisecanceller/MainActivity.kt
│       ├── cpp/
│       │   ├── CMakeLists.txt
│       │   └── native-lib.cpp
│       └── res/
│           ├── layout/activity_main.xml
│           ├── values/{strings,themes}.xml
│           ├── xml/{backup_rules,data_extraction_rules}.xml
│           └── mipmap*/drawable* launcher assets
├── .github/workflows/android-build.yml
├── build.gradle.kts
├── settings.gradle.kts
└── gradle.properties
```

## How It Works

1. User taps toggle in `MainActivity`.
2. App checks runtime permissions and requests missing ones.
3. On start:
   - Sets `AudioManager.MODE_IN_COMMUNICATION`
   - Starts Bluetooth SCO and enables SCO route
   - Calls native `startDenoise()`
4. Native engine:
   - Opens low-latency **output** stream with callback
   - Opens matching **input** stream (`VoiceCommunication` preset)
   - In output callback, reads mic frames from input stream
   - Runs DSP placeholder and writes to output buffer
5. On stop/destroy:
   - Stops native streams
   - Disables SCO and restores `MODE_NORMAL`

## Permissions and Device Requirements

### Manifest features

- `android.hardware.microphone` (required)
- `android.hardware.bluetooth` (required)

### Runtime permissions

- `RECORD_AUDIO`
- `MODIFY_AUDIO_SETTINGS`
- Android 12+ (`SDK 31+`): `BLUETOOTH_CONNECT`
- Android 11 and below: `BLUETOOTH`, `BLUETOOTH_ADMIN`

### Minimum environment

- **minSdk:** 26
- **targetSdk / compileSdk:** 34
- Android device with Bluetooth + microphone
- Preferably a TWS/Bluetooth headset supporting SCO voice path

## Build and Run

### Prerequisites

- JDK 17
- Android SDK / Android Studio with NDK + CMake support
- Gradle (wrapper or local install)

### Build debug APK

```bash
gradle assembleDebug --no-daemon
```

APK output:

```text
app/build/outputs/apk/debug/app-debug.apk
```

### Run in Android Studio

1. Open repository in Android Studio.
2. Let Gradle sync.
3. Connect device/emulator (Bluetooth tests require real device).
4. Run `app` configuration.
5. Grant requested permissions, then toggle ANC on/off.

## CI

Workflow: `.github/workflows/android-build.yml`

- Triggers on push and pull request to `main`
- Sets up JDK 17 + Gradle 8.4
- Runs `gradle assembleDebug --no-daemon`
- Uploads `app-debug.apk` artifact

## Native DSP Integration Point

The key insertion point for ANC/noise suppression is in:

- `app/src/main/cpp/native-lib.cpp` inside `onAudioReady(...)`

Replace placeholder frame processing with:

- RNNoise integration
- Echo cancellation / adaptive filtering
- Additional gain control / filtering chain as needed

## Current Limitations

- No production ANC algorithm yet (placeholder pass-through attenuation)
- Uses `Thread.sleep(1000)` on start while waiting for SCO (blocking UI)
- No explicit SCO state broadcast handling
- No advanced error recovery for stream interruptions/disconnects
- No automated tests currently included

## Suggested Next Steps

- Replace placeholder DSP with real frame-based denoiser/ANC
- Move SCO startup flow to async state-driven handling
- Add robust Oboe error callbacks and stream restart strategy
- Add instrumentation tests for permission/routing lifecycle
- Improve UX/state feedback for Bluetooth routing and failures

## License

No license file is currently included in this repository.
