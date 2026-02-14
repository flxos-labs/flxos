#!/usr/bin/env python3
"""
Update all references to the Services module:
1. Change namespace System::Services -> flx::services
2. Change include paths "core/services/" -> <flx/services/>
"""

import os
import re
from pathlib import Path

def update_file(filepath):
    """Update a single file with namespace and include path changes."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    
    # Update namespace declarations
    content = re.sub(r'namespace System::Services\s*{', r'namespace flx::services {', content)
    content = re.sub(r'}\s*//\s*namespace System::Services', r'} // namespace flx::services', content)
    
    # Update namespace usage (System::Services:: -> flx::services::)
    content = re.sub(r'System::Services::', r'flx::services::', content)
    
    # Update include paths for core service framework files
    content = re.sub(r'"core/services/IService\.hpp"', r'<flx/services/IService.hpp>', content)
    content = re.sub(r'"core/services/ServiceManifest\.hpp"', r'<flx/services/ServiceManifest.hpp>', content)
    content = re.sub(r'"core/services/ServiceRegistry\.hpp"', r'<flx/services/ServiceRegistry.hpp>', content)
    
    # Note: We keep service implementation includes as-is (e.g., "core/services/storage/SdCardService.hpp")
    # because those services haven't been extracted yet
    
    if content != original_content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        return True
    return False

def main():
    """Update all C++ files in the main directory."""
    main_dir = Path('/home/akash/flxos-labs/flxos/main')
    
    updated_files = []
    
    # Find all .cpp and .hpp files
    for ext in ['*.cpp', '*.hpp']:
        for filepath in main_dir.rglob(ext):
            if update_file(filepath):
                updated_files.append(filepath)
    
    print(f"Updated {len(updated_files)} files:")
    for filepath in sorted(updated_files):
        print(f"  - {filepath.relative_to(main_dir.parent)}")

if __name__ == '__main__':
    main()
