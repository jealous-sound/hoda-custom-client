#!/usr/bin/env python3
import argparse
import json
import os
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DISCORD_API = "https://discord.com/api/v10"
DISCORD_GUILD_ID = "1570134641669841047"
PATCH_NOTES_CHANNEL = "patch-notes"
TOKEN_ENVIRONMENT_VARIABLE = "HODA_DISCORD_BOT_TOKEN"
TOKEN_FILE = Path.home() / ".hoda-discord-bot-token"
DISCORD_MESSAGE_LIMIT = 2000
DISCORD_PAGE_SIZE = 100
RATE_LIMIT_STATUS = 429
USER_AGENT = "DiscordBot (https://example.com, 1.0)"
HEADING = re.compile(r"^## (v\d+\.\d+\.\d+) · ", re.MULTILINE)
PULL_REQUEST_REFERENCE = re.compile(r"#(\d+)")
BULLET_PREFIX = re.compile(r"^[-*]\s+")
CLI_DESCRIPTION = (
    "Write player-facing patch notes to the #patch-notes channel of the Hoda Reborn Discord server. "
    "'missing' lists releases that have no notes yet, 'sources' prints what a release changed (its pull "
    "request titles and descriptions), and 'post' publishes the notes you wrote for one release."
)
MISSING_HELP = "list published releases that have no notes in the channel"
SOURCES_HELP = "print the pull requests and commits that went into a release"
POST_HELP = "publish notes for one release; refuses a version that is already in the channel"
NOTES_FILE_HELP = "text file with one change per line; '-' reads standard input"
DRY_RUN_HELP = "print the message instead of posting it"


def version_key(tag):
    return tuple(int(part) for part in tag.lstrip("v").split("."))


def previous_tag(tags, tag):
    earlier = [other for other in tags if version_key(other) < version_key(tag)]
    return max(earlier, key=version_key) if earlier else None


def pull_request_numbers(subjects):
    numbers = []
    for subject in subjects:
        for match in PULL_REQUEST_REFERENCE.findall(subject):
            if int(match) not in numbers:
                numbers.append(int(match))
    return sorted(numbers)


def note_lines(text):
    lines = [BULLET_PREFIX.sub("", line.strip()) for line in text.splitlines()]
    return [line for line in lines if line]


def release_date(published_at):
    moment = datetime.fromisoformat(published_at.replace("Z", "+00:00"))
    return f"{moment.day} {moment:%b %Y}"


def render_note(tag, published_at, lines):
    return f"## {tag} · {release_date(published_at)}\n" + "\n".join(f"- {line}" for line in lines)


def posted_versions(contents):
    return {version for content in contents for version in HEADING.findall(content)}


def missing_versions(tags, contents):
    posted = posted_versions(contents)
    return sorted((tag for tag in tags if tag not in posted), key=version_key)


def run(*command):
    return subprocess.run(command, cwd=ROOT, check=True, capture_output=True, text=True).stdout


def published_releases():
    listing = json.loads(run("gh", "release", "list", "--limit", "1000", "--json", "tagName,publishedAt,isDraft"))
    return {release["tagName"]: release["publishedAt"] for release in listing if not release["isDraft"]}


def commit_subjects(earlier, tag):
    revisions = f"{earlier}..{tag}" if earlier else tag
    return run("git", "log", "--format=%s", revisions).splitlines()


def discord_token():
    token = os.environ.get(TOKEN_ENVIRONMENT_VARIABLE)
    if token:
        return token.strip()
    if TOKEN_FILE.exists():
        return TOKEN_FILE.read_text(encoding="utf-8").strip()
    raise SystemExit(f"no Discord bot token: set {TOKEN_ENVIRONMENT_VARIABLE} or create {TOKEN_FILE}")


