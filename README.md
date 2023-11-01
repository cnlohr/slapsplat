# vrcsplat

Ever wonder what happens if you take the gaussian out of gaussian splats and render them in VRChat?  Well, wonder no more! This project takes gaussian splats and turns them into a bunch of paint swatches.

![Cover Image](https://github.com/cnlohr/slapsplat/blob/master/Assets/slapsplat/coverimage.jpg?raw=true)

[VRC Link](https://vrchat.com/home/launch?worldId=wrld_0ced80f6-29f7-4310-be6a-342eaf80c2ca&instanceId=github~region(us))

## Approach

We create 3 textures to render from:

1. The image .asset: Each splat is compressed down to a single rgba float pixel.  This pixel is interpreted with asuint() to extract the: x, y, z, scale x, scale y, scale z, color and rotation.
    1. Position (6 bytes) is stored as a `uint16_t` x 3, in arbitrary scale to preserve quality.
    2. Scale (3 bytes) is stored exponentially in `uint8_t`'s, as `exp( value / 32.0 - 7.0 )`
    3. Rotation (3 bytes) is stored as `int8_t` with x, y, z; q is synthesized.  As a quaternion.
    4. Color (4 bytes) is stored as `uint8_t`.
    5. The `z` component of scale and `a` component of color being unused.
 2. The Mesh .asset:  Only one vertex but as many indices as there are splats.  This is done to conserve download space.  It compresses extremely well.  Also by only having one vertex, there can be an unlimited number of points, that point at that vertex.  A geometry shader will convert these points to the proper splats at runtime.
 3. The cardinal sort .asset:  A 32-bit-per-pixel texture containing the order in which the splats should be rendered, from front to back for each of the 4 cardinal directions.  This optimizes the rendering to catch early z, which is crucial for performance because otherwise there would be far too much overdraw to be practical.

## Setup
 * Requires AudioLink + Texel Video Player (from VCC)
 * Requires TinyCC to build the .exe https://github.com/cnlohr/tinycc-win64-installer/releases/tag/v0_0.9.27

## To use
 * open the `process` folder, then use command-line (cmd or powershell will both do)
 * place .ply files from polycam, etc. you want to use in there.

```
Usage: resplat2 [.ply file] [out, .ply file] [out, .asset image file] [out, .asset mesh file] [out, .asset image cardinal sort file]
```

### Optionally edit your out ply file.

The `out, .ply file` is a regular PLY mesh.  You can edit it freely in blender for instance and rexport then use `geotoasset.exe` to create the image, mesh and sort files for the rest of the pipeline.

## Then, to use those 3 things...

 * In Unity, use the SlapSplat shader on a new material.
 * Put that material on an object.
 * Use the Mesh .asset file as that object's mesh. NOTE: It is a point list (not even cloud) so you can't see it.
 * Use the image .asset file on that material's image in.
 * Use the cardinal sort file on the that material's cardinal sort.
 * Apply a brush pattern.
 
 BOOM It should appear.

 ## Credits
  * Imaya for the backyard
  * Nono for the Michaels capture
  * Ako for the Porsche
  * Laura MB for the paint swatch
 
