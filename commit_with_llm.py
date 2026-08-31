#!/usr/bin/env python3
"""
Prepare, commit and push the current Git changes with an Ollama-generated
commit message.

Default flow:
1. optionally build limited repository context for the LLM;
2. stage every change with `git add -A`, including deleted files;
3. unstage excluded files (for example `.~lock.*`) and oversized files;
4. generate one concise comment per changed file from its isolated diff;
5. generate a global commit message from those comments;
6. commit and run `git push`.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
import textwrap
import unicodedata
from dataclasses import dataclass
from pathlib import Path, PurePosixPath


DEFAULT_MODEL = "ministral-3:14b"
DEFAULT_MAX_DIFF_CHARS = 45000
DEFAULT_MAX_FILE_MB = 90.0
DEFAULT_LOG_COUNT = 12
MAX_COMMIT_MESSAGE_ATTEMPTS = 2
MIN_COMMIT_SUMMARY_ALNUM_CHARS = 8
MIN_COMMIT_MESSAGE_ALNUM_CHARS = 24
LLM_EXCLUDED_ROOTS = (
    PurePosixPath("obsidian/these/.obsidian"),
)


@dataclass(frozen=True)
class ChangedFile:
    status: str
    path: str
    old_path: str | None = None


@dataclass(frozen=True)
class OversizedFile:
    path: str
    size_bytes: int
    old_path: str | None = None


@dataclass(frozen=True)
class PromptContext:
    git_log: str = ""
    outline: str = ""


class CommandError(RuntimeError):
    pass


def decode_output(output: bytes) -> str:
    return output.decode("utf-8", errors="replace").strip()


def run_git(args: list[str], cwd: Path, *, raw: bool = False) -> str | bytes:
    result = subprocess.run(
        ["git", *args],
        cwd=cwd,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        command = "git " + " ".join(args)
        stderr = decode_output(result.stderr)
        raise CommandError(f"Command `{command}` failed:\n{stderr}")
    if raw:
        return result.stdout
    return decode_output(result.stdout)


def run_git_for_display(args: list[str], cwd: Path) -> str:
    # Git reports some success information (notably `git push`) on stderr.
    result = subprocess.run(
        ["git", *args],
        cwd=cwd,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    output = decode_output(result.stdout)
    if result.returncode != 0:
        command = "git " + " ".join(args)
        if output:
            raise CommandError(f"Command `{command}` failed:\n{output}")
        raise CommandError(f"Command `{command}` failed.")
    return output


def git_root(start: Path) -> Path:
    return Path(run_git(["rev-parse", "--show-toplevel"], start)).resolve()


def require_ollama():
    try:
        import ollama  # type: ignore
    except ImportError as exc:
        raise RuntimeError(
            "The Python package `ollama` is missing for interpreter "
            f"`{sys.executable}`. Install it with: "
            f"{sys.executable} -m pip install ollama"
        ) from exc
    return ollama


def decode_path(value: bytes) -> str:
    return value.decode("utf-8", errors="surrogateescape")


def parse_name_status(raw_status: bytes) -> list[ChangedFile]:
    parts = [part for part in raw_status.split(b"\0") if part]
    entries: list[ChangedFile] = []
    index = 0
    while index < len(parts):
        status = decode_path(parts[index])
        index += 1
        if status.startswith(("R", "C")):
            if index + 1 >= len(parts):
                raise RuntimeError("Unexpected `git diff --name-status -z` output.")
            old_path = decode_path(parts[index])
            new_path = decode_path(parts[index + 1])
            index += 2
            entries.append(ChangedFile(status=status, path=new_path, old_path=old_path))
        else:
            if index >= len(parts):
                raise RuntimeError("Unexpected `git diff --name-status -z` output.")
            path = decode_path(parts[index])
            index += 1
            entries.append(ChangedFile(status=status, path=path))
    return entries


def status_label(status: str, *, language: str = "en") -> str:
    primary = status[0] if status else "?"
    labels_by_language = {
        "fr": {
            "A": "ajout",
            "M": "modification",
            "D": "suppression",
            "R": "renommage",
            "C": "copie",
            "T": "changement de type",
            "U": "conflit",
        },
        "en": {
            "A": "addition",
            "M": "modification",
            "D": "deletion",
            "R": "rename",
            "C": "copy",
            "T": "type change",
            "U": "conflict",
        },
    }
    labels = labels_by_language.get(language, labels_by_language["fr"])
    if status.startswith(("R", "C")) and len(status) > 1:
        return f"{labels.get(primary, status)} ({status[1:]}%)"
    return labels.get(primary, status)


def truncate_text(text: str, max_chars: int) -> tuple[str, bool]:
    if max_chars <= 0 or len(text) <= max_chars:
        return text, False
    head = max_chars // 2
    tail = max_chars - head
    omitted = len(text) - max_chars
    marker = f"\n\n[... diff truncated: {omitted} characters omitted ...]\n\n"
    return text[:head] + marker + text[-tail:], True


def recent_git_log(repo: Path, count: int) -> str:
    try:
        return run_git(
            [
                "log",
                f"-{count}",
                "--date=short",
                "--pretty=format:%h %ad %s",
            ],
            repo,
        )
    except CommandError:
        return "(git history unavailable)"


def changed_directories(changed_files: list[ChangedFile]) -> list[str]:
    return sorted(
        {
            str(PurePosixPath(item.path).parent)
            for item in changed_files
            if str(PurePosixPath(item.path).parent) != "."
        }
    )


def repository_outline(repo: Path, changed_files: list[ChangedFile]) -> str:
    tracked = run_git(["ls-files", "-z"], repo, raw=True)
    paths = [decode_path(part) for part in tracked.split(b"\0") if part]
    first_level: dict[str, int] = {}
    second_level: dict[str, int] = {}
    for path in paths:
        segments = path.split("/")
        if not segments:
            continue
        first_level[segments[0]] = first_level.get(segments[0], 0) + 1
        if len(segments) > 1:
            key = "/".join(segments[:2])
            second_level[key] = second_level.get(key, 0) + 1

    changed_dirs = changed_directories(changed_files)

    top = ", ".join(f"{name} ({count})" for name, count in sorted(first_level.items()))
    touched = "\n".join(f"- {path}" for path in changed_dirs[:30])
    if len(changed_dirs) > 30:
        touched += f"\n- ... {len(changed_dirs) - 30} more directories"

    frequent_second_level = sorted(
        second_level.items(), key=lambda item: item[1], reverse=True
    )[:20]
    second = "\n".join(f"- {name}/ ({count})" for name, count in frequent_second_level)

    return textwrap.dedent(
        f"""
        Repository root:
        {repo}

        Tracked top-level entries:
        {top or "(none)"}

        Main tracked subdirectories:
        {second or "(none)"}

        Directories touched by this commit:
        {touched or "(repository root only)"}
        """
    ).strip()


def should_include_git_log(changed_files: list[ChangedFile]) -> bool:
    return len(changed_files) >= 4 and len(changed_directories(changed_files)) >= 2


def should_include_outline(changed_files: list[ChangedFile]) -> bool:
    return len(changed_files) >= 2 or any(
        str(PurePosixPath(item.path).parent) != "." for item in changed_files
    )


def build_prompt_context(
    repo: Path, changed_files: list[ChangedFile], log_count: int
) -> PromptContext:
    outline = repository_outline(repo, changed_files) if should_include_outline(
        changed_files
    ) else ""
    git_log = (
        recent_git_log(repo, log_count)
        if log_count > 0 and should_include_git_log(changed_files)
        else ""
    )
    return PromptContext(git_log=git_log, outline=outline)


def staged_changed_files(repo: Path) -> list[ChangedFile]:
    raw_status = run_git(["diff", "--cached", "--name-status", "-z"], repo, raw=True)
    return parse_name_status(raw_status)


def candidate_paths_for_changed_file(changed_file: ChangedFile) -> list[str]:
    paths = [changed_file.path]
    if changed_file.old_path:
        paths.append(changed_file.old_path)
    return paths


def is_commit_excluded_path(path: str) -> bool:
    return Path(path).name.startswith(".~lock.")


def is_commit_excluded_file(changed_file: ChangedFile) -> bool:
    return any(
        is_commit_excluded_path(path)
        for path in candidate_paths_for_changed_file(changed_file)
    )


def is_llm_excluded_path(path: str) -> bool:
    git_path = PurePosixPath(path)
    return any(
        git_path == excluded_root or excluded_root in git_path.parents
        for excluded_root in LLM_EXCLUDED_ROOTS
    )


def is_llm_excluded_file(changed_file: ChangedFile) -> bool:
    return any(
        is_llm_excluded_path(path)
        for path in candidate_paths_for_changed_file(changed_file)
    )


def split_llm_eligible_files(
    changed_files: list[ChangedFile],
) -> tuple[list[ChangedFile], list[ChangedFile]]:
    llm_eligible: list[ChangedFile] = []
    llm_excluded: list[ChangedFile] = []
    for changed_file in changed_files:
        if is_llm_excluded_file(changed_file):
            llm_excluded.append(changed_file)
        else:
            llm_eligible.append(changed_file)
    return llm_eligible, llm_excluded


def bytes_from_mb(value: float) -> int:
    return int(value * 1024 * 1024)


def human_size(size_bytes: int) -> str:
    return f"{size_bytes / (1024 * 1024):.1f} MB"


def find_oversized_staged_files(
    repo: Path, changed_files: list[ChangedFile], max_bytes: int
) -> list[OversizedFile]:
    oversized: list[OversizedFile] = []
    for changed_file in changed_files:
        if changed_file.status.startswith("D"):
            continue

        absolute_path = repo / changed_file.path
        try:
            if not absolute_path.is_file():
                continue
            size_bytes = absolute_path.stat().st_size
        except OSError:
            continue

        if size_bytes > max_bytes:
            oversized.append(
                OversizedFile(
                    path=changed_file.path,
                    size_bytes=size_bytes,
                    old_path=changed_file.old_path,
                )
            )
    return oversized


def unstage_paths(repo: Path, paths: list[str]) -> None:
    if not paths:
        return

    with tempfile.NamedTemporaryFile("wb", delete=False) as pathspec_file:
        pathspec_path = Path(pathspec_file.name)
        for path in paths:
            pathspec_file.write(path.encode("utf-8", errors="surrogateescape"))
            pathspec_file.write(b"\0")

    try:
        run_git(
            [
                "restore",
                "--staged",
                "--pathspec-from-file",
                str(pathspec_path),
                "--pathspec-file-nul",
            ],
            repo,
        )
    finally:
        try:
            pathspec_path.unlink()
        except FileNotFoundError:
            pass


def exclude_commit_excluded_staged_files(
    repo: Path, changed_files: list[ChangedFile]
) -> list[ChangedFile]:
    excluded_files = [
        changed_file
        for changed_file in changed_files
        if is_commit_excluded_file(changed_file)
    ]
    paths_to_unstage: list[str] = []
    for item in excluded_files:
        paths_to_unstage.extend(candidate_paths_for_changed_file(item))

    unstage_paths(repo, paths_to_unstage)
    return excluded_files


def exclude_oversized_staged_files(
    repo: Path, changed_files: list[ChangedFile], max_file_mb: float
) -> list[OversizedFile]:
    max_bytes = bytes_from_mb(max_file_mb)
    oversized = find_oversized_staged_files(repo, changed_files, max_bytes)
    paths_to_unstage: list[str] = []
    for item in oversized:
        if item.old_path:
            paths_to_unstage.append(item.old_path)
        paths_to_unstage.append(item.path)

    unstage_paths(repo, paths_to_unstage)
    return oversized


def format_oversized_files(oversized_files: list[OversizedFile]) -> str:
    lines = [
        f"- {item.path} ({human_size(item.size_bytes)})" for item in oversized_files[:20]
    ]
    if len(oversized_files) > 20:
        lines.append(f"- ... {len(oversized_files) - 20} more file(s)")
    return "\n".join(lines)


def staged_diff_for_file(repo: Path, changed_file: ChangedFile) -> str:
    pathspec = changed_file.path
    diff = run_git(["diff", "--cached", "--", pathspec], repo)
    if diff:
        return diff
    if changed_file.old_path:
        return run_git(["diff", "--cached", "--", changed_file.old_path], repo)
    return ""


def extract_ollama_content(response) -> str:
    message = getattr(response, "message", None)
    if message is not None:
        content = getattr(message, "content", None)
        if isinstance(content, str):
            return content.strip()
        if isinstance(message, dict):
            content = message.get("content")
            if isinstance(content, str):
                return content.strip()

    if isinstance(response, dict):
        nested = response.get("message")
        if isinstance(nested, dict):
            content = nested.get("content")
            if isinstance(content, str):
                return content.strip()
        content = response.get("response")
        if isinstance(content, str):
            return content.strip()

    raise RuntimeError("Unexpected Ollama response: unable to extract text.")


def ollama_chat(ollama, model: str, messages: list[dict[str, str]]) -> str:
    try:
        response = ollama.chat(
            model=model,
            messages=messages,
            options={
                "temperature": 0.2,
                "top_p": 0.9,
            },
        )
    except Exception as exc:  # Ollama exposes different exception classes by version.
        raise RuntimeError(
            f"Unable to call Ollama with model `{model}`. "
            "Check that Ollama is running and that the model is installed "
            f"(`ollama pull {model}`)."
        ) from exc
    return extract_ollama_content(response)


def format_prompt_section(title: str, content: str) -> str:
    content = content.strip()
    if not content:
        return ""
    return f"{title}:\n{content}"


def per_file_prompt(
    *,
    git_log: str,
    outline: str,
    changed_file: ChangedFile,
    diff: str,
    truncated: bool,
) -> list[dict[str, str]]:
    old_path = (
        f"\nPrevious path: {changed_file.old_path}" if changed_file.old_path else ""
    )
    truncation_note = (
        "\nThe diff was truncated in the middle; mention only the visible changes."
        if truncated
        else ""
    )
    sections = [
        format_prompt_section("Recent repository history", git_log),
        format_prompt_section("Useful repository structure", outline),
        textwrap.dedent(
            f"""
            File: {changed_file.path}{old_path}
            Status: {status_label(changed_file.status, language="en")} ({changed_file.status})
            {truncation_note}

            Isolated file diff:
            ```diff
            {diff or "(empty diff or binary file without a textual patch)"}
            ```
            """
        ).strip(),
    ]
    user_content = "\n\n".join(section for section in sections if section)

    return [
        {
            "role": "system",
            "content": (
                "You write commit comments in English. "
                "Be precise, concise, and factual. Start from the file path and the "
                "visible diff content. Use the repository structure only to resolve "
                "local ambiguity. Use recent history only when it explicitly confirms "
                "something already visible in the diff or path. Never invent project "
                "goals, article topics, institutions, corpora, deliverables, or "
                "context that are absent from the diff. "
                "Do not describe Git itself. Reply with only 1 to 3 short lines of "
                "plain text, with no bullets and no Markdown syntax."
            ),
        },
        {"role": "user", "content": user_content},
    ]


def summary_prompt(
    *,
    git_log: str,
    outline: str,
    file_comments: list[tuple[ChangedFile, str]],
) -> list[dict[str, str]]:
    comments = "\n\n".join(
        f"File: {item.path}\nStatus: {status_label(item.status, language='en')} ({item.status})\nComments:\n{comment}"
        for item, comment in file_comments
    )
    sections = [
        format_prompt_section("Recent repository history", git_log),
        format_prompt_section("Useful repository structure", outline),
        format_prompt_section("Comments gathered file by file", comments),
        textwrap.dedent(
            """
            Now produce the final commit message.

            Required format:
            <clear concise summary>
            <concise detail 1>
            <concise detail 2>

            Constraints:
            - the message must be plain text only;
            - no Markdown syntax: no bullets, no numbered list, no code block, no title;
            - one idea per line after the global summary;
            - the global summary must stay on a single line;
            - never write the phrase "Commit summary:";
            - each line must be complete, with no truncated word and no incomplete prefix;
            - the detail lines should cover meaningful changes without repeating every file comment when several changes are similar;
            - keep proper nouns, directories, or important corpora only if they already appear in the file-by-file comments;
            - do not add a code block, an extra title, or meta commentary.
            """
        ).strip(),
    ]
    user_content = "\n\n".join(section for section in sections if section)

    return [
        {
            "role": "system",
            "content": (
                "You transform diff observations into an English commit message. "
                "The result must be directly usable as a Git commit message in plain "
                "text. Keep only information supported by the file-by-file comments. "
                "Ignore any historical or structural context that would add details "
                "missing from those comments."
            ),
        },
        {"role": "user", "content": user_content},
    ]


def strip_generated_text_prefix(line: str) -> str:
    stripped = line.strip()
    stripped = re.sub(r"^[-*+]\s+", "", stripped)
    stripped = re.sub(r"^\d+[.)]\s+", "", stripped)
    stripped = stripped.replace("`", "")
    stripped = stripped.replace("*", "")
    return stripped


def alnum_char_count(text: str) -> int:
    return sum(1 for char in text if char.isalnum())


def commit_message_quality_issue(message: str) -> str | None:
    lines = [line.strip() for line in message.splitlines() if line.strip()]
    if not lines:
        return "empty message"

    summary = lines[0]
    if alnum_char_count(summary) < MIN_COMMIT_SUMMARY_ALNUM_CHARS:
        return f"summary too short: {summary!r}"

    if alnum_char_count("\n".join(lines)) < MIN_COMMIT_MESSAGE_ALNUM_CHARS:
        return "overall message too short"

    return None


def retry_messages_after_short_output(
    messages: list[dict[str, str]], previous_output: str, issue: str
) -> list[dict[str, str]]:
    retry_instruction = textwrap.dedent(
        f"""
        Your previous reply is invalid because it looks too short or truncated ({issue}).
        Start over from the beginning.

        Additional constraints:
        - write a complete summary with multiple words;
        - never end on a word fragment;
        - if unsure, prefer a generic but complete wording.
        """
    ).strip()
    return [
        *messages,
        {"role": "assistant", "content": previous_output},
        {"role": "user", "content": retry_instruction},
    ]


def normalize_commit_message(message: str) -> str:
    summary_labels = (
        "Résumé global du commit :",
        "Commit summary:",
        "Global commit summary:",
    )
    normalized_summary_labels = tuple(
        "".join(
            char
            for char in unicodedata.normalize("NFKD", label.lower())
            if not unicodedata.combining(char)
        ).rstrip(":")
        for label in summary_labels
    )

    def extract_summary_content(line: str) -> str | None:
        line = strip_generated_text_prefix(line)
        normalized = "".join(
            char
            for char in unicodedata.normalize("NFKD", line.lower())
            if not unicodedata.combining(char)
        )
        normalized = normalized.replace("*", "").replace("_", "").replace("`", "")
        if not any(
            normalized.strip().startswith(label)
            for label in normalized_summary_labels
        ):
            return None

        stripped = re.sub(r"^[\s*_`]+", "", line.strip())
        if ":" not in stripped:
            return ""

        content = stripped.split(":", 1)[1].strip()
        content = re.sub(r"^[\s*_`]+", "", content)
        return content

    lines = [line.rstrip() for line in message.strip().splitlines()]
    if not lines:
        raise RuntimeError("The LLM returned an empty commit message.")

    leading_summaries: list[str] = []
    index = 0
    while index < len(lines):
        if not lines[index].strip():
            index += 1
            continue

        summary_content = extract_summary_content(lines[index])
        if summary_content is None:
            break

        leading_summaries.append(summary_content)
        index += 1

    if leading_summaries:
        remaining_lines = [
            strip_generated_text_prefix(line) for line in lines[index:] if line.strip()
        ]
        canonical_summary = next(
            (item.strip() for item in reversed(leading_summaries) if item.strip()),
            "",
        )
        if not canonical_summary and remaining_lines:
            canonical_summary = remaining_lines.pop(0)

        if not canonical_summary:
            raise RuntimeError("The final commit message does not contain a summary.")

        lines = [canonical_summary]
        if remaining_lines:
            lines.extend(remaining_lines)

    normalized_lines = [strip_generated_text_prefix(line) for line in lines if line.strip()]
    if not normalized_lines:
        raise RuntimeError("The final commit message is empty after normalization.")

    normalized = "\n".join(normalized_lines) + "\n"
    return normalized


def write_temp_commit_message(repo: Path, message: str) -> Path:
    temp = tempfile.NamedTemporaryFile(
        "w",
        encoding="utf-8",
        prefix="llm-commit-",
        suffix=".txt",
        dir=repo,
        delete=False,
    )
    with temp:
        temp.write(message)
    return Path(temp.name)


def latest_commit_summary(repo: Path) -> str:
    commit_hash = run_git(["rev-parse", "--short", "HEAD"], repo)
    shortstat = run_git(["show", "--shortstat", "--format=", "-1", "HEAD"], repo)
    lines = [f"Created commit: {commit_hash}"]
    if shortstat.strip():
        lines.append(shortstat.strip())
    return "\n".join(lines)


def commit_with_message(repo: Path, message: str) -> str:
    message_file = write_temp_commit_message(repo, message)
    try:
        run_git(["commit", "--quiet", "-F", str(message_file)], repo)
        return latest_commit_summary(repo)
    finally:
        try:
            message_file.unlink()
        except FileNotFoundError:
            pass


def push(repo: Path) -> str:
    output = run_git_for_display(["push"], repo)
    if output:
        return output
    return "Push completed with no Git output."


def snapshot_index(repo: Path) -> str:
    return run_git(["write-tree"], repo)


def restore_index(repo: Path, tree_hash: str) -> None:
    run_git(["read-tree", tree_hash], repo)


def print_section(title: str, body: str) -> None:
    print(f"\n== {title} ==")
    print(body.strip() if body.strip() else "(empty)")


def format_changed_files(files: list[ChangedFile]) -> str:
    return "\n".join(
        f"- {item.status} {item.old_path + ' -> ' if item.old_path else ''}{item.path}"
        for item in files
    )


def fallback_commit_message_for_excluded_files(
    excluded_files: list[ChangedFile],
) -> str:
    if excluded_files and all(is_llm_excluded_file(item) for item in excluded_files):
        summary = "Update Obsidian configuration files"
        detail = "Files under .obsidian were included in the commit without LLM analysis."
        return f"{summary}\n{detail}\n"

    return (
        "Update files excluded from LLM analysis\n"
        "Files excluded from LLM analysis were included in the commit.\n"
    )


def fallback_commit_message_from_comments(
    file_comments: list[tuple[ChangedFile, str]],
) -> str:
    if not file_comments:
        return "Repository update\nCommit message generated without a reliable LLM summary.\n"

    if len(file_comments) == 1:
        summary = f"Update {file_comments[0][0].path}"
    else:
        summary = f"Update {len(file_comments)} files"

    detail_lines: list[str] = []
    for changed_file, comment in file_comments:
        comment_lines = [
            strip_generated_text_prefix(line)
            for line in comment.splitlines()
            if strip_generated_text_prefix(line)
        ]
        if comment_lines:
            detail = comment_lines[0]
        else:
            detail = (
                f"{status_label(changed_file.status, language='en').capitalize()}: "
                f"{changed_file.path}"
            )

        if detail == summary or detail in detail_lines:
            continue
        detail_lines.append(detail)
        if len(detail_lines) == 2:
            break

    return "\n".join([summary, *detail_lines]) + "\n"


def generate_commit_message_from_file_comments(
    ollama,
    model: str,
    *,
    git_log: str,
    outline: str,
    file_comments: list[tuple[ChangedFile, str]],
) -> str:
    messages = summary_prompt(
        git_log=git_log,
        outline=outline,
        file_comments=file_comments,
    )

    for attempt in range(1, MAX_COMMIT_MESSAGE_ATTEMPTS + 1):
        raw_message = ollama_chat(ollama, model, messages)
        try:
            normalized_message = normalize_commit_message(raw_message)
        except RuntimeError as exc:
            issue = str(exc)
        else:
            issue = commit_message_quality_issue(normalized_message)
            if issue is None:
                return normalized_message

        if attempt < MAX_COMMIT_MESSAGE_ATTEMPTS:
            print(f"Invalid LLM commit message ({issue}). Retrying...")
            messages = retry_messages_after_short_output(messages, raw_message, issue)
            continue

        print(
            f"Invalid LLM commit message ({issue}). "
            "Using a deterministic fallback."
        )
        return normalize_commit_message(
            fallback_commit_message_from_comments(file_comments)
        )

    raise RuntimeError("Unable to generate a commit message.")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Generate a commit message with Ollama, commit the current changes, "
            "then run git push."
        )
    )
    parser.add_argument(
        "--model",
        default=os.environ.get("OLLAMA_COMMIT_MODEL", DEFAULT_MODEL),
        help=f"Ollama model to use (default: {DEFAULT_MODEL})",
    )
    parser.add_argument(
        "--log-count",
        type=int,
        default=DEFAULT_LOG_COUNT,
        help=(
            "maximum number of recent commits available to the LLM when "
            "historical context is considered useful; 0 disables it "
            f"(default: {DEFAULT_LOG_COUNT})"
        ),
    )
    parser.add_argument(
        "--max-diff-chars",
        type=int,
        default=DEFAULT_MAX_DIFF_CHARS,
        help=(
            "maximum diff size sent per file; the middle is truncated beyond "
            f"that limit (default: {DEFAULT_MAX_DIFF_CHARS})"
        ),
    )
    parser.add_argument(
        "--max-file-mb",
        type=float,
        default=DEFAULT_MAX_FILE_MB,
        help=(
            "maximum file size to include in the commit, in MB; "
            f"larger files are unstaged (default: {DEFAULT_MAX_FILE_MB:g})"
        ),
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help=(
            "generate the message and show what would be done, without committing "
            "or pushing; the initial Git index is restored at the end"
        ),
    )
    parser.add_argument(
        "--no-push",
        action="store_true",
        help="create the commit but do not run git push",
    )
    parser.add_argument(
        "--skip-llm",
        action="store_true",
        help="only test change detection and staging, without calling Ollama",
    )
    return parser


def main() -> int:
    args = build_parser().parse_args()
    start = Path.cwd()
    repo = git_root(start)
    index_snapshot: str | None = None

    print(f"Git repository: {repo}")

    if args.dry_run:
        index_snapshot = snapshot_index(repo)

    try:
        print("Staging all changes with `git add -A`...")
        run_git(["add", "-A"], repo)

        changed_files = staged_changed_files(repo)
        commit_excluded_files = exclude_commit_excluded_staged_files(
            repo, changed_files
        )
        if commit_excluded_files:
            print_section(
                "Files excluded from the commit",
                format_changed_files(commit_excluded_files),
            )
            changed_files = staged_changed_files(repo)

        oversized_files = exclude_oversized_staged_files(
            repo, changed_files, args.max_file_mb
        )
        if oversized_files:
            print_section(
                f"Files unstaged because they exceed {args.max_file_mb:g} MB",
                format_oversized_files(oversized_files),
            )
            changed_files = staged_changed_files(repo)

        if not changed_files:
            print("No changes to commit.")
            return 0

        print(f"{len(changed_files)} file(s) staged.")
        llm_changed_files, llm_excluded_files = split_llm_eligible_files(changed_files)

        if llm_excluded_files:
            print_section(
                "Files excluded from LLM analysis",
                format_changed_files(llm_excluded_files),
            )

        if args.skip_llm:
            print_section(
                "Staged files",
                format_changed_files(changed_files),
            )
            print("Stopping as requested by --skip-llm.")
            return 0

        commit_message: str
        if not llm_changed_files:
            print("No file is eligible for LLM analysis.")
            commit_message = normalize_commit_message(
                fallback_commit_message_for_excluded_files(llm_excluded_files)
            )
        else:
            print("Checking the Ollama Python API...")
            ollama = require_ollama()
            prompt_context = build_prompt_context(repo, llm_changed_files, args.log_count)
            if prompt_context.git_log:
                print("LLM context: recent Git history included.")
            else:
                print("LLM context: Git history omitted to limit false positives.")
            if prompt_context.outline:
                print("LLM context: repository structure included.")
            file_comments: list[tuple[ChangedFile, str]] = []

            for index, changed_file in enumerate(llm_changed_files, start=1):
                print(
                    f"[{index}/{len(llm_changed_files)}] LLM analysis: {changed_file.path}"
                )
                diff = staged_diff_for_file(repo, changed_file)
                limited_diff, truncated = truncate_text(diff, args.max_diff_chars)
                comment = ollama_chat(
                    ollama,
                    args.model,
                    per_file_prompt(
                        git_log=prompt_context.git_log,
                        outline=prompt_context.outline,
                        changed_file=changed_file,
                        diff=limited_diff,
                        truncated=truncated,
                    ),
                )
                file_comments.append((changed_file, comment))

            print("Building the overall commit summary...")
            commit_message = generate_commit_message_from_file_comments(
                ollama,
                args.model,
                git_log=prompt_context.git_log,
                outline=prompt_context.outline,
                file_comments=file_comments,
            )

        print_section("Generated commit message", commit_message)

        if args.dry_run:
            print("Dry run: commit and push were not executed.")
            return 0

        print("Creating commit...")
        commit_output = commit_with_message(repo, commit_message)
        print_section("Git commit", commit_output)

        if args.no_push:
            print("`--no-push` set: push was not executed.")
            return 0

        print("Pushing to the configured remote...")
        push_output = push(repo)
        print_section("Git push", push_output)
        return 0
    finally:
        if index_snapshot is not None:
            restore_index(repo, index_snapshot)
            print("Git index restored after dry run.")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (CommandError, RuntimeError) as exc:
        print(f"\nError: {exc}", file=sys.stderr)
        raise SystemExit(1)
