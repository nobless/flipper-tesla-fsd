# User-provided board test outputs

Board: LilyGO T-2CAN FD (H784-A). Captures supplied on 2026-10-31.
These are serial startup/test outputs, not compiler logs or successful hardware certifications.

| Firmware | Original output | Source |
| --- | --- | --- |
| beta 8 | [Serial log](beta8/serial-log.txt) | Inline serial log pasted in this chat; original formatting retained, including literal \\n sequences and Markdown links. |
| beta 9 | [Serial log](beta9/serial-log.txt) | Inline serial log pasted in this chat; original formatting retained, including literal \\n sequences and Markdown links. |
| beta 10 | [Serial log](beta10/serial-log.txt) | Inline serial log pasted in this chat; original formatting retained, including literal \\n sequences and Markdown links. |
| beta 12 | [Serial log](beta12/serial-log.txt) | Original attached serial log in this chat. |
| beta 13 | [Serial log](beta13/serial-log.txt) | Original attached serial log from the linked Diagnose MCP2518FD Failure chat. |
| beta 14 | [Serial log](beta14/serial-log.txt) | Original attached serial log in this chat. |
| beta 16 | [Three captures and analysis](beta16/README.md) | Previously archived from the linked chat; variant identity is confirmed only where the capture explicitly names it. |

The files preserve the supplied text. Inline pastes may contain Markdown formatting,
literal backslash-n sequences or incomplete capture boundaries; these have not been
silently repaired. No outputs are invented for versions whose logs were not supplied.

Observed progression: beta 8/9 report zero register/RAM reads; beta 10 fails configuration;
beta 12 adds raw register snapshots; beta 13/14 add further SPI diagnostics.
See the original captures for exact results and the beta 16 analysis for the independent
driver comparisons. Failed verification does not identify the specific electrical fault.

Firmware and build workflows are unchanged by this archive.
