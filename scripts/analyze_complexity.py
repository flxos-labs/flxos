#!/usr/bin/env python3
"""
Analyze code complexity metrics for FlxOS source files.
Checks: function length, parameter count, nesting depth, cyclomatic complexity.
"""

import os
import re
import sys
from dataclasses import dataclass
from typing import List, Tuple

SEARCH_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Thresholds
MAX_FUNCTION_LINES = 80
MAX_PARAMETERS = 6
MAX_NESTING_DEPTH = 4
MAX_CYCLOMATIC = 15

@dataclass
class FunctionMetrics:
    file: str
    name: str
    line: int
    lines: int
    params: int
    nesting: int
    cyclomatic: int

def count_parameters(signature: str) -> int:
    """Count function parameters from signature."""
    match = re.search(r'\(([^)]*)\)', signature)
    if not match:
        return 0
    params = match.group(1).strip()
    if not params or params == 'void':
        return 0
    return len([p for p in params.split(',') if p.strip()])

def calculate_nesting(lines: List[str]) -> int:
    """Calculate maximum nesting depth."""
    max_depth = 0
    current_depth = 0
    for line in lines:
        current_depth += line.count('{') - line.count('}')
        max_depth = max(max_depth, current_depth)
    return max_depth

def calculate_cyclomatic(lines: List[str]) -> int:
    """Calculate cyclomatic complexity (simplified)."""
    complexity = 1
    patterns = [
        r'\bif\s*\(',
        r'\belse\s+if\s*\(',
        r'\bfor\s*\(',
        r'\bwhile\s*\(',
        r'\bcase\s+',
        r'\bcatch\s*\(',
        r'\?\s*[^:]*:',  # ternary
        r'\&\&',
        r'\|\|',
    ]
    content = '\n'.join(lines)
    for pattern in patterns:
        complexity += len(re.findall(pattern, content))
    return complexity

def extract_functions(filepath: str) -> List[FunctionMetrics]:
    """Extract function metrics from a file."""
    functions = []
    try:
        with open(filepath, 'r', errors='ignore') as f:
            content = f.read()
            lines = content.split('\n')
    except Exception:
        return functions

    # Simple regex to find function definitions
    func_pattern = re.compile(
        r'^(?:(?:static|inline|virtual|explicit|constexpr|override|const)\s+)*'
        r'(?:[\w:*&<>,\s]+)\s+'
        r'([\w:]+)\s*'
        r'\(([^)]*)\)\s*'
        r'(?:const|override|noexcept|final|\s)*'
        r'\{',
        re.MULTILINE
    )

    for match in func_pattern.finditer(content):
        func_name = match.group(1)
        start_pos = match.start()
        start_line = content[:start_pos].count('\n') + 1
        
        # Find matching closing brace
        brace_count = 1
        end_pos = match.end()
        while end_pos < len(content) and brace_count > 0:
            if content[end_pos] == '{':
                brace_count += 1
            elif content[end_pos] == '}':
                brace_count -= 1
            end_pos += 1
        
        end_line = content[:end_pos].count('\n') + 1
        func_lines = lines[start_line-1:end_line]
        
        metrics = FunctionMetrics(
            file=filepath,
            name=func_name,
            line=start_line,
            lines=len(func_lines),
            params=count_parameters(match.group(0)),
            nesting=calculate_nesting(func_lines),
            cyclomatic=calculate_cyclomatic(func_lines)
        )
        functions.append(metrics)
    
    return functions

def analyze_file(filepath: str) -> Tuple[List[str], List[FunctionMetrics]]:
    """Analyze a single file and return issues."""
    issues = []
    functions = extract_functions(filepath)
    rel_path = os.path.relpath(filepath, SEARCH_DIR)
    
    for func in functions:
        if func.lines > MAX_FUNCTION_LINES:
            issues.append(f"{rel_path}:{func.line}: function '{func.name}' is {func.lines} lines (max: {MAX_FUNCTION_LINES})")
        if func.params > MAX_PARAMETERS:
            issues.append(f"{rel_path}:{func.line}: function '{func.name}' has {func.params} parameters (max: {MAX_PARAMETERS})")
        if func.nesting > MAX_NESTING_DEPTH:
            issues.append(f"{rel_path}:{func.line}: function '{func.name}' has nesting depth {func.nesting} (max: {MAX_NESTING_DEPTH})")
        if func.cyclomatic > MAX_CYCLOMATIC:
            issues.append(f"{rel_path}:{func.line}: function '{func.name}' has cyclomatic complexity {func.cyclomatic} (max: {MAX_CYCLOMATIC})")
    
    return issues, functions

def main():
    print("=== FlxOS Code Complexity Analysis ===\n")
    
    all_issues = []
    all_functions = []
    file_count = 0
    
    target_dirs = {'System', 'UI', 'Connectivity', 'Kernel', 'Services', 'Core', 'Apps', 'Applications', 'Firmware', 'HalModule', 'Profiles'}
    
    for root, dirs, files in os.walk(SEARCH_DIR):
        if root == SEARCH_DIR:
             dirs[:] = [d for d in dirs if d in target_dirs]
             
        for file in files:
            if file.endswith(('.cpp', '.c')):
                filepath = os.path.join(root, file)
                issues, functions = analyze_file(filepath)
                all_issues.extend(issues)
                all_functions.extend(functions)
                file_count += 1
    
    # Print issues
    if all_issues:
        print("⚠️  Complexity Issues Found:\n")
        for issue in sorted(all_issues):
            print(f"  {issue}")
    else:
        print("✅ No complexity issues found!")
    
    # Print summary
    print(f"\n=== Summary ===")
    print(f"Files analyzed: {file_count}")
    print(f"Functions found: {len(all_functions)}")
    print(f"Issues found: {len(all_issues)}")
    
    # Top complex functions
    if all_functions:
        print(f"\n=== Top 10 Most Complex Functions ===")
        sorted_funcs = sorted(all_functions, key=lambda f: f.cyclomatic, reverse=True)[:10]
        for func in sorted_funcs:
            rel_path = os.path.relpath(func.file, SEARCH_DIR)
            print(f"  {rel_path}:{func.line} - {func.name}: complexity={func.cyclomatic}, lines={func.lines}")
    
    return 1 if all_issues else 0

if __name__ == "__main__":
    sys.exit(main())
