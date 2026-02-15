#!/bin/bash
find main System UI Connectivity Kernel Services Core \( -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" \) -print0 \
| xargs -0 clang-format -i \
&& echo "FORMATTING COMPLETE"
