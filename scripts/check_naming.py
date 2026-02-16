#!/usr/bin/env python3
"""
Check naming conventions in FlxOS source files.
Conventions: Classes=PascalCase, functions=camelCase, constants=UPPER_SNAKE.
"""

import os
import re
import sys
from typing import List, Tuple

SEARCH_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def is_pascal_case(name: str) -> bool:
    """Check if name is PascalCase."""
    return bool(re.match(r'^[A-Z][a-zA-Z0-9]*$', name))

def is_camel_case(name: str) -> bool:
    """Check if name is camelCase (or starts with m_ for members)."""
    return bool(re.match(r'^(m_)?[a-z][a-zA-Z0-9]*$', name))

def is_upper_snake(name: str) -> bool:
    """Check if name is UPPER_SNAKE_CASE."""
    return bool(re.match(r'^[A-Z][A-Z0-9_]*$', name))

def check_file(filepath: str) -> List[str]:
    """Check naming conventions in a file."""
    issues = []
    rel_path = os.path.relpath(filepath, SEARCH_DIR)
    
    try:
        with open(filepath, 'r', errors='ignore') as f:
            content = f.read()
            lines = content.split('\n')
    except Exception:
        return issues
    
    # Check class names (exclude namespace qualifiers and namespace declarations)
    for match in re.finditer(r'\b(?:class|struct)\s+(\w+)', content):
        name = match.group(1)
        # Skip if the name is followed by '::' (it's a namespace qualifier, not a class name)
        end_pos = match.end()
        if end_pos < len(content) and content[end_pos:end_pos+2] == '::':
            continue
        # Skip if this line is a namespace declaration
        line_start = content.rfind('\n', 0, match.start()) + 1
        line_end = content.find('\n', match.start())
        if line_end == -1:
            line_end = len(content)
        line_text = content[line_start:line_end]
        if re.match(r'\s*namespace\b', line_text):
            continue
        if not is_pascal_case(name) and name not in ['__attribute__']:
            line_num = content[:match.start()].count('\n') + 1
            issues.append(f"{rel_path}:{line_num}: class/struct '{name}' should be PascalCase")
    
    # Check function definitions (simplified)
    func_pattern = re.compile(
        r'^\s*(?:static\s+|inline\s+|virtual\s+)?'
        r'(?:[\w:*&<>,\s]+)\s+'
        r'([a-z_]\w*)\s*\([^;]*\)\s*(?:const|override|noexcept|final|\s)*\{',
        re.MULTILINE
    )
    for match in func_pattern.finditer(content):
        name = match.group(1)
        # Skip ESP/LVGL macros, main, app_main, operators
        if name.startswith(('lv_', 'esp_', 'app_', 'main', 'operator', '_')):
            continue
        if not is_camel_case(name):
            line_num = content[:match.start()].count('\n') + 1
            issues.append(f"{rel_path}:{line_num}: function '{name}' should be camelCase")
    
    # Check constants (static const, constexpr, #define)
    const_patterns = [
        (r'static\s+(?:const|constexpr)\s+\w+\s+(\w+)\s*=', 'constant'),
        (r'#\s*define\s+([A-Za-z_]\w*)', 'macro'),
    ]
    for pattern, const_type in const_patterns:
        for match in re.finditer(pattern, content):
            name = match.group(1)
            # Skip common patterns
            if name.startswith(('CONFIG_', 'ESP_', 'LV_', 'TAG')):
                continue
            if const_type == 'macro' and not is_upper_snake(name):
                line_num = content[:match.start()].count('\n') + 1
                issues.append(f"{rel_path}:{line_num}: macro '{name}' should be UPPER_SNAKE_CASE")
    
    # Check file naming
    basename = os.path.basename(filepath)
    name_only = os.path.splitext(basename)[0]
    if filepath.endswith(('.cpp', '.hpp')):
        if not is_pascal_case(name_only) and name_only not in ['main']:
            issues.append(f"{rel_path}: filename '{name_only}' should be PascalCase")
    
    return issues

def main():
    print("=== FlxOS Naming Convention Check ===\n")
    
    all_issues = []
    file_count = 0
    
    target_dirs = {'main', 'System', 'UI', 'Connectivity', 'Kernel', 'Services', 'Core', 'Apps', 'Applications', 'Firmware', 'HAL'}
    
    for root, dirs, files in os.walk(SEARCH_DIR):
        if root == SEARCH_DIR:
             dirs[:] = [d for d in dirs if d in target_dirs]

        for file in files:
            if file.endswith(('.cpp', '.hpp', '.c', '.h')):
                filepath = os.path.join(root, file)
                issues = check_file(filepath)
                all_issues.extend(issues)
                file_count += 1
    
    if all_issues:
        print(f"⚠️  Naming Convention Issues ({len(all_issues)}):\n")
        
        # Group by type
        class_issues = [i for i in all_issues if 'class' in i or 'struct' in i]
        func_issues = [i for i in all_issues if 'function' in i]
        const_issues = [i for i in all_issues if 'macro' in i or 'constant' in i]
        file_issues = [i for i in all_issues if 'filename' in i]
        
        if class_issues:
            print(f"Classes/Structs ({len(class_issues)}):")
            for issue in class_issues[:10]:
                print(f"  {issue}")
        
        if func_issues:
            print(f"\nFunctions ({len(func_issues)}):")
            for issue in func_issues[:15]:
                print(f"  {issue}")
        
        if const_issues:
            print(f"\nConstants/Macros ({len(const_issues)}):")
            for issue in const_issues[:10]:
                print(f"  {issue}")
        
        if file_issues:
            print(f"\nFilenames ({len(file_issues)}):")
            for issue in file_issues:
                print(f"  {issue}")
    else:
        print("✅ All naming conventions are correct!")
    
    print(f"\n=== Summary ===")
    print(f"Files checked: {file_count}")
    print(f"Issues found: {len(all_issues)}")
    
    return 1 if all_issues else 0

if __name__ == "__main__":
    sys.exit(main())
