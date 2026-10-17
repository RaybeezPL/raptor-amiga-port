# Amiga icon templates

These six `.info` files are **byte-for-byte copies** of the icons shipped in
the verified `0.9.9-rc.5` Aminet archive (`releases/0.9.9-rc.5/raptor.lha`,
SHA-256 `204f6a1f39a76dd53bc014e766e4c2b14f1f671b2053275f5eb479607ed569f3`).
They are the packaging templates and must not be regenerated, re-saved or
edited per release.

The directory uses a neutral `game/` subdirectory for the files that go into
`Raptor/` inside the archive. This keeps `Raptor.info` and
`Raptor/raptor.info` distinct, and avoids the broad `raptor` pattern in
`.gitignore` on case-insensitive filesystems.

| Template | Archive path | SHA-256 |
| --- | --- | --- |
| `Raptor.info` | `Raptor.info` (drawer) | `f3d7a03864a452d0b38360e3a2ff05bc3f8b2129aad964c29f212ae6372cf587` |
| `game/ReadMe.info` | `Raptor/ReadMe.info` (project) | `dd3054fc65c593e91fcc0ba942b3c10e452a76b503429a8c23c6241eed9eed33` |
| `game/raptor.info` | `Raptor/raptor.info` | `88585dbfa1cdeec7aecce17361f9db6e0b5cb15e34d5493be11ba4d9f0891812` |
| `game/raptor_nofpu.info` | `Raptor/raptor_nofpu.info` | `611cde034135603fe4fe6beab5b7565aee992afd0921e6523a411da4aa74c011` |
| `game/raptor_030.info` | `Raptor/raptor_030.info` | `90aaede4e6b6dff237eff25f83e613df9aebe84970bd4d0ff51c89db642af132` |
| `game/raptor_030_fpu.info` | `Raptor/raptor_030_fpu.info` | `9ed3119d24c145aaf93e713bcec134801fa504cb608843b4cb0a915c7ddb8b4e` |

## Import verification

The import was verified once with **amigainfo 0.6.1** (PyPI wheel
`amigainfo-0.6.1-py3-none-any.whl`, run from `PYTHONPATH` without
installation; rendering dependency Pillow 13.0.0). The full structural parse
covered the DiskObject header, Gadget, icon type, StackSize, DefaultTool,
DrawerData, the classic / NewIcon / ColorIcon / ARGB / PNG layers, the
complete ToolTypes table and both rendered icon states (normal and selected).

The machine-readable result is `verification.json`. Packaging does not
re-implement an icon parser: it requires `verification.json`, verifies the
pinned SHA-256 values and checks the recorded ToolTypes. A custom
`--icons-dir` must contain an equivalent `verification.json` produced by the
same one-time import procedure.

## ToolTypes already present

The four program icons already contain the inactive CD32 entries exactly
once, in this order:

```
(CD32 options: ON | OFF)
(CD32=OFF)
```

`JOYSTICK=ON` is present in all four program icons. Do not reconstruct these
entries at release time and do not add an active `CD32=ON` / `CD32=OFF`
line; the game ignores parenthesized entries and reads `cd32` from
`amiga.cfg` (default `OFF`).

## What must not change

- Do not open and re-save the icons in an icon editor.
- Do not strip the GlowIcon data or the classic images.
- Do not add or remove ToolTypes when preparing a release.
- Any intentional icon change is a separate, reviewed change that requires a
  new import verification and updated SHA-256 values here, in
  `verification.json` and in the release manifest.
