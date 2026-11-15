---
name: patch-notes
description: Write player-facing patch notes for a game release and post them to the #patch-notes channel of the Hoda Reborn Discord server. Use when asked to do, write, post or catch up on patch notes, or after a new v* release is published.
---

# Patch notes

`tools/patch_notes.py` does the mechanical parts. You write the notes.

## Steps

1. Find what is owed: `python3 tools/patch_notes.py missing` lists every published release with no notes in the
   channel, oldest first. Do them in that order so the channel stays chronological.
2. Read what the release changed: `python3 tools/patch_notes.py sources v0.20.0` prints the commits since the
   previous release and the full description of every pull request among them.
3. Write the notes to a scratch file, one change per line (a leading `- ` is optional).
4. Preview: `python3 tools/patch_notes.py post v0.20.0 notes.txt --dry-run`.
5. Post: `python3 tools/patch_notes.py post v0.20.0 notes.txt`. The tool adds the `## v0.20.0 · 15 Nov 2026`
   heading from the release's publish date, refuses a version that already has notes, and refuses a message over
   Discord's 2000 characters.

Posting is visible to every player at once and the tool cannot edit or delete. Unless the user already told you to
post, show them the dry-run text first.

## How to write them

- Write for players, in plain English. Say what they will see or do differently, not how the code changed.
- Take facts and figures from the pull request descriptions, not from titles alone and never from memory. If a
  description does not say it, do not claim it.
- Leave out developer-only work: CI, tooling, refactors, README and agent files, tests.
- A release with nothing a player can notice still gets one line, such as "Technical release with no gameplay
  changes."
- When several pull requests in one release reworked the same thing, describe where it ended up, not each step.
- When a release undoes or replaces an earlier one, say which: "(replaced in v0.15.0)", "stopped working in
  v0.14.0".
- Fixes say what was broken from the player's side: "Fixed Send on the expedition board doing nothing."
- Two to five bullets is typical. Put the biggest change first and bold a new feature's name once: `**Renown**`.
- No links, no pull request numbers, no mentions, no emoji.

## Setup

The tool needs `gh` signed in for this repository and the Discord bot token, read from the
`HODA_DISCORD_BOT_TOKEN` environment variable or from `~/.hoda-discord-bot-token`. Never print the token. If it is
missing, ask the user to put it in place; do not ask them to paste it into the chat.
