import os
import re

MODS_DIR = "mods"
README_FILE = "README.md"
START_TAG = "<!-- MODS_TABLE_START -->"
END_TAG = "<!-- MODS_TABLE_END -->"

def parse_mod_metadata(file_path):
    mod_name = None
    description = ""
    version = "1.0.0"

    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        # Read the top metadata block
        header = f.read(4096)

    # Extract fields from the // @key value lines
    name_match = re.search(r"//\s*@name\s+(.+)", header)
    desc_match = re.search(r"//\s*@description\s+(.+)", header)
    ver_match = re.search(r"//\s*@version\s+([0-9.]+)", header)

    if name_match:
        mod_name = name_match.group(1).strip()
    else:
        # Fallback using file name without .wh.cpp
        raw_name = os.path.basename(file_path).replace(".wh.cpp", "")
        mod_name = raw_name.replace("-", " ").title()

    if desc_match:
        description = desc_match.group(1).strip()
        # Prevent markdown table breakage if description contains pipe symbols
        description = description.replace("|", "&#124;")

    if ver_match:
        version = ver_match.group(1).strip()

    filename = os.path.basename(file_path)
    file_link = f"./{MODS_DIR}/{filename}"

    return {
        "name": mod_name,
        "desc": description,
        "version": version,
        "file_link": file_link
    }

def generate_markdown_table():
    if not os.path.exists(MODS_DIR):
        return "_No mods directory found._"

    # Only inspect .wh.cpp files inside mods/
    mod_files = sorted([
        f for f in os.listdir(MODS_DIR) 
        if f.endswith(".wh.cpp") and not f.startswith(".")
    ])

    if not mod_files:
        return "_No mods available currently._"

    rows = [
        "| Mod | Description | Version | Source |",
        "| :--- | :--- | :--- | :--- |"
    ]

    for filename in mod_files:
        filepath = os.path.join(MODS_DIR, filename)
        info = parse_mod_metadata(filepath)
        rows.append(
            f"| {info['name']} | {info['desc']} | `v{info['version']}` | [View Code]({info['file_link']}) |"
        )

    return "\n".join(rows)

def update_readme():
    if not os.path.exists(README_FILE):
        print(f"Error: {README_FILE} not found.")
        return

    table_content = generate_markdown_table()

    with open(README_FILE, "r", encoding="utf-8") as f:
        readme_text = f.read()

    pattern = rf"({re.escape(START_TAG)})(.*?)({re.escape(END_TAG)})"
    replacement = rf"\1\n{table_content}\n\3"

    if not re.search(pattern, readme_text, flags=re.DOTALL):
        print(f"Error: Marker tags {START_TAG} and {END_TAG} not found in {README_FILE}.")
        return

    new_readme_text = re.sub(pattern, replacement, readme_text, flags=re.DOTALL)

    with open(README_FILE, "w", encoding="utf-8") as f:
        f.write(new_readme_text)

    print("Successfully updated README.md.")

if __name__ == "__main__":
    update_readme()
