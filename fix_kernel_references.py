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
        
        pattern = fr'(?<!flx::kernel::)\b{type_name}\b'
        replacement = fr'flx::kernel::{type_name}'
        
        content = re.sub(pattern, replacement, content)

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
