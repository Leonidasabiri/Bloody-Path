
## Features MUST HAVE before the mgex:

### From the technichal side
 - [x] Particle system
 - [ ] Text rendering
 - [ ] Bloom post processing effect
 - [ ] Audio
 - [ ] Adjusted map format
 - [ ] Map editor
 - [ ] Managing the state of the game

### The gameplay loop
 The same as it was at the game jam, only more polished with better level design and visuals.

### Map editor and map format:

The current version suggests that all tiles sample from the same texture atlas, and bases on neighbors to automate the process,
, this does the job from a visual stand point, however, we will need more flexibility, meaning we need to specify some texture metadata
for each cell.


Example:

```
WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
 WWWWWW......................W
W......................WWWWWW
 WW....................WWW
W.....................WW
WWWWWWWWWWWWWWWWWWWWWWWWwwwww

[x][y] = "texture_path/texture.png"

```

So the map editor should be able to assign different texture data to each cell and save the file in this format.

Note: The parsing of the map should be adjusted, it expects the map to have a fixed width.

## The look of the game so far



<img width="551" height="381" alt="mm" src="https://github.com/user-attachments/assets/0747671c-f680-4953-8c05-141b256ff1ae" />

### Concept arts:

Example scene of how the game may look

![WhatsApp Image 2026-01-31 at 6 31 46 PM](https://github.com/user-attachments/assets/824ebe73-3b3a-43f5-92ec-4b7cc23cd586)


*insert more art here a mounir...*


