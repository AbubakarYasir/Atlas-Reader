# N5 Bookmarks + Document Safety: Owner Review Guide

<!-- atlas-status: N5|in-progress -->

This is an N5 **inspection build**, not an accepted release. Use it to inspect the
bookmark experience while the engineering closure ledger remains open. Automated
results cover the cases named in that ledger; they do not prove every hardware or
accessibility scenario.

> **Status:** N5 acceptance remains blocked until the [repair closure ledger](baselines/N1_N5_CLOSURE.md) passes. You can inspect this requested build now; a successful visual review does not silently close unverified engineering checks.

Accessibility review explicitly includes Windows Narrator: complete an edit, save and import workflow; verify control names, focus, selected/expanded state and error announcements. A skipped Narrator check is unverified, not PASS.

## Before you start

- Extract the whole review ZIP into a new folder and run `atlas_reader.exe`.
- **Use copies of your PDFs.** N5 is the first stage that writes into PDFs (only when you press Save); every save keeps a backup in Atlas's profile, but copies keep the review stress-free.
- Have ready: an Arabic or Urdu book with bookmarks, a long book (1,000+ pages), one PDF without bookmarks that has clear chapter headings, a password-protected PDF (`A008_password_rc4_40.pdf` in `samples`, password **atlas-user**), the synthetic signed sample `A018_self_signed.pdf` (its test signer is intentionally not trusted), or a signed/certified copy of your own, and Foxit (or another reader) to check results.
- Add the folder with your copies to the Library first: bookmarks are kept per book in the Library.

## Checks

1. **Edit bookmarks** (N5.3). Open a book, open the Bookmarks panel (left rail, F4). Add a bookmark at the current view (**Ctrl+B**), rename it (**F2**) with mixed Arabic and English, move it (**Alt+↑/↓**), nest and un-nest it (**Alt+→/←**), delete one, then **Ctrl+Z** / **Ctrl+Y**. Right-click shows the same commands. Nothing feels slow, and names stay exactly as typed.
2. **Find things** (N5.3). Type in *Find a bookmark*; press **Alt+Q** and find a command, a book and a bookmark from the title-bar search. On the long book the panel stays smooth.
3. **Auto Create Bookmarks** (N5.3). On the book without bookmarks: panel ⋯ → *Auto Create Bookmarks…*. The suggested headings and levels should be mostly right; untick a few, create them, then undo once (all of them go).
4. **What is saved where** (N5.2–N5.4). Bookmarks you add show as Atlas's own until saved. Close and reopen Atlas: everything is still there. Press **Ctrl+S**: the dialog says what will happen; after saving, open the same file in Foxit — the bookmarks are there, in order, with Arabic intact, and each goes to the right place.
5. **Protected files** (N5.1). Open the password sample and a signed/certified PDF. The status bar says what Atlas may do and why. The password book saves normally and still asks for its password afterwards. On the signed/certified one (or a read-only copy) you can still bookmark in Atlas, but Save says plainly it can't change this file and points to **Save a Copy** (**Ctrl+Shift+S**), which makes a new PDF with your bookmarks; the original's modified date is unchanged.
6. **Changed elsewhere** (N5.4). Save a book, close it in Atlas, change one of its bookmarks in Foxit and add one, save in Foxit, then reopen in Atlas. The panel tells you what changed and shows it; your own Atlas bookmarks are still there. If you rename the same bookmark in both, it is marked and the right-click menu lets you keep yours or use the PDF's.
7. **Export and import** (N5.5). Panel ⋯ → *Export Bookmarks…* as an Atlas bookmarks file, Markdown and CSV; open the Markdown in a text editor and the CSV in Excel (Arabic shows correctly, no formulas run). Then *Import Bookmarks…* the Atlas file into another copy of the book: try *at the end*, *skipping ones already here*, and undo.
8. **Languages and accessibility, including Narrator.** Repeat checks 1, 4 and 7 briefly in العربية and اردو (Ctrl+K): dialogs mirror, ordinary actions use understandable Arabic/Urdu, and Urdu is in Nastaliq. Familiar borrowed software terms and file-format names are intentional. Check joined letters and vowel marks, including بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ, and long mixed-language bookmark names. Do one edit-and-save with the keyboard only; try Windows 200% scale, high contrast and dark mode. With Narrator, check the selected bookmark, open/closed branches, focus and save/error announcements. Report skipped checks as SKIPPED.
9. **Workspace clarity.** Compare the Bookmarks panel, its menu, Ctrl+B, F2, the title-bar search and Save with Foxit side by side. Compare menu row spacing, widths and dialog sizes at the same display scale. Commands must fit, and a menu taller than the window must visibly scroll. Check that commands are easy to find and the document remains the main focus. Atlas uses its own licensed assets.

If an interrupted save asks for recovery, open **Review recovery**. **Keep current file** retains the PDF currently on disk; **Restore original** replaces it with the verified original backup. Both choices preserve copies of the two versions before resolving the hold. **Show backups** opens those files for inspection. Close and reopen the book afterwards to review bookmark conflicts. **Cancel** keeps saving blocked. If validation fails, the hold stays in place; do not delete the backups.

## Your reply

```text
N5 REVIEW · build commit: <from BUILD-INFO.json>
1 Edit bookmarks: PASS / FAIL / SKIPPED (why)
2 Find things: …
3 Auto Create: …
4 What is saved where: …
5 Protected files: …
6 Changed elsewhere: …
7 Export/import: …
8 Languages/accessibility: …
9 Comparison notes: …
Overall: N5 PASS / FAIL
Notes and screenshots:
```

A skipped check is not a pass, so please say why it was skipped. Problems get fixed, each with a test, and then only the failed checks are rechecked. Writing **N5 PASS** approves the bookmarks stage, its merge into `main` and the `2.0.0-beta.3` release.
