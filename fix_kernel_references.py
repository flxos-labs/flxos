import os
import re

SOURCE_DIR = "main"

# Types that are safe to replace globally (unique names)
UNIQUE_TYPES = [
    "TaskManager",
    "ResourceMonitorTask"
]

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    original_content = content

    for type_name in UNIQUE_TYPES:
        # Pattern: Word boundary, TypeName, Word boundary, NOT preceded by flx::kernel::
        # We want to replace "TaskManager" with "flx::kernel::TaskManager"
        # But ignore "flx::kernel::TaskManager"
        # Also skip #include lines and filenames (e.g. TaskManager.hpp)

        def replace_match(match, tn=type_name):
            line = match.string[match.string.rfind('\n', 0, match.start()) + 1:match.end()]
            # Skip #include lines
            if line.lstrip().startswith('#include'):
                return match.group(0)
            # Skip if followed by a dot (filename like TaskManager.hpp)
            end = match.end()
            if end < len(match.string) and match.string[end] == '.':
                return match.group(0)
            return f'flx::kernel::{tn}'

        pattern = fr'(?<!flx::kernel::)\b{type_name}\b'
        content = re.sub(pattern, replace_match, content)

    if content != original_content:
        print(f"Updating {filepath}")
        with open(filepath, 'w') as f:
            f.write(content)

def main():
    for root, dirs, files in os.walk(SOURCE_DIR):
        for file in files:
            if file.endswith('.hpp') or file.endswith('.cpp'):
                process_file(os.path.join(root, file))

if __name__ == "__main__":
    main()
