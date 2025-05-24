# Qt File & Folder Compressor

This project is a custom file compression and decompression tool built using **Qt Core**. It features:

- Chunk-based compression using `qCompress()` and `qUncompress()`.
- Embeds metadata (like original filenames) into the compressed files.
- Skips files that are already compressed (like images, videos, and archives).
- Supports recursive compression/decompression of entire directories.
- Custom file header validation and size checks for safe decompression.
- Automatically removes the original file after successful compression (if smaller).

### 🔧 Features
- 📁 **Folder and file support**  
  Compresses individual files or entire folder hierarchies recursively.
  
- 🧠 **Smart compression check**  
  Skips already compressed media using MIME type detection.

- 🔄 **Symmetric decompression**  
  Rebuilds files from compressed chunks with original names and formats.

---

## 🚀 How to Build and Run

### Prerequisites
- Qt 6 (Qt Core module)
- C++17 or later
- `cmake` (or Qt Creator IDE)

### 🧱 Build Instructions

#### Option 1: Using Qt Creator (Recommended)
1. Open the `main.cpp` file in **Qt Creator**.
2. Configure the Kit (Desktop Qt 5 or 6).
3. Click **Build** and then **Run**.

#### Option 2: Using Terminal
```bash
qmake -project
qmake
make
./YourBinaryName

