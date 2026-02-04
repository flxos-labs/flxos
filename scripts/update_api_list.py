import os
import re
import datetime

def get_prefix(api):
    parts = api.split('_')
    if len(parts) < 2:
        return "other"
    
    # Common multi-word prefixes for better grouping
    multi_word = [
        "lv_draw_sw", "lv_draw_vector", "lv_draw_layer", "lv_draw_mask", "lv_draw_image",
        "lv_ime_pinyin", "lv_file_explorer", "lv_binfont_loader", "lv_font_fmt_txt", 
        "lv_font_manager", "lv_anim_timeline", "lv_draw_buf", "lv_draw_arc", "lv_draw_rect",
        "lv_draw_label", "lv_draw_line", "lv_draw_triangle", "lv_draw_border",
        "lv_font_glyph", "lv_draw_glyph", "lv_draw_rect_dsc", "lv_draw_label_dsc"
    ]
    for mw in multi_word:
        if api.startswith(mw + "_") or api == mw:
            return mw
            
    if api.startswith("lv_draw_"):
        if len(parts) == 3: return "lv_draw"
        return parts[0] + "_" + parts[1]

    if api.startswith("lv_obj_"):
        return "lv_obj"

    return parts[0] + "_" + parts[1]

def update_api_list():
    header_cmd = 'find components/lvgl/src -name "*.h" ! -name "*_private.h" ! -path "*/private/*"'
    headers = os.popen(header_cmd).read().splitlines()
    
    if not headers:
        print("No headers found. Check paths.")
        return

    # Using chr(40) for '(' to avoid backslash issues in tool transport
    api_regex_str = r'^\s*(?:[A-Z_]+\s+)?(?:[a-zA-Z0-9_]+\s*\*?\s+)+(lv_[a-zA-Z0-9_]+)\s*' + re.escape('(')
    api_regex = re.compile(api_regex_str)
    
    found_apis = set()
    for header in headers:
        try:
            with open(header, 'r', encoding='utf-8', errors='ignore') as f:
                for line in f:
                    match = api_regex.search(line)
                    if match:
                        found_apis.add(match.group(1))
        except Exception:
            pass

    if not found_apis:
        fallback_regex = re.compile(r'\b(lv_[a-zA-Z0-9_]+)\s*' + re.escape('('))
        for header in headers:
            try:
                with open(header, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    matches = fallback_regex.findall(content)
                    for m in matches:
                        found_apis.add(m)
            except Exception:
                pass

    # Categorize
    sections = {}
    for api in sorted(list(found_apis)):
        prefix = get_prefix(api)
        if prefix not in sections:
            sections[prefix] = []
        sections[prefix].append(api)

    # Sort sections by size
    sorted_sections = sorted(sections.items(), key=lambda x: len(x[1]), reverse=True)

    # Write to file
    output_path = "LVGL_full_api_list.md"
    now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    with open(output_path, "w", encoding='utf-8') as f:
        f.write("# LVGL API List\n\n")
        f.write("*Last Updated: {}*\n\n".format(now))
        f.write("Sorted from highly featured (most functions) to least featured.\n\n")
        
        for prefix, api_list in sorted_sections:
            f.write("## {} ({} APIs)\n".format(prefix, len(api_list)))
            for api in sorted(api_list):
                f.write("- `{}`\n".format(api))
            f.write("\n")
    
    print("Updated {} with {} APIs.".format(output_path, len(found_apis)))

if __name__ == "__main__":
    update_api_list()