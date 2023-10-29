# vrcsplat

Ever wonder what happens if you take the gaussian out of gaussian splats and render them in VRChat?  Well, wonder no more! This project takes gaussian splats and turns them into a bunch of paint swatches.

![Cover Image](https://github.com/cnlohr/slapsplat/blob/master/Assets/slapsplat/coverimage.jpg?raw=true)

## Setup
 * Requires AudioLink + Texel Video Player (from VCC)
 * Requires TinyCC to build the .exe https://github.com/cnlohr/tinycc-win64-installer/releases/tag/v0_0.9.27

## To use
 * open the `process` folder, then use command-line (cmd or powershell will both do)
 * place .ply files you want to use in there.

```
Usage: resplat [.ply file] [out, .ply file] [out, .asset image file] [out, .asset mesh file] [out, .asset image cardinal sort file]
WARNING: PLY file output is currently broken.
```

 * In Unity, use the SlapSplat shader on a new material.
 * Put that material on an object.
 * Use the Mesh .asset file as that object's mesh. NOTE: It is a point list (not even cloud) so you can't see it.
 * Use the image .asset file on that material's image in.
 * Use the cardinal sort file on the that material's cardinal sort.
 
 BOOM It should appear.
 
