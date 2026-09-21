# Atlas Reader Native — Security and Privacy Model

## Scope

Atlas opens complex, potentially malicious local document files and eventually writes back to them. Security is therefore a product requirement even though Atlas is local-first and does not operate a cloud service.

Windows 2.0 treats all PDFs, paths, imported Atlas bundles, legacy databases, metadata, and external-file changes as **untrusted input**.

## 1. Security principles

- least privilege;
- no administrator requirement for ordinary use;
- no core-runtime network dependency;
- never execute document content;
- never build shell commands by concatenating filenames;
- dependency parsers remain isolated behind Atlas interfaces;
- mutation is more privileged than reading and receives separate capability checks;
- preserve user data on failure;
- secrets/passwords remain ephemeral unless a future explicit secure-storage ADR changes policy;
- logs/diagnostics are privacy-minimized by default.

## 2. Threat surfaces

### PDF parsing/rendering

Risks include malformed object graphs, huge dimensions/counts, decompression bombs, parser bugs, unsupported encryption, hostile metadata/outlines, recursion/depth attacks, and third-party native memory-safety defects.

Mitigations:

- pin/update PDF engine revisions deliberately;
- fuzz/fixture-test Atlas-owned normalization/serialization boundaries;
- enforce practical limits on recursion/tree depth/resources where Atlas owns allocation;
- contain parser failures as typed document errors;
- never write a malformed/recovery-mode PDF unless a validated safe path exists;
- keep PDF engines replaceable so a security/quality failure does not require an app rewrite;
- monitor upstream security advisories before RC/release.

A sandboxed/helper-process PDF engine may be evaluated later if profiling/threat evidence justifies process isolation. It is not assumed in N0 because IPC/complexity must also be measured.

### Paths and filesystem

Treat filenames/paths as opaque data.

- no command interpolation;
- canonicalization/identity logic must not escape configured semantics;
- handle long/Unicode/RTL paths;
- reparse points/junctions cannot produce infinite traversal;
- avoid following unexpected links outside roots without explicit policy;
- permission-denied areas are isolated rather than retried aggressively;
- temporary/backup files use controlled sibling paths/names and safe replacement rules.

### Imported bookmark/backup JSON

Validate:

- schema/version;
- maximum practical file size;
- hierarchy depth/node count;
- string sizes where resource abuse is possible;
- page/destination ranges;
- malformed/duplicate IDs;
- unknown fields through forward-compatibility policy.

Import never executes links/commands merely because a string exists in an export.

### Legacy Flutter migration

- source database/profile is read-only;
- never execute SQL/content sourced from data as code;
- parameterized DB access;
- schema/version detection;
- malformed/unsupported legacy data produces unresolved/skipped records, not destructive guessing;
- migration into native profile is transactional/repeatable.

### External links

If PDF links can launch external URLs/files:

- distinguish internal page links from external actions;
- never execute embedded scripts;
- potentially dangerous launch/file actions require conservative handling;
- URL opening uses OS APIs with explicit user action;
- display/confirm unusual schemes rather than blindly invoking them.

JavaScript execution in PDFs is not a Windows 2.0 requirement.

## 3. Password/encryption handling

- document-open and permissions authorization are separate concepts;
- no password cracking/restriction bypass;
- passwords never enter SQLite, logs, exports, crash/diagnostic bundles, analytics, or filenames;
- pre-2.0 policy: successful passwords may live only in process memory for the current session;
- clear password buffers/objects as reasonably possible after use/session close, recognizing standard C++/Qt memory copies may limit perfect erasure guarantees;
- persistent credential storage requires a later ADR and platform secure-store design.

## 4. Signed/certified PDFs

File writability is not authority to modify signed data.

- detect signature/certification state where engine supports it;
- do not silently invalidate signed original integrity;
- default local-only/working-copy flow when mutation is unsafe;
- never claim signature validity after a mutation Atlas cannot cryptographically verify;
- preserve signature-related structures when operation policy says source must remain unchanged.

