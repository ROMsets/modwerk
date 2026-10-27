---
name: modwerk-triage-issues
description: Triage Modwerk's open issues in one pass - read and sort each, reply where a reporter must answer or a fix already exists, then present the closures and a work plan for the rest. Use when the owner or a maintainer asks to triage, go through or plan the issues, or names several; to fix one issue use /modwerk-fix-issue.
---

# /modwerk-triage-issues [number ...]

**What to triage:** the issues named; with none, every open issue that changed since your last comment on it (`gh issue list --state open --limit 200 --json number,title,labels,author,updatedAt`). With more than about ten, split the reading across subagents where the agent supports it, each returning one line per issue.

1. **Read and sort** each by the issue worker's section in `docs/MAINTENANCE.md#issue-worker`:
   - read the body and every comment (`gh issue view <n> --json body,comments`);
   - sort it: fixed or answered, duplicate, needs details, confirmed, question or feature, stale;
   - look for its fix in open pull requests and recent releases, its cause in the beta findings, and other issues with the same cause.
2. **Reply where needed**, at most one comment per issue: ask for missing details, answer a question, or point to the release or pull request that fixes it.
   - Ask with `/modwerk needs-info …` on a module you maintain, a plain reply elsewhere (`docs/DEVELOPER_WORKFLOW.md#manage-reports-on-github`).
   - Write plainly and friendly, in the invoker's voice, and post from a file.
   - Post nothing when nothing is needed, and never repeat a request already on the thread.
   - Name no tester and quote nothing from private diagnostics.
   - Replies only: no labels, confirms, closes or resolves.
3. **Present the closures** in chat as one list, a reason for each: fixed in a named version, duplicate of #N, no details after asking, withdrawn. Close only those the invoker approves, with the closure reason the report flow names.
4. **Plan the rest** in chat: the confirmed bugs and the features worth doing, one line each, in the order to work on them.
   - Say what breaks for whom, how many issues share the cause, and where in the code it likely sits.
   - Group issues one fix closes. Put first what loses work or stops the instrument, then what most people hit.
   - Say who does it: the owner with `/modwerk-fix-issue`, the module's author (someone else's module waits for their OK), or a `Fix:` pull request for shared code.

   Start only what the invoker picks, each through `/modwerk-fix-issue`.
