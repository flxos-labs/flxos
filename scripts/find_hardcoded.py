
import re
import os

patterns = [
    r'lv_obj_set_size\(.*,\s*([^,]+),\s*([^,\)]+)\)',
    r'lv_obj_set_width\(.*,\s*([^,\)]+)\)',
    r'lv_obj_set_height\(.*,\s*([^,\)]+)\)',
    r'lv_obj_set_x\(.*,\s*([^,\)]+)\)',
    r'lv_obj_set_y\(.*,\s*([^,\)]+)\)',
    r'lv_obj_set_pos\(.*,\s*([^,]+),\s*([^,\)]+)\)',
    r'lv_obj_set_style_pad_\w+\(.*,\s*([^,]+),\s*\d+\)',
    r'lv_obj_set_style_margin_\w+\(.*,\s*([^,]+),\s*\d+\)',
    r'lv_obj_set_style_gap_\w+\(.*,\s*([^,]+),\s*\d+\)',
    r'lv_obj_set_style_radius\(.*,\s*([^,]+),\s*\d+\)',
    r'lv_dpx\((\d+)\)',
    r'lv_pct\((\d+)\)'
]

exclude_values = {'0', '1', '100', 'LV_SIZE_CONTENT', 'LV_PCT(100)', 'LV_PCT(0)', 'LV_RADIUS_CIRCLE'}

search_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def is_hardcoded(val):
    val = val.strip()
    if val in exclude_values:
        return False
    if val.startswith('UiConstants::'):
        return False
    if val.startswith('LV_SYMBOL_'):
        return False
    if 'LayoutConstants' in val:
        return False
    # Check if it's just a number
    if re.match(r'^-?\d+(\.\d+)?$', val):
        if val not in exclude_values:
            return True
    # Check if it's lv_pct(val)
    m = re.match(r'lv_pct\((\d+)\)', val)
    if m:
        if m.group(1) not in exclude_values:
            return True
    # Check if it's lv_dpx(val)
    m = re.match(r'lv_dpx\((\d+)\)', val)
    if m:
        if m.group(1) not in exclude_values:
            return True
    return False

def find_hardcoded():
    results = []
    target_dirs = {'main', 'System', 'UI', 'Connectivity', 'Kernel', 'Services', 'Core', 'Applications', 'Firmware', 'HAL'}
    for root, dirs, files in os.walk(search_dir):
        if root == search_dir:
             dirs[:] = [d for d in dirs if d in target_dirs]
        for file in files:
            if file.endswith(('.cpp', '.hpp')):
                path = os.path.join(root, file)
                with open(path, 'r', errors='ignore') as f:
                    lines = f.readlines()
                    for i, line in enumerate(lines):
                        for p in patterns:
                            matches = re.finditer(p, line)
                            for m in matches:
                                values = m.groups()
                                if any(is_hardcoded(v) for v in values):
                                    results.append(f"{path}:{i+1}: {line.strip()}")
                                    break
    return results

if __name__ == "__main__":
    hardcoded = find_hardcoded()
    for h in hardcoded:
        print(h)
