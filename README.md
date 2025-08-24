### Win32TypingPracticeApp

  Vast majority of the code is in [BrightType1.1/BrightType1.1.cpp](https://github.com/brightgao1/Win32TypingPracticeApp/blob/main/BrightType1.1/BrightType1.1.cpp)

  I made this project to learn Win32, hence why I essentially implemented BS_AUTORADIOBUTTON instead of using it, as well as a trie for searching fonts. Ik some of the stuff I did was overkill, note that it was just for learning purposes.

  I programmed this in late 2024/early 2025. Unfortunately didn't have git at that time (and didn't use GH). 
  
  **I do NOT add new features or work on this project anymore.**
### Usage
  Just clone and open the Visual Studio solution (.sln) file, then compile. 
  Or u can just run the exe in x64/Release/BrightType1.1.exe, ensuring text folder (containing quotes.txt & fonts.txt) is in the same directory as exe.

  NOTE THAT GCC WILL NOT BE ABLE TO COMPILE THIS.

  If u wanna use MSVC via command line, u need to compile the .rc (using rc.exe) into .res (compiled resource file), then link them using link.exe or /link option in cl.exe. I advise u to not do this painful process tho.
  
### Features
  Customizable by clicking the "Settings / Preferences" top menu (nav bar). Choose from over 40 fonts, 3 themes, and ur desired font size & quote length.

  Buttons on the bottom right to go to a new text (Esc) or the previous text (F1). When finished typing a text and the statistics are displayed, you can retry the text just typed (r) or new text (Esc).

  In my app, you can go back to previous words while typing (accidental backspace at beginning of new word), and typing a space at the beginning of a new word will punish you, unlike popular typing sites like typeracer, monkeytype, etc...
  
  It's much more efficient than typing sites obviously, and runs smoothly on old computers.

  U can even remove quotes from text/quotes.txt and add whatever text u want for typing practice! Each quote must be on its own line though.
### Use However U Like
  Feel free to take any of the code for use or fork the project, idc. Just know that I'm not responsible for anything if somehow (I doubt it tho) anything here gets used for commercial purposes.
