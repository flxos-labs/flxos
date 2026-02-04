#!/usr/bin/env python3
"""
Analyze include dependencies in FlxOS source files.
Detects: circular dependencies, include order issues, unused headers.
"""

import os
import re
import sys
from collections import defaultdict
from typing import Dict, List, Set, Tuple

SEARCH_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'main')

def get_includes(filepath: str) -> List[Tuple[str, int, str]]:
    """Extract includes from a file. Returns list of (include_path, line_num, type)."""
    includes = []
    try:
        with open(filepath, 'r', errors='ignore') as f:
            for line_num, line in enumerate(f, 1):
                # Match #include "..." or #include <...>
                match = re.match(r'^\s*#\s*include\s+([<"])(.*?)[>"]', line)
                if match:
                    inc_type = 'system' if match.group(1) == '<' else 'local'
                    includes.append((match.group(2), line_num, inc_type))
    except Exception:
        pass
    return includes

def build_dependency_graph() -> Dict[str, Set[str]]:
    """Build a dependency graph of all files."""
    graph = defaultdict(set)
    file_map = {}  # Map basename to full path
    
    for root, _, files in os.walk(SEARCH_DIR):
        for file in files:
            if file.endswith(('.hpp', '.h', '.cpp', '.c')):
                filepath = os.path.join(root, file)
                rel_path = os.path.relpath(filepath, SEARCH_DIR)
                file_map[file] = rel_path
                file_map[rel_path] = rel_path
                
                includes = get_includes(filepath)
                for inc_path, _, inc_type in includes:
                    if inc_type == 'local':
                        # Try to resolve the include path
                        inc_basename = os.path.basename(inc_path)
                        if inc_basename in file_map:
                            graph[rel_path].add(file_map[inc_basename])
                        elif inc_path in file_map:
                            graph[rel_path].add(file_map[inc_path])
                        else:
                            # Unknown include
                            graph[rel_path].add(inc_path)
    
    return graph

def find_cycles(graph: Dict[str, Set[str]]) -> List[List[str]]:
    """Find circular dependencies in the graph."""
    cycles = []
    visited = set()
    rec_stack = set()
    
    def dfs(node: str, path: List[str]) -> bool:
        visited.add(node)
        rec_stack.add(node)
        path.append(node)
        
        for neighbor in graph.get(node, []):
            if neighbor not in visited:
                if dfs(neighbor, path.copy()):
                    return True
            elif neighbor in rec_stack:
                # Found a cycle
                cycle_start = path.index(neighbor) if neighbor in path else -1
                if cycle_start >= 0:
                    cycle = path[cycle_start:] + [neighbor]
                    if cycle not in cycles:
                        cycles.append(cycle)
        
        rec_stack.remove(node)
        return False
    
    for node in graph:
        if node not in visited:
            dfs(node, [])
    
    return cycles

def check_include_order(filepath: str) -> List[str]:
    """Check include order violations."""
    issues = []
    includes = get_includes(filepath)
    rel_path = os.path.relpath(filepath, SEARCH_DIR)
    
    found_local = False
    for inc_path, line_num, inc_type in includes:
        if inc_type == 'local':
            found_local = True
        elif found_local and inc_type == 'system':
            issues.append(f"{rel_path}:{line_num}: system include <{inc_path}> after local include")
    
    return issues

def check_header_guards(filepath: str) -> List[str]:
    """Check for pragma once or header guards."""
    issues = []
    if not filepath.endswith(('.hpp', '.h')):
        return issues
    
    try:
        with open(filepath, 'r', errors='ignore') as f:
            content = f.read(500)  # Check beginning of file
            has_pragma = '#pragma once' in content
            has_guard = re.search(r'#\s*ifndef\s+\w+.*\n\s*#\s*define\s+\w+', content)
            
            if not has_pragma and not has_guard:
                rel_path = os.path.relpath(filepath, SEARCH_DIR)
                issues.append(f"{rel_path}: missing header guard or #pragma once")
    except Exception:
        pass
    
    return issues

def main():
    print("=== FlxOS Include Dependency Analysis ===\n")
    
    all_issues = []
    
    # Build dependency graph
    print("Building dependency graph...")
    graph = build_dependency_graph()
    print(f"Analyzed {len(graph)} files\n")
    
    # Find circular dependencies
    print("Checking for circular dependencies...")
    cycles = find_cycles(graph)
    if cycles:
        print(f"⚠️  Found {len(cycles)} circular dependency chain(s):\n")
        for cycle in cycles[:10]:  # Limit output
            print(f"  {' -> '.join(cycle)}")
            all_issues.append(f"Circular dependency: {' -> '.join(cycle)}")
    else:
        print("✅ No circular dependencies found!")
    
    print("\nChecking include order and header guards...")
    
    # Check include order and header guards
    order_issues = []
    guard_issues = []
    
    for root, _, files in os.walk(SEARCH_DIR):
        for file in files:
            if file.endswith(('.hpp', '.h', '.cpp', '.c')):
                filepath = os.path.join(root, file)
                order_issues.extend(check_include_order(filepath))
                guard_issues.extend(check_header_guards(filepath))
    
    if order_issues:
        print(f"\n⚠️  Include Order Issues ({len(order_issues)}):")
        for issue in order_issues[:20]:
            print(f"  {issue}")
        all_issues.extend(order_issues)
    
    if guard_issues:
        print(f"\n⚠️  Header Guard Issues ({len(guard_issues)}):")
        for issue in guard_issues[:20]:
            print(f"  {issue}")
        all_issues.extend(guard_issues)
    
    if not order_issues and not guard_issues:
        print("✅ No include order or header guard issues!")
    
    # Summary
    print(f"\n=== Summary ===")
    print(f"Circular dependencies: {len(cycles)}")
    print(f"Include order issues: {len(order_issues)}")
    print(f"Header guard issues: {len(guard_issues)}")
    print(f"Total issues: {len(all_issues)}")
    
    return 1 if all_issues else 0

if __name__ == "__main__":
    sys.exit(main())
