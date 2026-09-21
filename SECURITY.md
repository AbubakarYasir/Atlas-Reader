# Security Policy

## Supported versions

Atlas Reader Native is currently in pre-release development on the `2.0.0` line. Until Windows `2.0.0` is released, only the latest accepted/published native preview is considered supported for security fixes.

The existing Flutter Atlas Reader has its own release status and should not be assumed to share native security behavior.

## Reporting a vulnerability

Please **do not publish exploit details, private documents, passwords, or sensitive file samples in a public issue**.

Preferred reporting path:

1. Use GitHub **Private Vulnerability Reporting / Security Advisories** for this repository when available.
2. If that private channel is unavailable, open a public issue containing only a non-sensitive request for a private security contact/channel. Do not include exploit details until a private channel is established.

Include when safe:

- affected Atlas version/commit;
- Windows version/platform;
- high-level issue category;
- reproducible steps using a synthetic/redacted fixture if possible;
- security impact;
- whether the issue involves document parsing, filesystem paths, PDF mutation, import/backup, migration, or packaging.

## Security scope

High-priority reports include:

- code execution or memory-safety defects reachable from an untrusted PDF/import;
- path traversal or unsafe shell invocation;
- security/permission bypass;
- password/secret leakage;
- silent mutation/invalidation of protected or signed documents contrary to Atlas policy;
- destructive overwrite of externally changed documents;
- backup/import/migration corruption that can destroy existing user data;
- dependency/package compromise affecting distributed Atlas binaries.

See [`docs/SECURITY_MODEL.md`](docs/SECURITY_MODEL.md) for the engineering threat model.

## Research/testing guidance

Use copied, synthetic, or redistributable fixtures. Never test destructive behavior against irreplaceable user documents.

Atlas does not include or endorse password cracking, PDF restriction removal, or bypass of document security controls.

## Disclosure

We prefer coordinated disclosure after a fix or mitigation is available. Security fixes still require targeted regression/preservation testing before release, but critical issues may use an expedited release process.