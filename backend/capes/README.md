Drop cape images here to make them selectable in-game.

- Format: standard Minecraft cape texture, 64x32 PNG.
- Only the top-left ~22x17 region is used (the 1px top/bottom strips and the
  left/front/right/back panels of the cape itself). Anything in the bottom
  half of the image (the elytra layout) is ignored.
- The filename (without `.png`) is the cape's id, and is shown to players
  as-is in the cape picker — e.g. `creeper.png` shows up as "creeper".
- No restart needed: `GET /api/capes` re-scans this folder on every request.
