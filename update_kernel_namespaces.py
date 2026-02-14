import os
import re

# Path to the source code
SOURCE_DIR = "main"

# Kernel types to migrate
KERNEL_TYPES = [
    ("System::Task", "flx::kernel::Task"),
    ("System::TaskManager", "flx::kernel::TaskManager"),
    ("System::ResourceMonitorTask", "flx::kernel::ResourceMonitorTask"),
]

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    original_content = content

    for old_type, new_type in KERNEL_TYPES:
        # Replace System::Type with flx::kernel::Type
        content = content.replace(old_type, new_type)

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
