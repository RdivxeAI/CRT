# CRT PHYSICS EMULATOR

## Overview

**CRT PHYSICS EMULATOR** is a desktop application designed to visually and interactively simulate the physical behavior of Cathode Ray Tube (CRT) displays. The system models core CRT phenomena—including electron beam dynamics, phosphor response, and deflection systems—while enabling both raster-based image rendering and vector signal analysis.

This tool is developed as part of the **MechaML research initiative**, contributing to an ongoing study exploring CRT-based mechanisms for image generation and signal synthesis.

---

## Key Features

### 1. Electron Beam Simulation

* Real-time visualization of electron beam traversal across the screen
* Adjustable beam parameters:

  * Intensity
  * Velocity
  * Focus (spot size)
* Accurate scanline progression for raster rendering

### 2. Phosphor Response Modeling

* Simulates phosphor excitation and decay behavior
* Configurable persistence (afterglow) curves
* Supports different phosphor types (fast-decay, long-persistence)
* Visual representation of luminance fade over time

### 3. Deflection System (Horizontal & Vertical)

* Emulates electromagnetic deflection coils
* Adjustable sweep frequencies:

  * Horizontal sync (line scan)
  * Vertical sync (frame scan)
* Supports waveform-driven deflection inputs for experimentation

### 4. Raster Mode Rendering

* Displays images using traditional CRT raster scanning
* Line-by-line beam drawing with timing accuracy
* Optional scanline visibility and interlacing modes
* Image loading support (standard formats such as PNG, BMP, JPG)

### 5. Vector Signal Mode

* Direct beam steering based on vector input signals
* Suitable for oscilloscope-style rendering or vector graphics
* Exportable vector signal data for further processing or analysis

### 6. Telemetry & Diagnostics

* Real-time telemetry dashboard including:

  * Beam position (X, Y)
  * Signal voltage levels
  * Frame timing metrics
* Debug overlays for scanlines, beam path, and phosphor decay

### 7. Input & Data Handling

* Load external assets:

  * Raster images
  * Command/script files defining beam behavior
* Custom scripting interface for experimental control

### 8. Export Capabilities

* Export generated signals as:

  * Vector signal datasets
  * Timing sequences
* Useful for integration with hardware experiments or simulation pipelines

---

## Technical Architecture

### Core Modules

* **Beam Engine**: Computes electron trajectory and intensity
* **Deflection Engine**: Handles horizontal/vertical signal mapping
* **Phosphor Engine**: Models persistence and luminance decay
* **Renderer**: Converts physical simulation into visual output
* **Telemetry System**: Streams real-time simulation data

### Simulation Model

The emulator approximates CRT physics through:

* Time-based discrete simulation steps
* Signal-driven deflection equations
* Exponential decay functions for phosphor persistence

---

## Research Context (MechaML)

This emulator is part of a broader research effort by **MechaML** investigating alternative image generation paradigms using CRT-like mechanisms. The project explores:

* Encoding images as electron beam trajectories
* Leveraging phosphor persistence as a computational medium
* Generating images through analog signal synthesis rather than pixel buffers

The tool serves as both:

* A **visualization platform** for CRT physics
* A **prototype system** for experimenting with novel image generation techniques

---

## Use Cases

* **Educational**: Understanding CRT display physics interactively
* **Research**: Studying signal-based image generation systems
* **Signal Processing**: Designing and testing vector signal outputs
* **Creative Coding**: Generating visuals via beam control logic

---

## Future Work

* Generate images
* Enhanced phosphor material modeling
* Machine learning integration for beam path optimization
* Real-time signal synthesis from neural networks

---

## Conclusion

CRT Physics Emulator bridges historical display technology with modern computational research. By simulating the fundamental physics of CRT systems, it provides a platform for both education and innovation—particularly in exploring unconventional approaches to image generation rooted in analog signal behavior.
