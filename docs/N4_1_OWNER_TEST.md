# N4.1 Owner Review — Session and Open Model

<!-- atlas-status: N4|in-progress -->

Use this checklist only for N4.1. Page rendering is intentionally absent until N4.2; a ready PDF currently shows the honest status surface rather than document pages.

## Build under review

Test the exact `native-v2-n4-reader-foundation` head named in `docs/baselines/N4_READER_FOUNDATION_EVIDENCE.md`. Do not review a stale `main` or `2.0.0-beta.1` release build.

## Before starting

- Use disposable or backed-up PDFs only.
- Have one ordinary PDF, one password-protected PDF, and one reasonably large PDF available.
- Keep at least one PDF in an Atlas Library root so both Library Open and direct Open can be checked.
- No PDF should be modified by N4.1.

## Review

1. **Library → Reader**
   - Open an available PDF from the Library.
   - Confirm Atlas opens an in-app Reader tab rather than the Windows default PDF application.
   - Confirm the tab reaches a clear ready state with a page count.
   - Confirm Atlas does not claim page rendering yet.

2. **Direct Open and duplicate policy**
   - Press `Ctrl+Shift+O` and open a local PDF.
   - Open the same path again.
   - Confirm Atlas activates the existing tab rather than creating a duplicate tab.

3. **Tabs and close lifecycle**
   - Open at least three PDFs.
   - Switch between tabs, close the middle tab, then close the active tab with `Ctrl+W`.
   - Confirm focus/selection moves predictably and the Library remains reachable.
   - With a large PDF, close its tab while it is still opening. The tab must disappear cleanly and must not later reappear or change another tab.

4. **Password flow**
   - Open a password-protected PDF.
   - Confirm a password field is shown instead of a generic failure.
   - Enter a wrong password once. Confirm Atlas says it was not accepted and allows another attempt.
   - Enter the correct password. Confirm the tab reaches ready state.
   - Close/reopen Atlas later and confirm the password is requested again; Atlas must not remember it.

5. **Optional local restoration**
   - Leave restoration disabled, close Atlas, and reopen it. Confirm previous Reader tabs are not restored.
   - Enable **Restore reader tabs when Atlas starts**, leave an ordinary PDF and a protected PDF open, close Atlas, and reopen it.
   - Confirm the ordinary PDF reopens and the protected PDF returns to password-required state.
   - Disable restoration again and confirm it no longer restores the tabs on the next start.

6. **File sharing and release**
   - After an ordinary PDF reaches ready state, rename or move that PDF in File Explorer. Atlas must not keep an unnecessary read handle that blocks the operation.
   - Open a large PDF and close its tab while opening, then exit Atlas. Rename/move the PDF after Atlas exits. It must not remain locked by an orphan worker.

7. **English / Arabic / RTL**
   - Review the Library and N4.1 Reader surface in English and Arabic.
   - In Arabic, confirm the Reader/header controls, tab strip, password state, restoration option and messages follow RTL layout naturally.
   - Confirm mixed Latin filenames remain readable rather than visually scrambled.

8. **Keyboard, scale and accessibility**
   - At 100% and 200% Windows scale, confirm the N4.1 controls remain visible and usable without clipped password fields/buttons or inaccessible tab controls.
   - Complete the main flow with keyboard only: Library open, direct open shortcut, tab switching/focus traversal, password field/button, Library return and `Ctrl+W`.
   - With Narrator, confirm the Open PDF, Library/Return, tab-close, password field, Unlock PDF and restoration controls have understandable names. The password itself must never be spoken as ordinary visible text by Atlas.

9. **No-write check**
   - Confirm the tested PDFs' modified timestamps/content have not changed merely by opening, closing, restoring or supplying a password.

## Report

Reply with:

```text
N4.1 OWNER REVIEW
Head: <commit>

Library → Reader: PASS / FAIL
Direct open + duplicate handling: PASS / FAIL
Tabs + close-during-open: PASS / FAIL
Password flow: PASS / FAIL
Restoration + password non-persistence: PASS / FAIL
File rename/move after ready and shutdown: PASS / FAIL
English/Arabic/RTL: PASS / FAIL
100%/200% layout: PASS / FAIL
Keyboard: PASS / FAIL
Narrator/accessibility names: PASS / FAIL
PDFs remain unchanged: PASS / FAIL

Overall N4.1 result: PASS / FAIL
Notes:
```

`N4.1 PASS` closes only the session/open subgate. It does not accept N4 as a whole and does not authorize merging PR #6. N4.2 may begin only after any N4.1 defects found here are fixed and retested.
