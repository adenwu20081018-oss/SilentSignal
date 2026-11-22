# Publication-readiness review

This package is prepared for private GitHub review only. Public release is not authorized. See [upload review](Upload_Review.md) for privacy transformations and attribution concerns.

## Prepared

- [x] Separate working copy; original firmware, Altium and media files preserved.
- [x] Engineering README with specifications, architecture, calculations and supplied images.
- [x] Full acoustic and UI/output testing protocol; qualitative evidence separated from expectations.
- [x] Header-only CSV and standard-library analysis utility; no invented recognition results.
- [x] Build outputs, sdkconfig.old, editor configuration and pre-existing KiCad working files omitted.
- [x] Local HTML previews and local link checks prepared.

## Review before publication

- [ ] Confirm project authorship and contributors, including ownership/permission for C source, OLED driver/font data, Altium libraries and footprints, 3D models, BOM and photographs. No license file was present in the reviewed source tree. Do not assign an open-source license on assumption.
- [ ] Inspect embedded CAD metadata and media metadata for personal identifiers. The original schematic PNG/PDF contains a local username/path. That footer is removed from upload copies; original local files are unchanged. Review sanitized CAD dependencies before public release.
- [ ] Reconcile original Altium documents with pre-existing export screenshots; the schematic export identifies a KiCad working document. Import success is not electrical validation.
- [ ] Verify the actually deployed firmware version and document LED/motor wire rework visible in photographs.
- [ ] Resolve common-anode RGB schematic versus active-high firmware; inspect R2 motor-series resistor and actual motor ratings.
- [ ] Verify AMS1117 variant, loaded rail voltage, dropout, thermal behaviour and capacitor selection. Resolve M7 versus SS24 diode ambiguity and BOM capacitor-description discrepancies against fitted parts.
- [ ] Check USB/serial connector wiring, ground return link, boot/reset operation and board rule checks in Altium. No manufacturing-ready claim is made.
- [ ] Perform a clean ESP-IDF build; review the unusual compiler option in components/CMakeLists.txt. No fresh compilation or flashing was performed.
- [ ] Run the proposed tests, including background-only sessions, mode filtering, confirmation, physical vibration and motor feedback. Report denominators and conditions with any future metrics.
- [ ] Supply actual videos/audio if a demonstration or dataset is to be included; obtain recording and redistribution permission. No media of these types was found.
- [ ] Review all wording, image captions and attribution. Approve the final package explicitly before any publishing action.

## What is missing

Quantitative logs, labelled audio, demonstration videos, scenario-specific outcomes, measured latency/current/rail voltages, motor specification, deployed-revision identity, hardware rework notes, independent electrical validation and confirmed licensing. No distinct early-development photo sequence was identified; supplied photos are presented as assembled-device views.

## Provenance and limits

The BOM named bomfile.xls is an OOXML workbook internally; it is retained with its original filename, alongside a human-readable extraction. Files in this upload package are listed with SHA-256 hashes in upload_manifest.json; transformations are recorded in sanitization.json. The six device photographs and four original supplied 3D screenshots are renamed descriptively with EXIF metadata removed from JPEG upload copies while preserving compressed image data. The schematic file-path footer is removed in the upload PDF and its PNG is rendered from that sanitized PDF; circuit content is preserved. The source folder remains unchanged.

The closest practical local preview uses GitHub-like margins, typography, borders, tables and code blocks. Architecture uses a static SVG equivalent locally; GitHub will render the Mermaid block. GitHub chrome, syntax colours, image scaling and anchor behaviour may differ. The local preview is for review, not evidence that anything is hosted publicly.

Source-content search found no obvious credential assignments in firmware/source configuration; this is not a guarantee that opaque CAD metadata is free of private information. The visible schematic path was removed for private upload; unknown attribution and CAD dependency validation remain open. No third-party license or ownership conclusion can be inferred from file possession alone.
