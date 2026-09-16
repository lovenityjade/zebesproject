# Application icons

The packaged startup BMP/ICO files use the project-owned creator logo.
`provenance.json` binds the source and generated files by SHA-256.
Regenerate using `python3 Scripts/build-app-icon.py` (Pillow required).

After a valid player ROM is loaded, the game decodes the original 16 × 16
save-menu Samus helmet from that ROM and sets it as the running window icon.
No Nintendo helmet pixels are shipped in the executable or icon resources.
The original game artwork belongs to Nintendo.
