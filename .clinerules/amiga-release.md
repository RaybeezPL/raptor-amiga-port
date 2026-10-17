# Raptor Amiga release rule

When the task is to prepare an Amiga release (candidate, package, description,
manifest or checksums), read `docs/releasing-amiga.md` first and follow it.

- Package only with `tools/package_amiga_release.py`.
- Build only with the existing scripts through `tools/build_amiga_release.py`.
- Take documents from the exact source commit; do not copy them from the
  working tree.
- Use the icon templates in `packaging/amiga/icons/` byte-for-byte. Never
  reconstruct or edit the inactive `(CD32 options: ON | OFF)` / `(CD32=OFF)`
  ToolTypes.
- Verify the archive with an integrity test, an independent reader, the exact
  listing and a full control extraction before reporting success.
- A package without a tag is a candidate. Never create, move or delete a tag,
  commit, push, publish a GitHub release or upload to Aminet unless the user
  explicitly asks for that separate step.
- Never overwrite an existing release without an explicit `--force`; never
  delete unknown files in the output directory.
- Do not include game data, music, savegames, a personal `amiga.cfg`, logs or
  temporary files.
