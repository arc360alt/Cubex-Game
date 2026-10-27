Drop cape images here to make them selectable in-game.

- Format: a real Minecraft (Java Edition) cape file, 64x32 PNG. Any PNG
  colour format works (the official files are 8-bit palette images).
- The cape is the 22x17 region at the top left: the outside at (1,1), the
  inside at (12,1), 1px edges and top/bottom strips around them. The elytra
  (the shapes to the right of the cape) is ignored for now.
- The filename (without `.png`) is the cape's id. Any filename works,
  including spaces, e.g. `cape_Mojang_Office_9af2....png`.
- No restart needed: `GET /api/capes` re-scans this folder on every request.
