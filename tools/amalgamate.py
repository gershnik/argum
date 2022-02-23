#  Copyright 2022 Eugene Gershnik
#
#  Use of this source code is governed by a BSD-style
#  license that can be found in the LICENSE file or at
#  https://github.com/gershnik/argum/blob/master/LICENSE
#
import sys
import re
from pathlib import Path
from typing import Dict, List


line_comment_re = re.compile(r'\s*//.*')
sys_include_re = re.compile(r'\s*#\s*include\s+<([^>]+)>\s*')
user_include_re = re.compile(r'\s*#\s*include\s+"([^"]+)"\s*')


def processHeader(dir: Path, path: Path, sys_includes: List[str], processed_headers:Dict[str, bool], strip_initial_comment: bool):

    ret = ""
    initial_comment = strip_initial_comment
    with open(path, "r") as headerfile:
        for line in headerfile:
            m = line_comment_re.match(line)
            if m and initial_comment:
                continue
            initial_comment = False
            m = sys_include_re.match(line)
            if m:
                sys_includes.append(m.group(1))
                continue
            m = user_include_re.match(line)
            if m:
                name = m.group(1)
                if not processed_headers.get(name):
                    new_path = (dir / name).absolute()
                    ret += processHeader(new_path.parent, new_path, sys_includes, processed_headers, strip_initial_comment=True)
                    processed_headers[name] = True
            else:
                ret += line
    return ret


def combineHeaders(dir: Path, template: Path, output: Path):
    sys_includes = []
    processed_headers = {}

    text = processHeader(dir, template, sys_includes, processed_headers, strip_initial_comment=False)

    sys_includes = list(set(sys_includes))
    sys_includes.sort()
    sys_includes_text = ""
    for sys_include in sys_includes:
        sys_includes_text += ("\n#include <" + sys_include + ">")

    text = text.replace("##SYS_INCLUDES##", sys_includes_text)
    text = text.replace("##NAME##", output.name.replace('.', '_').upper())

    output.parent.mkdir(parents=True, exist_ok=True)
    with open(output, "w") as outfile:
        print(text, file=outfile)

def amalgamate():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--dir', '-d', default=str(Path.cwd))
    parser.add_argument('template')
    parser.add_argument('output')
    args = parser.parse_args()

    combineHeaders(Path(args.dir), Path(args.template), Path(args.output))

if __name__ == "__main__":
	amalgamate()