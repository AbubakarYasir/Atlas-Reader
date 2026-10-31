# Try the Atlas N4.1 review build

<!-- atlas-status: N4|in-progress -->

This review checks opening files, tabs and passwords. **You will see a file name and page count, not the PDF pages.** Showing pages is the next step after this review.

## Open the app

1. Extract the entire review ZIP into a new folder. Keep the files together.
2. Open `atlas_reader.exe`. You should get the Atlas window without a command window or a missing-DLL message.
3. Use the **Open PDF** button at the top, or press **Ctrl+Shift+O**.

The review ZIP includes a `samples` folder, so you do not need to use personal books. `A001_simple_text.pdf` has one page; `A002_geometry_labels.pdf` has three. `A008_password_rc4_40.pdf` is a synthetic protected test file; its public test password is **atlas-user**.

The app uses the normal local Atlas profile unless launched with `--profile-directory`. Review automation uses a separate disposable profile. Adding a sample folder to your Library creates Library records but does not move or modify the PDFs.

## Five short checks

1. **Open and tabs.** Open the one-page sample. Confirm it says one page. Open the same file again: there should still be one tab. Open the three-page sample, switch between them and close a tab with **Ctrl+W**. Use **Library** to go back.
2. **Password.** Open the protected sample. Enter a wrong password, then **atlas-user**. You should get a clear retry message first, then a ready message. Typed passwords should be hidden.
3. **Restart.** Enable **Restore reader tabs when Atlas starts**, leave two files open, close Atlas and open it again. The ordinary file should reopen; the protected file should ask for its password again. Turn restoration off and repeat: tabs should no longer return.
4. **Layout and keyboard.** Switch to Arabic and back. Check that names, tabs, buttons and the password box are readable. Try Tab, Shift+Tab, Enter, Ctrl+Tab and Ctrl+W. Review at your usual Windows scale and 200% if possible. The full checklist also includes Narrator.
5. **File safety.** After a sample reaches ready state, rename it in File Explorer. Atlas should not block the rename. Opening and closing files must not change their contents.

You can also add the `samples` folder with **Add folder**, then use a book's **Open** button to check Library → Reader.

## Tell us the result

Reply with the five check numbers and PASS or the problem you saw. Mention any skipped scale/Narrator checks explicitly; a skip is not a pass. Screenshots of problems are helpful. The complete [owner checklist](N4_1_OWNER_TEST.md) lists all required checks, including close-during-open and Narrator.

After all required checks pass, record **N4.1 PASS**. That approves only this opening/tabs step. The overall N4 reader stage and PR merge still require later work and approval.
