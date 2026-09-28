# Atlas Reader

### Less searching. More understanding.

**An open-source book reader and research library in the making—for people who do more than turn pages.**

Your next idea might be buried in a thousand-page book. Or scattered across a hundred PDFs. Finding it again shouldn't mean starting over.

Atlas Reader is working toward a better way to study: bring your books together, read with focus, and build a path back to what matters. Local-first. Windows first. With Arabic, Urdu, and right-to-left reading at the heart of the plan.

[Explore the vision](PLAN.md) · [See our progress](CHECKPOINTS.md) · [For developers](docs/DEVELOPER_GUIDE.md)

<!-- atlas-status: N4|in-progress -->

## A library is more than a folder of files

It is the passage you need to quote. The chapter you want to revisit. The connection between two books that took you hours to find.

We are building Atlas around that kind of reading.

**Find the book. Keep the thought. Pick up where your research left off.**

## What we're building for you

The following describes our product direction, not features available today.

- **Your books, brought together.** A searchable PDF library across your folders, designed to handle moved books and disconnected drives without quietly forgetting them.
- **Reading with room to think.** A focused PDF workspace, responsive navigation, and search that helps you stay with the text.
- **Bookmarks with depth.** Organize a book into meaningful paths—not just a pile of saved pages—with nested outlines built for serious study.
- **Arabic and Urdu, considered from the start.** Joined letters, diacritics, mixed-language text, and right-to-left layouts belong in the foundation, not on a last-minute checklist.
- **Your work, kept close.** Local-first research, portable PDF information where safe, and clearly identified local storage when the original document cannot be changed safely.

> Portable when possible. Local when necessary. Never lost silently.

## Small steps. A serious foundation.

**Today: an accepted native Library beta foundation—not yet a daily-use reader.**

We have established the native Windows application shell and tested the PDF technology behind future reading and editing features. The approved stage includes Arabic/Urdu sample rendering, image-only PDFs, navigation, document-preservation tests, and repeated stability checks.

The project owner tested and approved **N3** on **27 September 2026**. Its native Library can add folders, index PDFs in the background, search English/Arabic/Urdu metadata, show Favorites and Recents, and protect a book's identity when files move or drives disappear. The accepted engineering version is **`2.0.0-beta.1`**. This is a verified development beta foundation, not a finished consumer release.

**What's not ready yet:** the built-in reading interface, bookmark editor, and annotations begin in later stages. There is no finished native consumer release to download today. Test packages are for engineering evaluation, and success on our test samples does not mean every PDF has been proven perfect.

[What N3 passed—and what it means](docs/baselines/N3_ACCEPTANCE.md) · [Detailed N3 evidence](docs/baselines/N3_LIBRARY_INDEX_EVIDENCE.md)

## The road ahead

**First, your library.** N3 is Accepted: finding, indexing, and organizing local PDFs now has a verified native foundation.

**Now, your reading workspace.** N4 is in progress: native PDF viewing, navigation, search and a focused, accessible reading canvas. It is not ready for owner testing yet.

**Next, your research tools.** Resilient bookmarks, followed by ink and annotations.

**More of your books, before 2.0.** EPUB, CBZ/CBR and user-owned, DRM-free AZW3, MOBI and PRC files are planned after the PDF foundation. KFX requires a feasibility decision. Today's N3 candidate indexes PDFs only. [See the format plan](docs/FORMAT_EXPANSION_ROADMAP.md).

**Before everyday use, the hard checks.** Further Windows integration, accessibility, performance, and release testing. Every stage has automated checks and hands-on acceptance requirements.

Windows is our first destination. Android, Linux, macOS, and iOS/iPadOS are longer-term plans—not available native editions. We have not announced a release date.

[Full roadmap](CHECKPOINTS.md) · [Planned Windows features](docs/FEATURE_SCOPE_2_0.md) · [Our quality commitments](docs/CHECKPOINT_QA_MATRIX.md)

## Follow the making of Atlas

If you want a PDF reader built around deep reading, organized research, and Arabic/Urdu needs, follow the journey.

**Star this repository to keep Atlas on your radar.** Share it with a reader or researcher who would care about this direction. For updates, use GitHub's Watch options or browse the [change history](CHANGELOG.md).

Want to look under the hood? Start with the [developer guide](docs/DEVELOPER_GUIDE.md) or the [full documentation](docs/README.md).

---

**About the older app:** Flutter development has stopped. That implementation is obsolete and retained only as a backup and migration reference on [`legacy/flutter`](https://github.com/AbubakarYasir/Atlas-Reader/tree/legacy/flutter). The current C++/Qt native project lives on `main`. These two branch names are permanent.


### Planned offline tools

Before stable 2.0, the roadmap includes full annotation controls, PDF text/image editing, page organization, form tools, secure redaction, equations and offline signatures. These are planned, not available in the current N3 preview. OCR, audio notebooks and online/collaboration services are excluded. See the [delivery stages and safety gates](docs/OFFLINE_EDITING_ROADMAP.md).
