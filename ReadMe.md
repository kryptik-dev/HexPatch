# HexPatch - Xbox 360 Patching Tool For BadUpdateExploit

A comprehensive Xbox 360 homebrew application that applies essential patches to enable unsigned code execution and remove various security restrictions to the BadUpdateExploit.

![HexPatch UI](https://raw.githubusercontent.com/kryptik-dev/HexPatch/master/Media/Images/ui.png)

## ⚠️ Disclaimer

This software is for educational and research purposes only. Use at your own risk. The authors are not responsible for any damage to your console or any legal issues that may arise from its use.

## Features

- **Hypervisor Patching**: Applies critical hypervisor patches to disable security checks
- **Kernel Patching**: Patches kernel functions to allow unsigned code execution
- **XAM Patching**: Modifies XAM (Xbox Application Management) to remove restrictions
- **Console Information Display**: Shows CPU key, DVD key, and console type
- **Professional UI**: Clean interface with success/final video playback
- **Multi-language Support**: Supports multiple console languages
- **XeLL Integration**: Optional XeLL bootloader launching capability

## Requirements

- Xbox 360 console (any model)
- USB storage device

## Installation

1. Download the latest release from the [Releases](https://github.com/kryptik-dev/HexPatch/releases/tag/v1.1) page
2. Add the default.xex to the BadUpdatePayload Folder on your USB.
3. Run The Exploit

## Button Controls

- **Back Button**: Exit to dashboard
- **X Button**: Save console data to file
- **Y Button**: Dump 1BL ROM to file

## Technical Details

### Patches Applied

- **Hypervisor Patches**:
  - Memory protection bypasses
  - Expansion signature verification removal
  - XEX loading patches
  - XeKeys security patches

- **Kernel Patches**:
  - Signature verification bypasses
  - Media type verification removal
  - Device ID verification patches
  - Version check bypasses

- **XAM Patches**:
  - Insecure socket privilege grants
  - System link ping modifications

### Console Information Displayed

- **CPU Key**: 16-character hexadecimal key unique to your console
- **DVD Key**: 16-character hexadecimal key for DVD drive authentication
- **Console Type**: Motherboard model (e.g., Jasper, Trinity, Corona)

## Source code will be provided once refined



---
