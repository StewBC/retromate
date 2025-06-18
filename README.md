# RetroMate  
A Free Internet Chess Server front-end for modern and retro computers.  
  
## Conception  
The idea to build RetroMate came from Oliver Schmidt.  He asked if I would be interested in writing this, and if I was, he would provide a framework and incorporate IP65 so I could just get on with the game.  Without Oliver, this game would not have happened.  
  
## Requirements  
At this time, RetroMate for the Apple II requires 64K and an Uthernet II.  RetroMate uses IP65 for online communication.  RetroMate works very well in AppleWin.  
The Arari XL and C64 versions also use IP65 and require compatible hardware or emulators.  C64 should work with RRNet, which I have not managed to get to work on my laptop with wireless only.  The Atari version doesn't fit in memory so I have not gotten so far as to try and test the online component of the game.
The SDL2 version has been tested on Windows and Linux.  
  
## Development  
The Apple II, Atari XL and C64 versions are built using cc65.  It's almost all "C" code.  The hires drawing code was written by Oliver Schmidt, in 6502 assembly language, for cc66-Chess and re-used here.  
The Atari and C64 versions are more or less ports of the Apple 2 version, with modifications as required.
The SDL2 version is all "C" and has been compiled using clang, gnu c and Visual Studio 22.  FWIW, I found vcpkg on Windows and Linux to be the best way to install SDL2 so it works with CMake.  
  
## Using RetroMate  
The following describes how to use the interface.  To learn more about FICS and all that can be done (a lot more through the Telnet interface than what's exposed in the menu's) search for FICS help, or enter help in the RetroMate terminal, described later.  
  
## Pre-game, Offline, Menu  
Play Online will connect to freechess.org on port 5000 - the same way you would if you were using Telnet.  
If you have a FICS account, you can set your account and password in the settings.  You could also choose a different server/port.  
Quit will terminate the application.  On the Apple II, this will reboot the machine (ProDOS gets trashed by RetroMate).
  
## Online  
There are several game options to choose from, which are described below.  The default is a standard unrated game of about 15 minutes against any skill level opponent.  In my experience, most people on FICS play Blitz, and these games are around 3 to 5 minutes.
  
### Game Settings  
The first menu option I will cover is Game Settings.  
The game types standard, blitz, lightning and untimed are regular chess games, with different time limits.  
The game types crazyhouse, wild (with sub types wild 0-5, 8, 8a and fr) and suicide are variations on chess.  
Whichever type of game you choose here, is the type of game the New Game option will seek an opponent for you to play.  
  
When using a registered account there's an option, Rated, which defaults to No.  When yes, you seek to play games that affect your rating.  
   
Use Sought means look first for partners that are already looking for a game of the same type.  If you choose No to Use Sought, more options appear.  The game will use these options to look for a partner for you, when you choose New Game.  
  
The options when not using Sought are Start Time and Incremental Time. These times are how many minutes you have on the clock when you start and how many seconds are added to you clock after every move.  So, in reality, the Game Types standard, blitz, lightning and untimed are just variations of these values.  Untimed has a start and increment of 0.  RetroMate defaults to Standard (15, 0), Blitz (5, 2) and Lightning (2, 2).  The other games are defaulted to (3, 0).
Note that for some Wild variants (0 and 1), castling will have to be done in the terminal, as it requires a special command (o-o-o and o-o).  
  
The last two options Min and Max ratings.  Using these you can try and find players with a specific skill rating.  If you play in an unregistered account, you in essence have a 0 rating.  To be rated, you will have to register for an account.  
  
### New Game  
As discussed above, New Game will use the Game Settings to find a partner to play Chess against.  
  
### View Terminal  
This setting (keyboard shortcuts TAB or CTRL-T) switch to a telnet screen.  Here you can interact with the FICS server and do many more things that are not available from the UI.  Use the ESC, TAB or CTRL+T to switch back to the chess board view.  See later for some sample commands.  

### Hide Menu  
This option simply hides the menu (Keyboard shortcut ESC) so you can see the chess board.  
  
### Logout  
This will log out of the FICS server and will return you to the pre-game, offline, menu.  
  
## Using the chess board display  
If you are in a game, you have a cursor that you can move with the cursor keys, or WASD.  Enter will select a source piece, and Enter on another square will move the selected piece, if the move is legal, to the destination (2nd selected) square.  
You can press CTRL+S to bring up a prompt where you can enter a phrase of up to 50 characters to say to your opponent.  
ESC will hide/unhide the menu.  
TAB or CTRL+T will switch to the terminal.  
  
