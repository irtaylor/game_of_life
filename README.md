# Conway's Game of Life, Simple Simulator

A simple Game of Life Simulator. 
I wrote it from scratch, in the Handmade Hero Style for the purposes of practicing and learning. 

![Game of Life](gol.PNG)


## The game's workflow:
1. On startup, we open a blank canvas
2. Clicking on a pixel toggles the cell to live or dead
3. In EDIT mode, the user sets the starting state of the game
4. User clicks play to set the system in motion

## USER INPUT KEYS:
- P = play or pause
- R = reset to starting point and enter edit mode
- C = clear to blank canvas and enter edit mode


## Order of Development / TODO:
- Get a buffer for animation
- Get keyboard and mouse input
- Lock the framerate? I could be lazy and simulate one frame per second...
- File I/O, saving the game's starting state?
- fix fullscreen cursor position
- clean up the cursor icon for the game...
