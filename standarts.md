# Technical Test – C++ Developer  
## DICOM Image Viewer (Desktop)

---

## Overview

This technical test consists of designing and implementing a **desktop DICOM image viewer** using **C++ (C++17 or newer)**, with a focus on **correctness, code quality, clarity, and professional standards**.

The goal is to build a **simple, well-structured application** that allows loading and correctly visualizing a DICOM image through a graphical user interface.

---

## Required Background

The developer is expected to have strong experience in:

- Modern C++ (C++17 or newer)
- Qt-based desktop applications
- Image visualization
- Use of third-party libraries
- Clean, readable, and maintainable code

---

## Test Objective

Develop a C++ desktop application that allows the user to:

- Select a **DICOM (.dcm)** file from disk
- Load the selected file
- Correctly display the DICOM image in a **graphical user interface**

---

## Evaluation Focus

The solution will be evaluated based on:

- Correctness of the application
- Code quality and consistency
- Proper use of DICOM libraries
- Error handling
- Project organization and documentation

---

## Minimum Functional Scope (Required)

The application **MUST**:

1. Allow the user to select a DICOM file using a **file selection dialog (GUI)**
2. Load the selected DICOM file
3. Display the image correctly, respecting:
   - Image dimensions
   - Grayscale rendering (monochrome images)
   - Correct orientation
4. Provide a basic GUI containing:
   - An image display area
   - Clear error messages for invalid or unsupported files

---

## Technical Requirements

- **Language:** C++17 or newer  
- **Target OS:** Windows, Linux, or macOS  
- **Third-party libraries:** Allowed  
- **GUI framework:** Qt (recommended) or equivalent (must be justified)

---

## Optional Features (Nice to Have)

Implement **only if they do not compromise simplicity**:

- Window Level / Window Width
- Display of DICOM metadata
- Support for RGB images

---

## Coding Standards (Mandatory)

### Class Naming

- Use **PascalCase**
- All class names must start with the prefix **C**

### Member Variables

- All private member variables must start with **m_**
- Use camelCase

### Qt Signals and Slots

- Use camelCase
- Signals describe events
- Slots describe actions

### Methods

- Use camelCase
- Prefer verb-based names

### Constants and Enums

- Use `constexpr`
- Use `enum class`

---

## Deliverables

- Git repository with full source code
- README.md with build and run instructions

⚠️ **Do NOT list any AI system as the author.**

---

## Test DICOM File

https://drive.google.com/file/d/1hQXHPRN9xqAlUyUxl3HDvj_O7cEVERKc/view