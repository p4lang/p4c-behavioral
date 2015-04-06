#!/usr/bin/env python

# Copyright 2013-present Barefoot Networks, Inc. 
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
import sys
from p4_hlir.main import HLIR
import smart
import shutil

def get_parser():
    parser = argparse.ArgumentParser(description='p4c-behavioral arguments')
    parser.add_argument('source', metavar='source', type=str,
                        help='a source file to include in the P4 program')

    parser.add_argument('--dump_yaml', action='store_true', default = False,
                        help='dump yaml representation of smart dictionary')
    parser.add_argument('--debug', action='store_true', default = False,
                        help='debugging mode')
    parser.add_argument('--optimize', action='store_true', default = False,
                        help='optimized mode')
    parser.add_argument('--with-tcam-cache',
                        action='store_true', default = False,
                        help='using experimental tcam cache for lookups')
    parser.add_argument('--with-cheap-tcam',
                        action='store_true', default = False,
                        help='using experimental (and buggy) tcam implementation')
    parser.add_argument('--thrift', action='store_true', default = False,
                        help='Generate and include thrift interfaces')
    parser.add_argument('--gen-dir', dest='gen_dir', default = "output",
                        help="destination directory for generated files")
    parser.add_argument('--templates-dir', dest='templates_dir', default = "templates",
                        help="templates directory")
    parser.add_argument('--p4-name', type=str, dest='p4_name',
                        help='The name to use for the program')
    parser.add_argument('--p4-prefix', type=str, dest='p4_prefix',
                        help='The name to use for the runtime APIs')
    parser.add_argument('--public-inc-path', dest='public_inc_path', default="",
                        help="Path to use for rmt.h, pd.h, ... references")
    parser.add_argument('--meta-config', dest='meta_config',
                        default = "",
                        help="Path to json file specifying metadata config")

    return parser

def _get_p4_basename(p4_source):
    return os.path.splitext(os.path.basename(p4_source))[0]

def _validate_dir(dir_name):
    if not os.path.isdir(args.gen_dir):
        print args.gen_dir, "is not a valid directory"
        sys.exit(1)
    return os.path.abspath(args.gen_dir)

def main():
    parser = get_parser()
    args = parser.parse_args()

    _TEMPLATES_DIR = os.path.dirname(os.path.realpath(__file__))  
    gen_dir = os.path.abspath(args.gen_dir)
    templates_dir = os.path.join(_TEMPLATES_DIR, args.templates_dir)
    if os.path.exists(gen_dir):
        if not os.path.isdir(gen_dir):
            sys.stderr.write(args.gen_dir + " exists but is not a directory\n")
            sys.exit(1)
    else:
        try:
            os.mkdir(gen_dir)
        except:
            sys.stderr.write("Could not create output directory %s\n" %
                             args.gen_dir)
            sys.exit(1) 

    if args.p4_name:
        p4_name = args.p4_name
    else:
        p4_name = _get_p4_basename(args.source)

    if args.p4_prefix:
        p4_prefix = args.p4_prefix
    else:
        p4_prefix = p4_name

    h = HLIR(args.source)
    h.add_preprocessor_args("-D__TARGET_BM__")
    if not h.build():
        print "Error while building HLIR"
        sys.exit(1)

    print "Generating files in directory", gen_dir

    render_dict = smart.render_dict_create(h, 
                                           p4_name, p4_prefix,
                                           args.meta_config,
                                           args.public_inc_path,
                                           dump_yaml = args.dump_yaml)
    smart.render_all_files(render_dict, gen_dir, templates_dir,
                           with_thrift = args.thrift)

if __name__ == "__main__":
    main()
