#!/usr/bin/env python3
"""
Replace all occurrences of 'Services::' with 'flx::services::' in C++ files.
This fixes references that were previously relying on 'System::Services' (or 'Services' within 'System' namespace)
and now need to point to the new 'flx::services' namespace.
"""

import os
import re
from pathlib import Path

def update_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    
    # Replace Services:: with flx::services::
    # match Services:: but NOT flx::Services:: (if that even referenced anything, but to be safe)
    # The new namespace is lowercase 'services', old was 'Services'.
    
    # Simple replacement should work because new namespace is lowercase
    new_content = content.replace('Services::', 'flx::services::')
    
    # Also fix any "namespace Services =" or "namespace Services {" that might be left
    # But we want to target usages like Services::SystemInfoService
    
    if new_content != original_content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    return False

def main():
    main_dir = Path('/home/akash/flxos-labs/flxos/main')
    
    updated_files = []
    
    for ext in ['*.cpp', '*.hpp']:
        for filepath in main_dir.rglob(ext):
            if update_file(filepath):
                updated_files.append(filepath)
    
    print(f"Updated {len(updated_files)} files:")
    for filepath in sorted(updated_files):
        print(f"  - {filepath.relative_to(main_dir.parent)}")

if __name__ == '__main__':
    main()
