# SoundScan

Integrated Qt application for high-speed ultrasonic inspection of laminated
structures. It combines NDT acquisition and display (Phaselink), 3D point-cloud
rendering (3DScan), robot/PLC motion control (TwinCAT ADS and KUKA RSI over UDP)
and DELMIA-driven path simulation in a single operator UI.

## Overview

- **NDT acquisition and display** — the Phaselink sources (UT/A-scan widgets,
  gates, ACG/TCG, phased-array and scanning views, dialog set) are compiled into
  this application from the sibling `Phaselink/` folder, and the vendor client
  SDK is linked from `C:\Phaselink`.
- **3D imaging** — the `3DScan/` sources (scan pipeline, data panel, scan data
  interface, VTK VBO actor) provide the live point cloud. `algorithm.cu` is
  compiled before the link step by an `nvcc` pre-build target declared in
  `SoundScan.pro`, and the cloud is drawn through VTK 9.6.
- **Motion control** — `ads_client`, `ads_poller` and `ads_read_thread` talk to
  the Beckhoff/TwinCAT PLC over the ADS API (`TcAdsDll`). `RoboMotionInfo`
  carries the Cartesian pose and the gantry position.
- **Robot interface** — `udpserver` receives the KUKA RSI XML stream (position
  `RIst`, joint angles `AIPos` and the `IPOC` counter) on UDP port 59153 and
  re-emits it as Qt signals; `tcpserver` accepts TCP clients and exchanges XML
  command/pose messages with them.
- **Path simulation** — `delmiaworker` drives DELMIA through ActiveX
  (`QAxObject`) to create/open/save documents and run simulation commands, so
  the scan path can be prepared in the simulation environment.
- **UI** — `mainwindow1` is the main operator window that embeds the NDT and 3D
  widgets, `mainwindow2` is a secondary window, and `colormanager` /
  `debugoutput` provide theming and the in-application debug output.
- **Start-up behaviour** — `main.cpp` sets per-monitor DPI awareness and derives
  a `QT_SCALE_FACTOR` from the 2560×1440 design resolution, disables VSync and
  multisampling for the point cloud, suppresses VTK popup windows and shows a
  splash screen.
- **Offline harness** — `tools/scan_harness.cpp` is a small console program that
  loads a scan CSV through `Scan::loadCSVAndBuild()` and dumps grid index, pass
  index and world coordinates, which makes it possible to validate the scan
  pipeline without the GUI.

## Repository layout

| Path | Purpose |
| --- | --- |
| `main.cpp` | Entry point: DPI scaling, surface format, splash screen, translation |
| `mainwindow1.{h,cpp,ui}` | Main operator window (embeds the NDT and 3D views) |
| `mainwindow2.{h,cpp,ui}` | Secondary window |
| `ads_client.{h,cpp}`, `ads_poller.{h,cpp}`, `ads_read_thread.{h,cpp}` | TwinCAT ADS link to the PLC |
| `udpserver.{h,cpp}` | KUKA RSI / AIPos / IPOC receiver (UDP) |
| `tcpserver.{h,cpp}` | TCP server for XML command/pose exchange |
| `delmiaworker.{h,cpp}`, `DelmiaWorker.h` | DELMIA automation through ActiveX |
| `colormanager.{h,cpp}`, `debugoutput.{h,cpp}` | Theme handling and in-app debug output |
| `SoundScan.pro` | qmake project: links TwinCAT, Phaselink, VTK and CUDA, and compiles the sub-project sources |
| `tools/scan_harness.cpp` | Console harness for offline CSV validation |
| `.vscode/scripts/build_*.bat` | Build scripts for SoundScan, 3DScan and Phaselink |
| `images/`, `resources.qrc` | Application icons/images |

## Sub-project sources

`SoundScan.pro` compiles sources that live outside this repository, from two
sibling folders:

- `3DScan/` — `mainwindow3`, `datapanel`, `scandata`, `scan`, `vtkvboactor` and
  `algorithm.cu` (separate repository: `DrestryRobot/3DScan`)
- `Phaselink/` — the NDT widgets, dialogs, `app`, `datadispatch`, `ndtbase` and
  the `.ui` forms (separate repository: `DrestryRobot/Phaselink`)

Both folders must be checked out inside the SoundScan project directory before
building.

## Requirements

- Windows with MSVC 2022 and Qt 6.8.3 (`msvc2022_64`) plus `jom`
- TwinCAT ADS: `C:\TwinCAT\AdsApi\TcAdsDll\Include` and `...\x64\lib`
  (`TcAdsDll`)
- Phaselink client SDK: `C:\Phaselink\include` and `C:\Phaselink\lib`
  (`client`)
- VTK 9.6 at `C:\Program Files (x86)\VTK` (Rendering, Interaction, IO,
  Imaging and Qt support modules)
- CUDA 13.3 at `C:\CUDA\v13.3` (compiled for `sm_86`)
- Python 3.13, linked as `python313` — the include/lib paths in
  `SoundScan.pro` point at a per-user Python installation and must be adjusted

## Build

The `.vscode/scripts` folder contains `build_soundscan.bat`,
`build_3dscan.bat` and `build_phaselink.bat`. Each script initializes the MSVC
environment, runs qmake (Qt 6.8.3, `win32-msvc`, release) into `build/release`
and then builds with `jom`.

The equivalent manual steps are:

```
qmake SoundScan.pro -spec win32-msvc CONFIG+=release "INCLUDEPATH+=C:/SoundScan/Phaselink"
jom
```

`CONFIG += force_debug_info` is set, so release builds still produce PDB
symbols.

## Notes

- SDK, VTK, CUDA and Python paths are absolute and Windows-specific; they need
  to be edited for another machine.
- `build/release/release/default_config.json` is a device configuration
  snapshot (gates, focal laws, voltages, probe geometry) kept in the repository.
- `mainwindow1.{cpp,ui}` is by far the largest part of the project and contains
  most of the operator workflow.

## License

No license file is present in this repository. Contact the repository owner for
licensing terms. The Phaselink, TwinCAT and VTK SDKs are covered by their own
vendor licences.
