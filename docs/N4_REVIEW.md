# N4 Reader: Owner Review Guide

<!-- atlas-status: N4|in-progress -->

One short review at the end of N4 (about 45 minutes). Agents run the detailed tests first, so this review is about what only you can judge: is reading comfortable, clear, correct in Arabic, and trustworthy?

> **Status:** N4 is still being built. Opening, tabs and passwords (N4.1) work today. Showing pages arrives in N4.2. A build made before N4.5 shows only some of the checks below; each check says which part it needs.

## Before you start

- Extract the whole review ZIP into a new folder and run `atlas_reader.exe`. It should open with no command window and no missing-DLL message.
- Use the `samples` folder or **copies** of your own PDFs. Atlas must not change any file during N4.
- `A008_password_rc4_40.pdf` is a synthetic protected file; its public test password is **atlas-user**.
- Have ready: one scanned (image-only) book, one Arabic/Urdu book, one long book (1,000+ pages), one with rotated or mixed-size pages, and one with many links.

## Checks

1. **Opening and tabs** (N4.1). Open a book from the Library and one with **Ctrl+Shift+O**. Opening the same file twice should not create a second tab. Switch tabs with **Ctrl+Tab** and close with **Ctrl+W**. Close a large book while it is still opening; nothing should reappear later.
2. **Password** (N4.1). Open the protected sample, enter a wrong password, then the right one. The message should be clear and the text hidden. After restarting Atlas it must ask again; Atlas never remembers passwords.
3. **Restore tabs** (N4.1). Turn on *Restore reader tabs when Atlas starts*, close and reopen Atlas, then turn it off and check again.
4. **File safety** (N4.1). While a book is open, rename it in File Explorer; Atlas must not block it. At the end, your files' modified dates are unchanged.
5. **Reading feel** (N4.2–N4.3). Scroll the long book quickly, jump to page 900, zoom in and out, and switch fit width/fit page. Pages appear quickly, scrolling is smooth, and nothing freezes.
6. **Pages, labels and layout** (N4.3). Go to a page by its printed label (for example "xii" or "45"); try facing pages and cover page; check rotated and mixed-size pages; set an Arabic book to right-to-left page order.
7. **Find your way** (N4.4). Use the outline and thumbnails, search for an Arabic word and an English word, follow a link, then use **Back** to return exactly where you were.
8. **Honesty** (N4.4). The scanned book reads normally and says plainly that it has no searchable text. Clicking a web link asks before opening the browser.
9. **Arabic, keyboard and accessibility** (all). Switch the app to Arabic: layout mirrors and names stay readable. Do one full task with the keyboard only. With Narrator on, buttons, tabs and the password field should have sensible names. Check at 200% Windows scale and in dark mode.
10. **Compared with what you use now** (N4.5). Do checks 5–7 in Foxit or SumatraPDF on the same files. Is Atlas as comfortable? Note anything that felt better or worse.

## Your reply

```text
N4 REVIEW · build commit: <from BUILD-INFO.json>
1 Opening/tabs: PASS / FAIL / SKIPPED (why)
2 Password: …
3 Restore: …
4 File safety: …
5 Reading feel: …
6 Pages/labels/layout: …
7 Find your way: …
8 Honesty: …
9 Arabic/keyboard/accessibility: …
10 Comparison notes: …
Overall: N4 PASS / FAIL
Notes and screenshots:
```

A skipped check is not a pass, so please say why it was skipped. Problems get fixed, each with a test, and then only the failed checks are rechecked. Writing **N4 PASS** approves the reader stage, its merge into `main` and the `2.0.0-beta.2` release.