## 5. Safe mutation

Every write preflight checks:

1. expected document identity;
2. source revision has not changed externally;
3. current filesystem permissions;
4. PDF security permissions;
5. signed/certified policy;
6. destination/storage availability;
7. requested operation fits engine preservation guarantees.

Commit sequence:

**temporary write → validate → preserve/recover original → promote → reopen/re-index → mark committed**.

Failures never mark local state Embedded prematurely.

## 6. Native-code hardening

As toolchain/checkpoints mature:

- compile with high warnings;
- enable MSVC/Windows security defaults supplied by modern toolchain;
- ASLR/DEP/control-flow protections as provided/compatible by toolchain/Qt packaging;
- use RAII/standard containers/smart ownership rather than raw manual lifetime where practical;
- avoid unchecked integer conversions around PDF coordinates/sizes/counts;
- `std::span`/bounded views where useful;
- fuzz Atlas-owned parsers/serializers;
- run sanitizers on supported configurations;
- CodeQL C/C++ once source surface is meaningful.

Do not disable mitigations for micro-performance without evidence and explicit review.

## 7. Dependency security

Each production dependency has:

- pinned version/revision/baseline;
- upstream source;
- license inventory;
- security update/advisory monitoring path;
- rollback path;
- release SBOM entry.

No automatic dependency update is merged without tests. Critical security fixes may use an expedited checkpoint but still require targeted regression/preservation tests.

## 8. Network/privacy

Windows 2.0 core features work offline.

No silent:

- telemetry;
- document upload;
- usage analytics;
- remote indexing;
- cloud AI processing;
- third-party crash upload.

GitHub Actions/developer tools operate only in development/CI, not on user documents at runtime.

If update checking/crash reporting is added later, it requires explicit privacy/security design and user control.

## 9. Logs and diagnostics

Ordinary logs may include:

- Atlas version/build;
- typed error category;
- component/timing counters;
- sanitized technical diagnostics.

Ordinary logs should not include by default:

- PDF text/page content;
- bookmark descriptions/notes;
- passwords;
- full sensitive file paths when a safer identifier is sufficient;
- personal metadata beyond what is necessary for diagnosis.

A user-requested diagnostic bundle should show what will be exported and apply path/content redaction where practical.

## 10. Denial-of-service/resource limits

Atlas must fail gracefully on pathological input.

Potential bounded resources:

- maximum concurrent render jobs;
- cache sizes;
- import tree node/depth limits based on practical usability;
- thumbnail/cover dimensions;
- decompressed buffers controlled by Atlas;
- background task queues;
- log file size/rotation;
- SQLite transaction batch size.

Limits should be high enough for legitimate research PDFs (including 10,000+ bookmarks) and backed by stress tests.

## 11. Security test cases before RC

- malformed/corrupt PDF corpus does not crash whole app or trigger unsafe save;
- huge/deep bookmark import rejects/contains resource abuse;
- malicious/odd filenames are treated as data, not commands;
- external revision conflict prevents overwrite;
- password never appears in logs/DB/export;
- restricted PDF does not gain forbidden embedded changes;
- signed original remains unmodified in local-only flow;
- junction/reparse cycle cannot loop indefinitely;
- malformed backup/migration file cannot corrupt existing profile;
- interrupted mutation restores/leaves recoverable source.

## 12. Vulnerability reporting

Before public native beta distribution, add a repository `SECURITY.md` with the supported-version/reporting policy and a private reporting channel if GitHub private vulnerability reporting is enabled. Do not encourage users to publish exploitable file samples/secrets in public issues.

## 13. Security is not a feature checkpoint island

N2 PDF qualification, N3 filesystem/index, N5 mutation/bookmarks, N8 migration/backup, and N11 packaging each extend this model. Security defects can block any checkpoint even if its happy-path feature works.