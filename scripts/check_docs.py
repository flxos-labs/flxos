#!/usr/bin/env python3
"""
Check documentation coverage in FlxOS source files.
Checks: file headers, function docs, TODO/FIXME tracking, README presence.
"""

import os
import re
import sys
from typing import List, Dict

SEARCH_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def check_file_header(filepath: str) -> bool:
    """Check if file has a header comment."""
    try:
        with open(filepath, 'r', errors='ignore') as f:
            content = f.read(500)
            # Check for block comment or series of line comments at start
            has_block = content.lstrip().startswith('/*') or content.lstrip().startswith('//')
            return has_block
    except Exception:
        return True

def find_todos(filepath: str) -> List[tuple]:
    """Find TODO/FIXME/HACK comments."""
    todos = []
    try:
        with open(filepath, 'r', errors='ignore') as f:
            for line_num, line in enumerate(f, 1):
                match = re.search(r'(TODO|FIXME|HACK|XXX)[:.]?\s*(.*)$', line, re.IGNORECASE)
                if match:
                    todos.append((line_num, match.group(1).upper(), match.group(2).strip()[:60]))
    except Exception:
        pass
    return todos

def check_function_docs(filepath: str) -> List[str]:
    """Check for undocumented public functions."""
    undocumented = []
    if not filepath.endswith(('.hpp', '.h')):
        return undocumented
    
    try:
        with open(filepath, 'r', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return undocumented
    
    rel_path = os.path.relpath(filepath, SEARCH_DIR)
    in_public = True  # Assume public by default for structs
    
    for i, line in enumerate(lines):
        # Track public/private sections
        if 'private:' in line or 'protected:' in line:
            in_public = False
        elif 'public:' in line:
            in_public = True
        
        # Look for function declarations (not definitions)
        if in_public and re.match(r'^\s*(?:virtual\s+)?[\w:*&<>,\s]+\s+\w+\s*\([^;]*\)\s*(?:const|override|noexcept|=)*.+;', line):
            # Check if previous lines have documentation
            has_doc = False
            for j in range(max(0, i-3), i):
                if '///' in lines[j] or '/**' in lines[j] or '*/' in lines[j] or '@' in lines[j]:
                    has_doc = True
                    break
            
            if not has_doc:
                func_match = re.search(r'(\w+)\s*\(', line)
                if func_match:
                    func_name = func_match.group(1)
                    # Skip common patterns
                    if func_name not in ['getInstance', 'get', 'set'] and not func_name.startswith('_'):
                        undocumented.append(f"{rel_path}:{i+1}: '{func_name}' lacks documentation")
    
    return undocumented

def check_readme_presence() -> Dict[str, bool]:
    """Check for README.md in each major module directory."""
    readme_status = {}
    module_dirs = {'main', 'System', 'UI', 'Connectivity', 'Kernel', 'Services', 'Core'}
    for item in os.listdir(SEARCH_DIR):
        item_path = os.path.join(SEARCH_DIR, item)
        if os.path.isdir(item_path) and item in module_dirs:
            readme_path = os.path.join(item_path, 'README.md')
            readme_status[item] = os.path.exists(readme_path)
    return readme_status

def main():
    print("=== FlxOS Documentation Coverage Check ===\n")
    
    files_without_headers = []
    all_todos = []
    undoc_functions = []
    file_count = 0
    
    target_dirs = {'main', 'System', 'UI', 'Connectivity', 'Kernel', 'Services', 'Core'}
    
    for root, dirs, files in os.walk(SEARCH_DIR):
        if root == SEARCH_DIR:
             dirs[:] = [d for d in dirs if d in target_dirs]

        for file in files:
            if file.endswith(('.cpp', '.hpp', '.c', '.h')):
                filepath = os.path.join(root, file)
                rel_path = os.path.relpath(filepath, SEARCH_DIR)
                file_count += 1
                
                # Check file headers
                if not check_file_header(filepath):
                    files_without_headers.append(rel_path)
                
                # Find TODOs
                todos = find_todos(filepath)
                for line_num, todo_type, text in todos:
                    all_todos.append((rel_path, line_num, todo_type, text))
                
                # Check function docs
                undoc_functions.extend(check_function_docs(filepath))
    
    # Report file headers
    if files_without_headers:
        print(f"⚠️  Files Without Headers ({len(files_without_headers)}):")
        for f in files_without_headers[:10]:
            print(f"  {f}")
    else:
        print("✅ All files have header comments!")
    
    # Report TODOs
    print(f"\n📝 TODO/FIXME Comments ({len(all_todos)}):")
    if all_todos:
        todo_count = len([t for t in all_todos if t[2] == 'TODO'])
        fixme_count = len([t for t in all_todos if t[2] == 'FIXME'])
        hack_count = len([t for t in all_todos if t[2] in ['HACK', 'XXX']])
        
        print(f"  TODO: {todo_count}, FIXME: {fixme_count}, HACK/XXX: {hack_count}")
        print("\n  Recent items:")
        for rel_path, line_num, todo_type, text in all_todos[:10]:
            print(f"    [{todo_type}] {rel_path}:{line_num}: {text}")
    
    # Report undocumented functions
    if undoc_functions:
        print(f"\n⚠️  Undocumented Public Functions ({len(undoc_functions)}):")
        for func in undoc_functions[:15]:
            print(f"  {func}")
    else:
        print("\n✅ All public functions are documented!")
    
    # README check
    readme_status = check_readme_presence()
    print(f"\n📁 Module README Status:")
    for module, has_readme in sorted(readme_status.items()):
        status = "✅" if has_readme else "❌"
        print(f"  {status} {module}/README.md")
    
    # Summary
    missing_readmes = sum(1 for v in readme_status.values() if not v)
    total_issues = len(files_without_headers) + len(undoc_functions) + missing_readmes
    
    print(f"\n=== Summary ===")
    print(f"Files checked: {file_count}")
    print(f"Files without headers: {len(files_without_headers)}")
    print(f"TODO/FIXME comments: {len(all_todos)}")
    print(f"Undocumented functions: {len(undoc_functions)}")
    print(f"Missing READMEs: {missing_readmes}")
    
    return 1 if total_issues > 0 else 0

if __name__ == "__main__":
    sys.exit(main())
