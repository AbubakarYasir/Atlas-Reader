# N3 Owner Test — plain-language checklist

<!-- atlas-status: N3|ready-for-owner-test -->

Use only the exact N3 Windows ZIP and SHA-256 supplied in the final handoff. Test copied books, not your only copy. N3 is not passed until every required item below has a clear result.

## Prepare one safe test library

Make two or more test folders containing:

- ordinary English PDFs;
- Arabic and Urdu PDFs with long/mixed filenames;
- one image-only PDF;
- one password-locked PDF;
- one damaged/corrupt PDF copy;
- nested folders;
- a removable drive or another folder you can temporarily disconnect/rename;
- one PDF you can safely rename, move, copy and replace.

## Test in this order

1. **Start clean.** Unzip the package, run `atlas_reader.exe`, and confirm the Library opens without a missing-DLL message.
2. **Add folders.** Add both test folders. The window must stay usable while scanning; books should appear without the app freezing.
3. **Find books.** Search an exact word from an English title, an Arabic title and an Urdu title/filename. Check All books and each folder view. N3 matches stored words/phrases; it does not promise Arabic stemming.
4. **Use normal views.** Mark a book as Favorite, open an available book, then confirm it appears under Favorites and Recently opened. List and Grid must both remain readable.
5. **Rename or move one book on the same drive.** Press `F5`, open Review changes, confirm the shown old/new paths are correct, then apply the move. The item should keep its Favorite/Recent identity.
6. **Copy a book while keeping the original.** Press `F5`. Atlas must keep the copy separate; it must not silently move or merge the original.
7. **Replace a file at the same path.** Press `F5`. Atlas must show uncertainty/review rather than silently give the new file the old book's identity.
8. **Disconnect a folder/drive.** Close Atlas, disconnect or temporarily rename the removable test root, reopen Atlas and press `F5`. The folder must say offline and its known books must remain in the Library. Reconnect it, press `F5`, and confirm recovery.
9. **Restart.** Close and reopen Atlas. Folders, Favorites, Recents and any accepted move must still be present.
10. **Check keyboard and scale.** Use `Ctrl+K`, `Ctrl+O`, `F5`, `Tab`, `Shift+Tab`, `Enter` and `Esc`. Repeat the main Library/search flow at Windows 200% scale. Focus must be visible and important text/actions must not be clipped.
11. **Check Arabic mode.** Switch to Arabic. Navigation must mirror sensibly; Arabic/Urdu text must remain joined/readable; English filenames and numbers must not reverse incorrectly.
12. **Compare one same task.** In Zotero, Calibre, Adobe Acrobat or another mature library/search product, use the same test folder and search task. Record product name/version/date, what felt better there, and what Atlas should improve. This is learning evidence, not a “best” claim.

## Send this report

```text
N3 result: PASS or FAIL
Package SHA-256: ...
Windows version and display scale: ...
Storage tested: internal SSD / removable drive / other ...

1 Start clean: PASS/FAIL — note
2 Add folders/responsiveness: PASS/FAIL — note
3 English/Arabic/Urdu search: PASS/FAIL — note
4 Favorites/Recents/List/Grid: PASS/FAIL — note
5 Rename/move and keep identity: PASS/FAIL — note
6 Copy stays separate: PASS/FAIL — note
7 Same-path replacement is not guessed: PASS/FAIL — note
8 Offline/reconnect keeps books: PASS/FAIL — note
9 Restart keeps state: PASS/FAIL — note
10 Keyboard and 200%: PASS/FAIL — note
11 Arabic/RTL: PASS/FAIL — note
12 Comparison product/version/date and lesson: ...

Final words: N3 PASS
```

If any required item fails, report `N3 FAIL` with what you saw. Do not work around a failure by deleting the profile or re-adding everything; recovery behavior is part of the test.
