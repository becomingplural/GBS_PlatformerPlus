# GBS_PlatformerPlus
Platformer+ Plugin for GBStudio
Version 1.6Beta

Plugins:
PlatformerPlus          - The Central Plugin, does most of the stuff, edits Platform.c and Platform.h
PlatformerCamera        - Adds camera follow and camera lead (as well as LERP for horizontal movement). Edits camera.c and camera.h -- NB. Will affect other gameplay modes!
PlatformerPlusGravity   - Changes actors so that their update loop can check for some (simplified) collisions. Potentially slows actors a little, so delete this if you're not using it. Changes actor.c and actor.h, so it will affect other gameplay modes.
OnlyPlatforms           - A simplified version of P+ that only adds platform actors and solid actors. You cannot have both P+ and OnlyPlatforms in the same project.
