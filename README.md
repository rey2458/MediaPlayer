# Skeuo MP3 Player
As you can see, I'm a huge fan of an early 2000s aesthetics so here's new 2000s-kinda-project. This is the mp3 player, I wanted to make it look like something you could find when you search for skins for windows media player or for winamp.

<img width="800" height="450" alt="mpgif" src="https://github.com/user-attachments/assets/0b400530-b5e3-466d-b5f7-71f65713eaef" />

[Watch the full video On Youtube](https://youtu.be/5rKzEV3sfvU)

## Tools
* C++
* Qt Framework

## Features
* Frameless Custom UI: non-standard window shape, I didn't take the lazy route of using static background images. So I made the design using the boundless power of the great code and for colors I played with contrast and gradient.
* Core Playback Controls: Fully mapped skeuomorphic buttons for Play/Pause, Next, Prev and an Eject button to load files.
* Expandable Playlist: A dynamic playlist view that displays your folder with your music files, it also works as queue for this playlist.
* Interactive Elements: Smooth volume control slider and a real-time track progress bar.

## How to run
Open the .pro file in Qt Creator, configure with your compiler kit and build the project. Once it built and started use the "Eject" button and find your folder with .mp3 files.
