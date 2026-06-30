#!/usr/bin/env python3
"""Convert test fixtures: @test(expect=N) → @test, void return, add imports.

Only affects @test functions. Helper functions that return u64 are NOT changed.
"""

import os
import re

FIXTURES_DIR = "tests/fixtures"

def process_file(filepath):
    with open(filepath) as f:
        content = f.read()
    
    name = os.path.basename(filepath)
    
    # 0. Extract expected values from @test(expect=N) before removing them
    expect_map = {}
    lines = content.split('\n')
    for i, line in enumerate(lines):
        m = re.match(r'@test\(expect=(\d+)\)', line.strip())
        if m:
            expected = int(m.group(1))
            for j in range(i + 1, min(i + 3, len(lines))):
                fm = re.match(r'^func\s+(\w+)', lines[j])
                if fm:
                    expect_map[fm.group(1)] = expected
                    break
    
    # 1. Replace @test(expect=N) with @test
    content = re.sub(r'@test\(expect=\d+\)', '@test', content)
    
    # 2. Add import "std/test" if not present (only for files with @test)
    has_import_test = 'import "std/test"' in content or "import 'std/test'" in content
    has_any_import = re.search(r'^import\s+', content, re.MULTILINE)
    has_test_annotation = '@test' in content
    
    if not has_import_test and has_test_annotation:
        if has_any_import:
            content = re.sub(r'^(import[^\n]*\n)(?!import)', r'\1import "std/test"\n', content, count=1, flags=re.MULTILINE)
        else:
            lines = content.split('\n')
            insert_pos = 0
            for i, line in enumerate(lines):
                stripped = line.strip()
                if stripped == '' or stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
                    insert_pos = i + 1
                else:
                    break
            lines.insert(insert_pos, 'import "std/test"')
            lines.insert(insert_pos + 1, '')
            content = '\n'.join(lines)
    
    # 3. Process line by line, tracking @test function boundaries
    lines = content.split('\n')
    new_lines = []
    in_test_func = False
    current_func_name = None
    brace_depth = 0
    i = 0
    
    while i < len(lines):
        line = lines[i]
        
        # Detect @test annotation (on its own line or same line as func)
        if line.strip() == '@test':
            in_test_func = True
            new_lines.append(line)
            i += 1
            continue
        
        # Also detect @test on same line as func declaration
        if re.match(r'@test\s+func\s+', line):
            in_test_func = True
            # Don't add the line yet - let the func detection handle it
        
        # Detect function declaration after @test
        if in_test_func:
            m = re.match(r'^(func\s+(\w+)\s*\([^)]*\))\s*:\s*(u64|int)', line)
            if m:
                current_func_name = m.group(2)
                line = re.sub(r'(func\s+\w+\s*\([^)]*\))\s*:\s*(u64|int)', r'\1: void', line)
                brace_depth = 0
                for ch in line:
                    if ch == '{': brace_depth += 1
                    if ch == '}': brace_depth -= 1
                new_lines.append(line)
                i += 1
                continue
        
        if in_test_func:
            # Track brace depth
            for ch in line:
                if ch == '{': brace_depth += 1
                if ch == '}': brace_depth -= 1
            
            # Handle return statements (at any depth in @test function)
            m = re.match(r'^(\s*)return\s+(.*?)\s*$', line)
            if m:
                indent = m.group(1)
                expr_raw = m.group(2)
                # Strip inline comments
                expr = re.sub(r'\s*//.*$', '', expr_raw).strip()
                expected = expect_map.get(current_func_name, 0)
                
                # Simple constant - remove entirely
                if re.match(r'^\d+$', expr) or re.match(r'^0x[0-9a-fA-F]+$', expr):
                    i += 1
                    continue
                # Has function calls or assignments - keep as statement
                if '(' in expr or '=' in expr:
                    new_lines.append(f'{indent}{expr}')
                    i += 1
                    continue
                # Variable or expression - convert to assertEquals
                new_lines.append(f'{indent}assertEquals({expected}, {expr}, "{name}")')
                i += 1
                continue
            
            # Exit test function when brace depth returns to 0
            if brace_depth <= 0:
                in_test_func = False
                current_func_name = None
        
        new_lines.append(line)
        i += 1
    
    content = '\n'.join(new_lines)
    
    with open(filepath, 'w') as f:
        f.write(content)

def main():
    fixtures = sorted(os.listdir(FIXTURES_DIR))
    fixtures = [f for f in fixtures if f.endswith('.ae')]
    
    for fixture in fixtures:
        filepath = os.path.join(FIXTURES_DIR, fixture)
        process_file(filepath)
        print(f"  Processed: {fixture}")

if __name__ == '__main__':
    main()
