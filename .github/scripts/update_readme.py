import os
import re

MODS_DIR = "mods"
README_FILE = "README.md"
START_TAG = "<!-- MODS_TABLE_START -->"
END_TAG = "<!-- MODS_TABLE_END -->"


def extract_readme_content(content):
    """Extracts markdown inside // ==WindhawkModReadme== /* ... */ block."""
    pattern = r"//\s*==WindhawkModReadme==\s*[\r\n]+/\*\s*[\r\n]+(.*?)\s*\*/\s*[\r\n]+//\s*==/WindhawkModReadme=="
    match = re.search(pattern, content, re.DOTALL)
    if match:
        return match.group(1).strip()
    return None


def parse_mod(file_path):
    filename = os.path.basename(file_path)

    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        full_content = f.read()

    # Read the first 4KB for metadata tags
    header = full_content[:4096]

    id_match = re.search(r"//\s*@id\s+([a-zA-Z0-9_\-]+)", header)
    name_match = re.search(r"//\s*@name\s+(.+)", header)
    desc_match = re.search(r"//\s*@description\s+(.+)", header)
    ver_match = re.search(r"//\s*@version\s+([0-9.]+)", header)

    mod_id = (
        id_match.group(1).strip()
        if id_match
        else filename.replace(".wh.cpp", "")
    )

    if name_match:
        mod_name = name_match.group(1).strip()
    else:
        mod_name = mod_id.replace("-", " ").title()

    description = desc_match.group(1).strip() if desc_match else ""
    description = description.replace("|", "&#124;")

    version = ver_match.group(1).strip() if ver_match else "1.0.0"

    readme_text = extract_readme_content(full_content)

    return {
        "id": mod_id,
        "name": mod_name,
        "desc": description,
        "version": version,
        "filename": filename,
        "readme_text": readme_text,
    }


def sync_mod_readme(mod_info):
    """Writes mods/<id>.md ONLY if it doesn't exist or content has changed."""
    if not mod_info["readme_text"]:
        return False

    doc_path = os.path.join(MODS_DIR, f"{mod_info['id']}.md")
    new_content = mod_info["readme_text"] + "\n"

    # Check if file exists and has identical content
    if os.path.exists(doc_path):
        with open(doc_path, "r", encoding="utf-8") as f:
            existing_content = f.read()
        if existing_content == new_content:
            return True  # Exists and unchanged

    # Write only if new or modified
    with open(doc_path, "w", encoding="utf-8") as f:
        f.write(new_content)
    print(f"Updated docs for: {mod_info['id']}")
    return True


def cleanup_orphan_readmes(valid_ids):
    """Removes any <id>.md in mods/ that no longer has a corresponding .wh.cpp."""
    for fname in os.listdir(MODS_DIR):
        if fname.endswith(".md"):
            mod_id = fname[:-3]
            if mod_id not in valid_ids:
                os.remove(os.path.join(MODS_DIR, fname))
                print(f"Removed orphaned doc: {fname}")


def sync_all():
    if not os.path.exists(MODS_DIR) or not os.path.exists(README_FILE):
        return

    mod_files = [
        f
        for f in os.listdir(MODS_DIR)
        if f.endswith(".wh.cpp") and not f.startswith(".")
    ]

    parsed_mods = []
    valid_ids = set()

    for filename in mod_files:
        filepath = os.path.join(MODS_DIR, filename)
        info = parse_mod(filepath)
        parsed_mods.append(info)
        valid_ids.add(info["id"])
        sync_mod_readme(info)

    cleanup_orphan_readmes(valid_ids)

    # Sort alphabetically by Mod display name
    parsed_mods.sort(key=lambda x: x["name"].lower())

    # Build main table
    rows = [
        "| Mod | Description | Version | Docs | Source |",
        "| :--- | :--- | :--- | :--- | :--- |",
    ]

    for info in parsed_mods:
        code_link = f"[Code](./{MODS_DIR}/{info['filename']})"
        if info["readme_text"]:
            docs_link = f"[Readme](./{MODS_DIR}/{info['id']}.md)"
        else:
            docs_link = "-"

        rows.append(
            f"| {info['name']} | {info['desc']} | `v{info['version']}` | {docs_link} | {code_link} |"
        )

    new_table = "\n".join(rows)

    with open(README_FILE, "r", encoding="utf-8") as f:
        readme_text = f.read()

    pattern = rf"({re.escape(START_TAG)})(.*?)({re.escape(END_TAG)})"
    replacement = rf"\1\n{new_table}\n\3"

    updated_readme = re.sub(pattern, replacement, readme_text, flags=re.DOTALL)

    # Write README.md only if the table actually changed
    if updated_readme != readme_text:
        with open(README_FILE, "w", encoding="utf-8") as f:
            f.write(updated_readme)
        print("Updated root README.md table.")
    else:
        print("README.md table is already up-to-date.")


if __name__ == "__main__":
    sync_all()
