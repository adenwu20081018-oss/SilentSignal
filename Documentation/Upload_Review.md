# Private upload: privacy and attribution review

This repository is for private review by its owner. Public release is not authorized. No open-source license has been assigned.

## Privacy preparation

The original local project and preview files remain unchanged. Upload copies have photo EXIF (including GPS fields) removed, BOM creator properties removed, personal user-directory names masked in CAD configuration paths, and the schematic file-path footer removed from the actual PDF content. Its PNG is rendered from the sanitized PDF. CAD edits are limited to equal-length personal path metadata; geometry and record lengths are unchanged, but reopening the sanitized CAD files in Altium has not been tested. Machine-specific configuration paths are not portable library dependencies.

Generated HTML, local preview scripts, caches, temporary files and build outputs are excluded. See upload_manifest.json for included-file hashes and sanitization.json for transformations. The manifest is generated before the initial commit.

## Attribution and redistribution

No LICENSE, COPYING or NOTICE file was supplied. The inspected OLED driver/font does not identify a license. Confirm provenance and redistribution rights for font data, CAD library/footprint content and 3D assets before any public release. User-supplied files are retained for the user's authorized private review; that does not establish a blanket redistribution license. No contributor identities or personal-contribution claims have been invented.

## Evidence limits

No demonstration video, audio dataset or quantitative recognition log was supplied. The repository includes available photographs and documentation, not fabricated media or results. Build/flash execution and physical tests remain unperformed during preparation. The detailed hardware and recognition limitations in the README still apply.
