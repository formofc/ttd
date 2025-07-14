# Text-to-Delirium (TTD)  

A minimalistic text-to-speech (TTS) engine that replaces words with pre-recorded audio clips.  

### **How It Works**  
1. You provide a **config file** with word mappings:  
   - `"word" "audio_file" start_time duration`  
   - Example: `"hello" "sounds/greeting.wav" 0.5 1.2`  
2. The program reads **stdin**, matches words, and either:  
   - **Plays** the corresponding audio clips in real-time.  
   - **Generates** a `.wav` file if `-o output.wav` is specified.  

### **Why?**  
- Because sometimes you just need Vladimir Zhirinovsky to read your emails.  
- Because real TTS engines are **bloated**.  
- Funny

### **Usage**  
```bash
./ttd config.txt < input.txt           # Play audio live  
./ttd config.txt -o output.wav < input.txt  # Save to file  
```  

### **Dependencies**  
- [`miniaudio.h`](https://miniaud.io/)
- A C compiler (even ansi)

### **Build**
```
gcc src/main.c src/miniaudio.c src/sleep.c src/arena_alloc.c -I includes -O3 -ansi -Wall -Wextra -o ttd
```

### **Limitations**  
- Ouput audio files are **mono, 44.1kHz**.

### **Some important quirks**  

Since **TTD operates on raw bytes** (not "text" in the human sense), it does not understand whitespace, punctuation, or capitalization. This means:  

- If your input has `" hello"`, but the config only maps `"hello"` (no space), then `" hello"` will not trigger the sound.  
- **Punctuation marks** (`.,!?`) are just bytes—map them explicitly if you want them spoken.  
- **Capitalization matters**: `"Hello"` ≠ `"hello"` unless you define both.  

To handle spaces, add a silent or spacer sound:  
```
" " "silence.wav" 0.0 0.1  
"," "comma.wav" 0.0 0.3  
"." "fullstop.wav" 0.0 0.4  
```  
