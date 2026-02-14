
import os
import re

# Types to migrate to flx:: namespace
TYPES_TO_MIGRATE = [
    "Singleton",
    "Observable",
    "StringObservable",
    "Result",
    "ClipboardManager",
    "ClipboardOp",
    "ClipboardEntry"
]

# Path to the source code
SOURCE_DIR = "main"

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    original_content = content

    for type_name in TYPES_TO_MIGRATE:
        # Avoid replacing already prefixed types
        # Regex looks for type_name NOT preceded by flx:: or System:: or ::
        
        # 1. Replace System::Type with flx::Type
        content = re.sub(fr'System::{type_name}\b', f'flx::{type_name}', content)
        
        # 2. Replace Type with flx::Type, ensuring it's not already prefixed
        # We use a negative lookbehind to check for ::
        # And we check for word boundaries or common surrounding chars
        
        # Pattern: (?<!::)\bType\b
        # matches "Type" but not "::Type" or "flx::Type"
        content = re.sub(f'(?<!::)\\b{type_name}\\b', f'flx::{type_name}', content)

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