def discord(method, path, body=None):
    while True:
        request = urllib.request.Request(
            DISCORD_API + path,
            method=method,
            data=None if body is None else json.dumps(body).encode(),
            headers={
                "Authorization": f"Bot {discord_token()}",
                "Content-Type": "application/json",
                "User-Agent": USER_AGENT,
            },
        )
        try:
            with urllib.request.urlopen(request) as response:
                return json.loads(response.read() or "null")
        except urllib.error.HTTPError as error:
            detail = error.read().decode()
            if error.code != RATE_LIMIT_STATUS:
                raise SystemExit(f"Discord {method} {path} failed with {error.code}: {detail}") from error
            time.sleep(json.loads(detail).get("retry_after", 1))


def patch_notes_channel_id():
    for channel in discord("GET", f"/guilds/{DISCORD_GUILD_ID}/channels"):
        if channel["name"] == PATCH_NOTES_CHANNEL:
            return channel["id"]
    raise SystemExit(f"the server has no #{PATCH_NOTES_CHANNEL} channel")


def channel_contents(channel_id):
    contents = []
    before = ""
    while True:
        page = discord("GET", f"/channels/{channel_id}/messages?limit={DISCORD_PAGE_SIZE}{before}")
        contents += [message["content"] for message in page]
        if len(page) < DISCORD_PAGE_SIZE:
            return contents
        before = f"&before={page[-1]['id']}"


def list_missing(_arguments):
    missing = missing_versions(published_releases(), channel_contents(patch_notes_channel_id()))
    print("\n".join(missing) if missing else "every release has notes")
    return 0


def print_sources(arguments):
    releases = published_releases()
    if arguments.tag not in releases:
        raise SystemExit(f"{arguments.tag} is not a published release")
    run("git", "fetch", "--quiet", "--tags", "origin")
    earlier = previous_tag(releases, arguments.tag)
    subjects = commit_subjects(earlier, arguments.tag)
    print(f"{arguments.tag}, published {release_date(releases[arguments.tag])}, changes since {earlier or 'the start'}")
    print("\nCommits:")
    print("\n".join(f"  {subject}" for subject in subjects))
    for number in pull_request_numbers(subjects):
        pull_request = json.loads(run("gh", "pr", "view", str(number), "--json", "title,body"))
        print(f"\n===== #{number}: {pull_request['title']}\n{pull_request['body']}")
    return 0


def post_note(arguments):
    releases = published_releases()
    if arguments.tag not in releases:
        raise SystemExit(f"{arguments.tag} is not a published release")
    source = sys.stdin.read() if arguments.notes == "-" else Path(arguments.notes).read_text(encoding="utf-8")
    lines = note_lines(source)
    if not lines:
        raise SystemExit("the notes are empty")
    message = render_note(arguments.tag, releases[arguments.tag], lines)
    if len(message) > DISCORD_MESSAGE_LIMIT:
        raise SystemExit(f"the notes are {len(message)} characters; Discord allows {DISCORD_MESSAGE_LIMIT}")
    if arguments.dry_run:
        print(message)
        return 0
    channel_id = patch_notes_channel_id()
    if arguments.tag in posted_versions(channel_contents(channel_id)):
        raise SystemExit(f"{arguments.tag} already has notes in #{PATCH_NOTES_CHANNEL}")
    discord("POST", f"/channels/{channel_id}/messages", {"content": message, "allowed_mentions": {"parse": []}})
    print(f"posted {arguments.tag} to #{PATCH_NOTES_CHANNEL}")
    return 0


def parse_arguments(argv):
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("missing", help=MISSING_HELP).set_defaults(handler=list_missing)
    sources = commands.add_parser("sources", help=SOURCES_HELP)
    sources.add_argument("tag")
    sources.set_defaults(handler=print_sources)
    post = commands.add_parser("post", help=POST_HELP)
    post.add_argument("tag")
    post.add_argument("notes", help=NOTES_FILE_HELP)
    post.add_argument("--dry-run", action="store_true", help=DRY_RUN_HELP)
    post.set_defaults(handler=post_note)
    return parser.parse_args(argv)


def main(argv=None):
    arguments = parse_arguments(argv)
    return arguments.handler(arguments)


if __name__ == "__main__":
    sys.exit(main())