On the right hand side is a status display.   If you are in the game (one of the players), you will be listed at the top as black or white, and your opponent will be listed below you.  
The Next indicator is for which player is to move and the Last indicates the last move that was made (by the opposite player than the one listed in Next).  
  
Status messages, such as SAY or Checkmate or resign messages appear below the Last: message line.  
  
Even when it is not your turn, you are able to select a source piece to move.  This is useful for Blitz or Lightning games where you can prime your move while your opponent is making their move - every second counts!  
  
## The Terminal in more detail  
The Terminal is 80 columns wide.  If the computer display isn't capable of 80 columns, use CTRL-O (left) and CTRL-P (right) to "move" the 80 col display so all text can be viewed.  There is no scroll back, what's gone off-screen is gone.  
  
Some useful commands in the terminal  
* finger - see what you account name is, useful if you logged in as Guest (you get assigned a name such as GuestXXXX)  
* match <user> - Challenge a specific user to a match.  Useful for 2 people that want to play together.  Follow prompts (accept)  
* games - List all of the games currently being played  
* observe <game number> - Start watching a game in progress between two other opponents  
* unobserve [<game numer>] - Stop watching a game, or all observed games  
* resign - give up, will affect rating  
* abort - ask opponent if you can quit, and it won't affect your rating  
* help [<subject] - view the built-into FICS help pages  
* refresh - the server sends an update of whatever state you are currently in  
* say <text> - Tell your opponent something  
* sought - see who is looking to play games that you are eligible to join  
* seek [<parameters>] - advertise that you are looking to play a game (with optional parameters)  
* logout - go offline  
  
The RetroMate front-end is, as much as possible, stateless.  You can, for example, observe multiple games at the same time.  RetroMate will display each board as it gets an update.  Not practical, but it illustrates the point.  
  
## About the Apple II version  
Keep in mind that the Apple II is not quite in the same processing power class as modern machines, so you are at a bit of a disadvantage in Blitz/Lightning games since the text processing and updating the screen take considerable time.  
  
## Mouse support  
The SDL2 version supports a mouse in menus and the game board.  Left click is the same as ENTER and right-click is the same as ESC.  
  
## Known Bugs & Issues  
None of the 8-bit versions currently work.  
* Apple 2  - Fails with Error $56 at load time  
* Atari xl - Segment 'DATA' overflows memory area 'MAIN' - Need to figure out the shadow ram, I think  
* C64      - Runs but menu doesn't show up  
I have been unable to get RRNet working so there may be bigger issues, other than the reorg I made.
When I excluded IP65 from the Atari, I could see it working (rendering) and the C64 also was running earlier.
The SDL2 version still works well.

## Building the game
The build system is CMake based.  In the retromate folder, use the commands  
```
mkdir build
cd build
cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=/home/swessels/develop/github/external/vcpkg/scripts/buildsystems/vcpkg.cmake
make
```
You could also use `-G "Ninja"` if you use the `ninja` system.  
The targets are `atarixl`, `c64`, `apple2` and `sdl2` and each of these has a _test target - example `apple2_test` - that will attempt to run the game in (in an emulator if needed). In other words you can use `make c64_test` to build and run the c64 version in vice (x64sc or x64), if the emulator was found in the configure stage.  
  
On Windows, I have found `-G "NMake Makefiles"`, and then `nmake` (or `nmake <target>`) to work well.
  
I use VS Code with the CMake Tools extension, and this also works very well.  
  
These variables are checked to help find the tools needed to build (and run) the game, if the programs, named below, are not in the path:  
```
Path asnd what it searches for
VICE_HOME       - x64sc x64 & c1541
APPLEWIN_HOME   - applewin AppleWin.exe
CP2_HOME        - cp2
ATARI800_HOME   - atari800
DIR2ATR_HOME    - dir2atr
```
  
I develop using Linux through WSL or Windows directly.  Note that if you are using WSL, you can launch windows executables using the full name, including the .exe extension.  That way, it is, for example, possible to use AppleWin.exe, on Windows, to run the application developed on WSL.  
CP2 is CiderPress-II (cp2 is the command line tool for Apple 2 disk images) and is available here: https://github.com/fadden/CiderPress2  
dir2atr (Atari disk images tool) is available here: https://www.horus.com/~hias/atari/  (Windows exe in zip - `Atari Tools for Win32 (Windows)`; Otherwise source in `atariso-<date>.tar.gz`)  
c1541 (C64 disk images tool) is part of the Vice Emulator distribution.    


Initial Release  
6 June 2025  
swessels@email.com
