# rectpack2D
Tiny, fast and efficient rectangle packing library allowing multiple, dynamic-sized bins

[This algorithm][1] should meet every gamedeving needs. 
It's very, very efficient, lightweight, and I've myself improved it with searching the best possible sorting function 
(whether it's by area, perimeter, width, height, max(width, height)) 
and the best possible bin size so **you don't have to hardcode the width/height yourself anymore**.

It's also easy to  design it so it automatically portions out your rectangles into more bins
if one with fixed maximum size is not sufficient, so you probably want to pass all your textures to it
and pass a maximum texture size as the value for maximum bins' dimension, and BAM ! 
You have your texture atlases ready to be uploaded to GPU. Same goes for font packing.

400 random rectangles, automatically divided into 3 bins of maximum 400x400 size:
![enter image description here][2]


  [1]: http://www.blackpawn.com/texts/lightmaps/default.html
  [2]: http://i.stack.imgur.com/mOgcn.png
