# A010 Certification Structure Fixture

A010 is a deterministic structural safety fixture for Atlas N2. It contains a PDF AcroForm signature field, a `/Type /Sig` dictionary, catalog `/Perms /DocMDP`, and a DocMDP transform with permission level `/P 2`.

It is **not cryptographically valid**: `/Contents` contains synthetic bytes and `/ByteRange` is a sentinel `[0 0 0 0]`. It must never be cited as evidence that qpdf, pypdf, Qt PDF, PDFium, or Atlas can cryptographically verify signatures.

Its purpose is narrower and safety-critical:

- prove the fixture's signature/certification structures can be detected independently;
- prove qpdf JSON exposes enough structure for Atlas to detect the state before a transformation;
- prove a qpdf rewrite changes document bytes while the signature/DocMDP structure remains present;
- establish the Atlas policy that a structurally signed/certified PDF may not be silently mutated and then represented as retaining verified integrity;
- require independent cryptographic verification/re-signing for any future workflow that needs to make a validity claim.

Deterministic SHA-256: `95bc8daabea7d46bb70432bb65fc2fd8af3ad6e590f0fbf539e443930ce8f668`.
