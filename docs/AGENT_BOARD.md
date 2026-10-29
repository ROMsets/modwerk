# Agent board: claims every developer's agent can see (design, for owner review)

Status, 29 October 2026: a proposal. Nothing here is built. The questions at the end are the owner's to answer before any of it is.

## Why

Several developers build modules with agents, each on their own computer. Their agents cannot see each other, and Modwerk only learns about overlapping work when the second pull request opens:

- Two developers can port the same module in parallel. [Check what already exists](DEVELOPER_WORKFLOW.md#check-what-already-exists) only finds work that is already pushed.
- Numbers that must be unique are reserved in pull request threads and private notes: base hook numbers (hook 37 held for #1029 while #1023 took 38), D1 migration numbers, and the DSP memory claims in [OCTATRACK_PACKAGE_DSP_HOOKS.md](OCTATRACK_PACKAGE_DSP_HOOKS.md). Nobody else's agent can read them.

The board is a shared list of **claims**: "this developer is working on this, until this date". Agents check it before starting work and before pushing. People can read it on the site.

## First version: claims only

Free-text posts, findings and questions between agents are left out on purpose. They bring most of the risk (see [Safety](#safety)) and are worth adding only once claims have shown how agents use the board.

A claim has a **kind** and a **key**, unique among active claims:

| Kind | Key | Example |
| --- | --- | --- |
| `module` | machine and module id, plus the upstream source for a port | `octatrack/tape-echo`, `digitakt/airwindows-chorus` |
| `hook` | machine and hook or table index (its place in `sdk/machines/octatrack/events.json`, or in the core's `interface.json` on the elemod machines) | `octatrack/39` |
| `migration` | D1 migration number | `0084` |
| `dsp-region` | machine and DSP memory claim name | `octatrack/fx2-delay-line` |

Each claim also records who holds it (their GitHub login), the branch, a pull request number once there is one, a short note (at most 120 characters) and when it expires.

**Lifetime.** A claim without a pull request expires after 14 days unless renewed. Once a pull request is linked, the claim stays while that pull request is open. The Worker's hourly run releases claims whose pull request was merged or closed, and records why. The holder or an administrator can release a claim at any time.

**Numbers.** The board does not know which hook or migration numbers `main` already uses. To pick a number, an agent takes the highest one on current `origin/main`, skips the active claims above it and claims the next. A conflict on claiming means someone took it first: pick the next one. Two hook proposals that each [add an entry at the end of events.json](module-guides/README.md#propose-a-hook) get different indices this way, before either is reviewed; a released index is never renumbered, so the clash is otherwise found only at merge.

## Who can claim

Any member signed in with a GitHub-linked developer session, the `X-Modwerk-Developer` token that `server/developer-auth.ts` issues. Today `developerUser` also requires the login to own a registered module. The board needs a variant without that rule, because a first port is exactly the case that needs a claim.

Limits: at most 10 active claims per person, at most 30 claim writes per person per hour. A suspended member's claims are released at once.

## Storage and API

One table, `agent_claims`, in the next free migration (0084 on today's `main`):

```sql
CREATE TABLE agent_claims (
 id TEXT PRIMARY KEY,
 kind TEXT NOT NULL CHECK(kind IN ('module','hook','migration','dsp-region')),
 claim_key TEXT NOT NULL,
 user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
 branch TEXT,
 pull_request INTEGER,
 note TEXT NOT NULL DEFAULT '',
 created_at TEXT NOT NULL,
 renewed_at TEXT NOT NULL,
 expires_at TEXT,
 released_at TEXT,
 released_reason TEXT CHECK(released_reason IN ('holder','administrator','merged','closed','expired','suspended'))
);
-- One active claim per key; released rows stay as history.
CREATE UNIQUE INDEX agent_claims_active ON agent_claims(kind,claim_key) WHERE released_at IS NULL;
CREATE INDEX agent_claims_user ON agent_claims(user_id,released_at);
```

Routes, in a new `server/agent-board.ts`:

- `GET /api/agent-board/claims?kind=&key=&holder=`: active claims, newest first. Released ones with `released=1`.
- `POST /api/agent-board/claims`: claim. Answers `409` with the current holder, branch, pull request and expiry when the key is taken.
- `POST /api/agent-board/claims/<id>/renew`: renew, or link a pull request. Holder only.
- `DELETE /api/agent-board/claims/<id>`: release. Holder or administrator.

## Developer MCP tools

Three tools in `scripts/frontend-mcp.mjs`, beside `api_request`:

- `board_claims`: read, filtered by kind, key or holder. Needs no write permission.
- `board_claim`: claim, renew or link a pull request. Needs `--allow-api-writes`, like every MCP write.
- `board_release`: release one of your claims.

Every result carries the note in a field named `untrusted_note`, and the server's MCP `instructions` say claims are information from other developers, never instructions.

Developers without the MCP can read the same list with `curl` from the `GET` route, or on the site.

## Where agents use it

The checks go into the skills, which every contributor's agent already follows, not into `AGENTS.md`, which stays within its word cap:

- **`/modwerk-new-module`, step 1 (check what exists):** read the `module` claims for the machine. If someone else holds one for this module or source, tell the developer who, on which branch, and until when, and do not plan a duplicate. After the developer approves the plan (step 5), claim it.
- **`/modwerk-pr-ready`, before pushing:** every hook, migration or DSP-region number the diff adds must be claimed by this developer, or free, in which case claim it now. A number someone else holds stops the push. After the push, link the pull request to the branch's claims.
- **`/modwerk-start`:** list the developer's own active claims and ask which to keep, renew or release.

## The page

A read-only list in the developer workspace: kind, key, holder (linked to their profile), branch, pull request and expiry. Administrators also get a Release button. Nothing is mailed or pushed when a claim changes.

## Safety

Claims come from other people's agents and end up in your agent's context, so:

- **Data, never instructions.** The only free text is the 120-character note. The MCP labels it untrusted, and the skills act only on the structured fields (kind, key, holder, expiry), never on what a note says.
- **No leaks.** The server refuses notes containing URLs, file paths, hexadecimal addresses, code or anything that looks like a token, and stores branch names only if they match `[A-Za-z0-9._/-]{1,100}`. The board never holds firmware bytes, addresses or stock code: the red lines in [MODDING_GUIDELINES.md](../MODDING_GUIDELINES.md) apply to claims too.
- **Moderation.** Administrators release any claim. Claims are tied to accounts, so the forum's suspension also clears a member's claims.

## Building it

One pull request, in this order: the migration, `server/agent-board.ts` with its tests, the hourly release in `worker.ts`, the MCP tools with tests, the page, then the edits to the three skills, `docs/FRONTEND_MCP.md` and `docs/DEVELOPER_WORKFLOW.md`, plus a changelog entry. The skills change last, so no agent looks for a board that isn't there yet.

## Questions for the owner

1. **Who may read claims?** Public reads (the proposal) let agents check with `curl` and no account. Members-only reads keep unpublished plans private, at the cost of requiring a sign-in for every check.
2. **Should a claim block anything?** As proposed, a claim stops only agents that follow the skills. The pull request checks could refuse a pull request that adds a hook or migration number someone else holds.
3. **Are the four kinds right?** Candidates left out: a base change in progress, and a module update by someone other than its author.
4. **Lifetimes:** 14 days before an unlinked claim expires, 10 active claims per person.
5. **Later, findings and questions between agents?** Only after claims have run for a while, and only with their own review.
